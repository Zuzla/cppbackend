#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

void Dog::SetPosition(const float& x, const float& y) 
{
    position_.x = x;
    position_.y = y;
}

template <typename T>
bool IsInBounds(const T& value, T low, T high) 
{
    if (low > high)
        std::swap(low, high);

    return (value >= low) && (value <= high);
}

void Dog::UpdatePosition() 
{
    auto current_time = std::chrono::steady_clock::now();
    auto v = start_time_;
    auto time_diff_ = std::chrono::duration_cast<std::chrono::milliseconds>((current_time - start_time_)).count();

    float x = position_.x + speed_.x * time_diff_;
    float y = position_.y + speed_.y * time_diff_;

    /*
    TO DO.
    Чтобы быстро находить участок дороги по координатам, построить вспомогательную структуру данных, чтобы вместо 
    линейного перебора всех дорог на карте искать дороги внутри map, unordered_map или отсортированного vector.
    */
    auto roads_ = map_->GetRoads();
    const Road *current_road = nullptr;
    for (const auto& road: roads_)
    {
        auto check_x_ = IsInBounds<float>(std::floor(x), road.GetStart().x, road.GetEnd().x);
        auto check_y_ = IsInBounds<float>(std::floor(y), road.GetStart().y, road.GetEnd().y);

        if (!check_x_ || !check_y_)
            continue;

        // if more than 2 roads are found, then this is an intersection and we can move in any direction
        if (current_road != nullptr)
        {
            position_.x = x;
            position_.y = y;
            return;
        }
        
        current_road = &road;        
    }

    if (current_road == nullptr)
        return;

    if (current_road->IsHorizontal())
    {
        if (y < position_.y + 0.4 && y > position_.y - 0.4)
        {
            position_.x = x;
            position_.y = y;
            return;
        }
    }
    else
    {
        if (x < position_.x + 0.4 && x > position_.x - 0.4)
        {
            position_.x = x;
            position_.y = y;
            return;
        }
    }
}

void Dog::SetSpeed(const float& x, const float& y) 
{
    speed_.x = x;
    speed_.y = y;
}

void Dog::SetDirection(const std::string& direction)
{
    direction_ = direction;

    if (direction_ == "L")
    {
        SetSpeed(-map_->GetDogSpeed(), 0);
    }
    else if (direction_ == "R")
    {
        SetSpeed(map_->GetDogSpeed(), 0);
    }
    else if (direction_ == "U")
    {
        SetSpeed(0, -map_->GetDogSpeed());
    }
    else if (direction_ == "D")
    {
        SetSpeed(0, map_->GetDogSpeed());
    }
    else if (direction_ == "")
    {
        SetSpeed(0, 0);
    }

    // UpdatePosition();
}

void GameSession::AddDog(Dog&& dog)
{
    SetDefaultPositionDog(std::move(dog));
    
    const size_t index = dogs_.size();
    if (auto [it, inserted] = dog_id_to_index_.emplace(dog.GetId(), index); !inserted) {
        throw std::invalid_argument("Dog with id "s + *dog.GetId() + " already exists"s);
    } else {
        try 
        {
            dogs_.emplace_back(std::move(dog));
        } catch (...) {
            dog_id_to_index_.erase(it);
            throw;
        }
    }
}

Dog* GameSession::FindDog(const Dog::Id& id) const noexcept 
{
    if (auto it = dog_id_to_index_.find(id); it != dog_id_to_index_.end()) {
        return &const_cast<Dog&>(dogs_.at(it->second));
    }
    return nullptr;
}

const int GameSession::GenerateNum(int start, int end)
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

void GameSession::SetDefaultPositionDog(Dog&& dog)
{
    try
    {    
        //auto roads = map_.GetRoads();
        //auto road = roads.at(GenerateNum(1, roads.size()));
        //auto x = GenerateNum(road.GetStart().x, road.GetEnd().x);
        //auto y = GenerateNum(road.GetStart().y, road.GetEnd().y);
        auto x = 0;
        auto y = 0;
        
        dog.SetPosition(x, y);
    }
    catch(...)
    {
        dog.SetPosition(0, 0);
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept 
{
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::FindGameSession(const GameSession::Id& id) const noexcept 
{      
    if (auto it = game_session_id_to_index_.find(id); it != game_session_id_to_index_.end())
    {
        return &const_cast<GameSession&>(game_sessions_.at(it->second));
    }
    return nullptr;
}
        
void Map::AddOffice(Office&& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map&& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

void Game::AddGameSession(GameSession&& game_session_)
{
    const size_t index = game_sessions_.size();
    if (auto [it, inserted] = game_session_id_to_index_.emplace(game_session_.GetId(), index); !inserted) {
        throw std::invalid_argument("Game Session with id "s + *game_session_.GetId() + " already exists"s);
    } else {
        try {
            game_sessions_.emplace_back(std::move(game_session_));
        } catch (...) {
            game_session_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSession* Game::ConnectToSession(const std::string& map_id, const std::string& user_name)
{
    GameSession *game_session_ = nullptr;
    Dog *dog_ = nullptr;

    try
    {        
        if (game_sessions_.size() == 0)
        {
            CreateNewSession(map_id);
        }

        game_session_ = FindGameSession(util::Tagged<std::string, GameSession>(map_id));

        if (game_session_ == nullptr)
        {
            game_session_ = CreateNewSession(map_id);
        }

        dog_ = new Dog(util::Tagged<std::string, Dog>(user_name), std::make_shared<Map>(game_session_->GetMap()));
        
        // TO DO. Remake
        game_session_->AddDog(std::move(*dog_));

        return game_session_;
    }
    catch(...)
    {
        if (game_session_ == nullptr)
            return nullptr;

        if (dog_ == nullptr)
            return nullptr;

        DisconnectSession(game_session_, dog_);

        return nullptr;
    }
}

void Game::DisconnectSession(GameSession* game_session_, Dog *dog_)
{
    game_session_->DeleteDog(dog_->GetId());
    delete dog_;
}

GameSession* Game::CreateNewSession(const std::string& map_id)
{
    const Map *data_map = FindMap(util::Tagged<std::string, Map>(map_id));
    GameSession *game_session = new model::GameSession(util::Tagged<std::string, GameSession>(map_id), std::move(*data_map));
    AddGameSession(std::move(*game_session));

    return game_session;
}

}  // namespace model
