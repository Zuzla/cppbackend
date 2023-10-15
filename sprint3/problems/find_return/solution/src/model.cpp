#include "model.h"
#include "loots.h"
#include "loot_generator.h"

#include <stdexcept>
#include <functional>

namespace model
{

    using namespace std::literals;

    void Dog::SetPosition(const double &x, const double &y)
    {
        position_.x = x;
        position_.y = y;
    }

    void Dog::SetSpeed(const int32_t &x, const int32_t &y)
    {
        speed_.x = x;
        speed_.y = y;
    }

    template <typename T>
    bool IsInBounds(const T &value, T low, T high)
    {
        if (low > high)
            std::swap(low, high);

        return (value >= low) && (value <= high);
    }

    void Dog::UpdateHorizontal(const Road &&road, const double &x)
    {
        double start = road.GetStart().x;
        double end = road.GetEnd().x;

        if (start > end)
            std::swap(start, end);

        if (IsInBounds<double>(x, start, end))
        {
            position_.x = x;
        }
        else
        {
            if (x > end)
            {
                position_.x = end + kRoadWidth;
            }
            else
            {
                position_.x = start - kRoadWidth;
            }

            speed_.x = 0;
            speed_.y = 0;
        }
    }

    void Dog::UpdateVertical(const Road &&road, const double &y)
    {
        double start = road.GetStart().y;
        double end = road.GetEnd().y;

        if (start > end)
            std::swap(start, end);

        if (IsInBounds<double>(y, start, end))
        {
            position_.y = y;
        }
        else
        {
            if (y > end)
            {
                position_.y = end + kRoadWidth;
            }
            else
            {
                position_.y = start - kRoadWidth;
            }

            speed_.x = 0;
            speed_.y = 0;
        }
    }

    void Dog::Tick(std::chrono::milliseconds time)
    {
        if (time.count() == 0)
            return;

        double time_diff_ = time.count() / kMillisecondsToSeconds;

        double x = position_.x + speed_.x * time_diff_;
        double y = position_.y + speed_.y * time_diff_;

        /*
        TO DO.
        Чтобы быстро находить участок дороги по координатам, построить вспомогательную структуру данных, чтобы вместо
        линейного перебора всех дорог на карте искать дороги внутри map, unordered_map или отсортированного vector.
        */
        auto roads_ = map_->GetRoads();
        std::vector<Road> vec_road;
        for (auto &road : roads_)
        {
            auto check_x_ = IsInBounds<int>(std::round(position_.x), road.GetStart().x, road.GetEnd().x);
            auto check_y_ = IsInBounds<int>(std::round(position_.y), road.GetStart().y, road.GetEnd().y);

            if (!check_x_ || !check_y_)
                continue;

            vec_road.push_back(road);
        }

        if (vec_road.empty())
            return;

        for (const auto &road : vec_road)
        {
            bool p1 = road.IsHorizontal();
            bool p2 = direction_ == kLeftDirection || direction_ == kRightDirection;

            if (p1 && p2)
            {
                UpdateHorizontal(std::move(road), x);
                UpdateRoad(std::move(road));
                return;
            }

            p1 = road.IsVertical();
            p2 = direction_ == kUpDirection || direction_ == kDownDirection;

            if (p1 && p2)
            {
                UpdateVertical(std::move(road), y);
                UpdateRoad(std::move(road));
                return;
            }
        }

        if (direction_ == kLeftDirection)
        {

            if (x < static_cast<int64_t>(std::round(position_.x) - kRoadWidth))
            {
                position_.x = std::round(position_.x) - kRoadWidth;
                speed_.x = 0;
                speed_.y = 0;
            }
            else
            {
                position_.x = x;
            }
        }
        else if (direction_ == kRightDirection)
        {
            if (x > static_cast<int64_t>(std::round(position_.x) + kRoadWidth))
            {
                position_.x = std::round(position_.x) + kRoadWidth;
                speed_.x = 0;
                speed_.y = 0;
            }
            else
            {
                position_.x = x;
            }
        }
        else if (direction_ == kUpDirection)
        {
            if (y < static_cast<int64_t>(std::round(position_.y) - kRoadWidth))
            {
                position_.y = std::round(position_.y) - kRoadWidth;
                speed_.x = 0;
                speed_.y = 0;
            }
            else
            {
                position_.y = y;
            }
        }
        else if (direction_ == kDownDirection)
        {
            if (y > static_cast<int64_t>(std::round(position_.y) + kRoadWidth))
            {
                position_.y = std::round(position_.y) + kRoadWidth;
                speed_.x = 0;
                speed_.y = 0;
            }
            else
            {
                position_.y = y;
            }
        }
    }

    void Dog::SetDirection(const std::string &direction)
    {
        direction_ = direction;

        int32_t current_speed_ = map_->GetDogSpeed();

        if (direction_ == kLeftDirection)
        {
            SetSpeed(-current_speed_, 0);
        }
        else if (direction_ == kRightDirection)
        {
            SetSpeed(current_speed_, 0);
        }
        else if (direction_ == kUpDirection)
        {
            SetSpeed(0, -current_speed_);
        }
        else if (direction_ == kDownDirection)
        {
            SetSpeed(0, current_speed_);
        }
        else if (direction_.empty())
        {
            SetSpeed(0, 0);
        }
    }

    void Dog::UpdateRoad(const Road &&road)
    {
        if (current_road_ == nullptr)
        {
            current_road_ = std::make_shared<Road>(road);
            return;
        }

        if (*current_road_.get() == road)
        {
            return;
        }

        *current_road_ = road;
    }

    void Dog::AddItemToBag(MapLoot &&item)
    {
        bag_.push_back(item);
    }

    MapLoot *Dog::GetItemFromBag(size_t id)
    {
        for (auto &loot : bag_)
        {
            if (id == loot.id_)
                return &loot;
        }
    }

    size_t Dog::GetScore() const noexcept
    {
        return score_;
    }

    void Dog::ClearBag() noexcept
    {
        for (auto &loot : bag_)
        {
            score_ += loot.value_;
        }

        bag_.erase(bag_.begin(), bag_.end());
    }

    std::vector<MapLoot> Dog::GetBag() const noexcept
    {
        return bag_;
    }

    std::shared_ptr<Road> Dog::GetRoad() const noexcept
    {
        return current_road_;
    }

    void Game::AddMap(Map &&map)
    {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted)
        {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        }
        else
        {
            try
            {
                maps_.emplace_back(std::move(map));
            }
            catch (...)
            {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    void Game::AddGameSession(std::shared_ptr<model::GameSession> &game_session)
    {
        const size_t index = game_sessions_.size();
        if (auto [it, inserted] = game_session_id_to_index_.emplace(game_session->GetId(), index); !inserted)
        {
            throw std::invalid_argument("Game Session with id "s + *game_session->GetId() + " already exists"s);
        }
        else
        {
            try
            {
                game_sessions_.emplace_back(std::move(game_session));
            }
            catch (...)
            {
                std::cout << "exception\n";
                game_session_id_to_index_.erase(it);
                throw;
            }
        }
    }

    void Game::Tick(std::chrono::milliseconds time, add_data::GameLoots &game_loots)
    {
        for (auto &game_session : game_sessions_)
        {
            game_session->LootGenerator(game_loots, time);
            game_session->Tick(time);
            game_session->FindCollision();
        }
    }

    void Game::DisconnectSession(GameSession *game_session_, Dog *dog_)
    {
        game_session_->DeleteDog(dog_->GetId());
        delete dog_;
    }

    void GameSession::SetPositionDog(std::shared_ptr<Dog> &dog, const bool default_spawn)
    {
        if (default_spawn)
        {
            dog->SetPosition(0, 0);
        }
        else
        {
            auto roads = map_.GetRoads();
            auto road = roads.at(GenerateNum(1, roads.size()));
            auto x = GenerateNum(road.GetStart().x, road.GetEnd().x);
            auto y = GenerateNum(road.GetStart().y, road.GetEnd().y);

            dog->SetPosition(x, y);
        }
    }

    void GameSession::AddDog(std::shared_ptr<Dog> &dog, const bool default_spawn)
    {
        SetPositionDog(dog, default_spawn);

        const size_t index = dogs_.size();
        if (auto [it, inserted] = dog_id_to_index_.emplace(dog->GetId(), index); !inserted)
        {
            delete &dog;
            throw std::invalid_argument("Dog with id "s + *dog->GetId() + " already exists"s);
        }
        else
        {
            try
            {
                dogs_.emplace_back(std::move(dog));
            }
            catch (...)
            {
                dog_id_to_index_.erase(it);
                throw;
            }
        }
    }

    std::shared_ptr<Dog> GameSession::FindDog(const Dog::Id &id) const noexcept
    {
        if (auto it = dog_id_to_index_.find(id); it != dog_id_to_index_.end())
        {
            return dogs_.at(it->second);
        }
        return nullptr;
    }

    void GameSession::Tick(std::chrono::milliseconds time)
    {
        for (auto &dog_ : dogs_)
        {
            dog_->Tick(time);
        }
    }

    void GameSession::LootGenerator(add_data::GameLoots &game_loots, std::chrono::milliseconds delta)
    {
        auto current_map_loot = map_.GetLoots();
        auto all_loot = game_loots.GetLoot(map_.GetName());

       // for (size_t i = 0; i < map_.GetRoads().size(); ++i)        {
        auto count_ = game_loots.GetGenerator()->Generate(delta, current_map_loot.size(), dogs_.size());
        for (size_t i = 0; i < count_; ++i)
        {
            auto roads = map_.GetRoads();
            auto road = roads.at(GenerateNum(1, roads.size()));
         //   auto road = roads.at(i);
            auto x = GenerateNum(road.GetStart().x, road.GetEnd().x);
            auto y = GenerateNum(road.GetStart().y, road.GetEnd().y);
            auto type = GenerateNum(0, all_loot.size() - 1);

            MapLoot new_map_loot(num_loot_++, all_loot.at(type).value_, type, x, y);
            map_.AddLoot(new_map_loot);
        }
    }

    void GameSession::FindCollision()
    {
        collision_detector::Provider provider{};

        for (const auto &dog : dogs_)
        {
            std::string id_str = *dog->GetId();
            double curr_position_x = dog->GetPosition().x;
            double curr_position_y = dog->GetPosition().y;

            double end_position_x = curr_position_x;
            double end_position_y = curr_position_y;

            auto road = dog->GetRoad();
            if (road != nullptr)
            {
                end_position_x = static_cast<double>(road->GetEnd().x);
                end_position_y = static_cast<double>(road->GetEnd().y);
            }

            collision_detector::Gatherer gatherer{id_str, {curr_position_x, curr_position_y}, {end_position_x, end_position_y}, kDogWidth};
            provider.AddGatherer(gatherer);
        }

        for (const auto &loot : map_.GetLoots())
        {
            collision_detector::Item item{loot.id_, {loot.position_x_, loot.position_y_}, kItemWidth};
            provider.AddItem(item);
        }

        for (const auto &office : map_.GetOffices())
        {
            auto curr_position_x = static_cast<double>(office.GetPosition().x);
            auto curr_position_y = static_cast<double>(office.GetPosition().y);
            collision_detector::Item item{kOfficeID, {curr_position_x, curr_position_y}, 0.5};
            provider.AddItem(item);
        }

        auto collected_loot = collision_detector::FindGatherEvents(provider);
        if (collected_loot.empty())
        {
            return;
        }

        for (auto clltd_loot : collected_loot)
        {
            CollectLoot(provider, clltd_loot.item_id, clltd_loot.gatherer_id);
            DropLoot(provider, clltd_loot.item_id, clltd_loot.gatherer_id);
        }
    };

    void GameSession::CollectLoot(const collision_detector::Provider &provider, size_t item_id, std::string gatherer_id)
    {
        if (item_id != kOfficeID)
            return;
            
        auto dog = FindDog(util::Tagged<std::string, Dog>(gatherer_id));

        if (dog == nullptr)
            return;

        auto item = map_.GetLoot(item_id);

        if (item == nullptr)
            return;

        dog->AddItemToBag(std::move(*item));

        map_.DeleteItemFromMap(item_id);
    };

    void GameSession::DropLoot(const collision_detector::Provider &provider, size_t item_id, std::string gatherer_id)
    {
        if (item_id != kOfficeID)
            return;

        auto dog = FindDog(util::Tagged<std::string, Dog>(gatherer_id));

        if (dog == nullptr)
            return;

        dog->ClearBag();
    };

    const int32_t GameSession::GenerateNum(int32_t start, int32_t end)
    {
        if (start == end)
            return start;

        if (start > end)
            std::swap(start, end);

        boost::random::mt19937 gen;
        gen.seed(static_cast<unsigned int>(std::time(0)));
        boost::random::uniform_int_distribution<> dist(start, end);
        return dist(gen);
    }

    const Map *Game::FindMap(const Map::Id &id) const noexcept
    {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end())
        {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    void Map::DeleteMapLoot(const MapLoot &loot)
    {
        map_loot_.erase(std::find(map_loot_.begin(), map_loot_.end(), loot));
    }

    MapLoot *Map::GetLoot(size_t id)
    {
        for (auto &loot : map_loot_)
        {
            if (id == loot.id_)
                return &loot;
        }

        return nullptr;
    }

    void Map::DeleteItemFromMap(size_t item_id)
    {
        for (size_t i = 0; i < map_loot_.size(); ++i)
        {
            if (map_loot_.at(i).id_ == item_id)
            {
                map_loot_.erase(map_loot_.begin() + i);
            }
        }
    }

    void Map::AddOffice(const Office &office)
    {
        if (warehouse_id_to_index_.contains(office.GetId()))
        {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office &o = offices_.emplace_back(std::move(office));
        try
        {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        }
        catch (...)
        {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }

    std::shared_ptr<GameSession> Game::FindGameSession(const GameSession::Id &id) noexcept
    {
        if (auto it = game_session_id_to_index_.find(id); it != game_session_id_to_index_.end())
        {
            return game_sessions_.at(it->second);
        }
        return nullptr;
    }

    std::shared_ptr<GameSession> Game::ConnectToSession(const std::string &map_id, const std::string &user_name)
    {
        std::shared_ptr<GameSession> game_session_ = nullptr;
        std::shared_ptr<Dog> dog_ = nullptr;

        if (game_sessions_.size() == 0)
        {
            CreateNewSession(map_id);
        }

        game_session_ = FindGameSession(util::Tagged<std::string, GameSession>(map_id));

        if (game_session_ == nullptr)
        {
            game_session_ = CreateNewSession(map_id);
        }

        dog_ = std::make_shared<Dog>(util::Tagged<std::string, Dog>(user_name), std::make_shared<Map>(game_session_->GetMap()));

        // TO DO. Remake
        game_session_->AddDog(dog_, IsDefaultSpawn());

        return game_session_;
    }

    std::shared_ptr<model::GameSession> Game::CreateNewSession(const std::string &map_id)
    {
        const Map *data_map = FindMap(util::Tagged<std::string, Map>(map_id));
        auto game_session = std::make_shared<model::GameSession>(util::Tagged<std::string, GameSession>(map_id), std::move(*data_map));
        AddGameSession(game_session);

        return game_session;
    }

} // namespace model
