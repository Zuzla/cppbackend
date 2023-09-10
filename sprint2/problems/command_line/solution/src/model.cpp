#include "model.h"

#include <stdexcept>

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
                return;
            }

            p1 = road.IsVertical();
            p2 = direction_ == kUpDirection || direction_ == kDownDirection;

            if (p1 && p2)
            {
                UpdateVertical(std::move(road), y);
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

    void Game::AddGameSession(GameSession &&game_session_)
    {
        const size_t index = game_sessions_.size();
        if (auto [it, inserted] = game_session_id_to_index_.emplace(game_session_.GetId(), index); !inserted)
        {
            throw std::invalid_argument("Game Session with id "s + *game_session_.GetId() + " already exists"s);
        }
        else
        {
            try
            {
                game_sessions_.emplace_back(std::move(game_session_));
            }
            catch (...)
            {
                game_session_id_to_index_.erase(it);
                throw;
            }
        }
    }

    void Game::Tick(std::chrono::milliseconds time)
    {
        for (auto &game_session : game_sessions_)
        {
            game_session.Tick(time);
        }
    }

    void Game::DisconnectSession(GameSession *game_session_, Dog *dog_)
    {
        game_session_->DeleteDog(dog_->GetId());
        delete dog_;
    }

    void GameSession::SetPositionDog(Dog &&dog, const bool default_spawn)
    {
        if (default_spawn)
        {
            dog.SetPosition(0, 0);
        }
        else
        {
            auto roads = map_.GetRoads();
            auto road = roads.at(GenerateNum(1, roads.size()));
            auto x = GenerateNum(road.GetStart().x, road.GetEnd().x);
            auto y = GenerateNum(road.GetStart().y, road.GetEnd().y);

            dog.SetPosition(x, y);
        }
    }

    void GameSession::AddDog(Dog &&dog, const bool default_spawn)
    {
        SetPositionDog(std::move(dog), default_spawn);

        const size_t index = dogs_.size();
        if (auto [it, inserted] = dog_id_to_index_.emplace(dog.GetId(), index); !inserted)
        {
            delete &dog;
            throw std::invalid_argument("Dog with id "s + *dog.GetId() + " already exists"s);
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

    Dog *GameSession::FindDog(const Dog::Id &id) const noexcept
    {
        if (auto it = dog_id_to_index_.find(id); it != dog_id_to_index_.end())
        {
            return &const_cast<Dog &>(dogs_.at(it->second));
        }
        return nullptr;
    }

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

    GameSession *Game::FindGameSession(const GameSession::Id &id) const noexcept
    {
        if (auto it = game_session_id_to_index_.find(id); it != game_session_id_to_index_.end())
        {
            return &const_cast<GameSession &>(game_sessions_.at(it->second));
        }
        return nullptr;
    }

    GameSession *Game::ConnectToSession(const std::string &map_id, const std::string &user_name)
    {
        GameSession *game_session_ = nullptr;
        Dog *dog_ = nullptr;

        if (game_sessions_.size() == 0)
        {
            CreateNewSession(map_id);
        }

        game_session_ = FindGameSession(util::Tagged<std::string, GameSession>(map_id));

        if (game_session_ == nullptr)
        {
            game_session_ = CreateNewSession(map_id);
        }

        dog_ = std::make_shared<Dog>(util::Tagged<std::string, Dog>(user_name), std::make_shared<Map>(game_session_->GetMap())).get();

        // TO DO. Remake
        game_session_->AddDog(std::move(*dog_), IsDefaultSpawn());

        return game_session_;
    }

    GameSession *Game::CreateNewSession(const std::string &map_id)
    {
        const Map *data_map = FindMap(util::Tagged<std::string, Map>(map_id));
        GameSession *game_session = std::make_shared<model::GameSession>(util::Tagged<std::string, GameSession>(map_id), std::move(*data_map)).get();
        AddGameSession(std::move(*game_session));

        return game_session;
    }

} // namespace model
