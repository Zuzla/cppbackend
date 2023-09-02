#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

void Dog::SetPosition(const float& x, const float& y) 
{
    position_.x = x;
    position_.y = y;
}

void Dog::SetSpeed(const float& x, const float& y) 
{
    speed_.x = x;
    speed_.y = y;
}

void GameSession::AddDog(Dog dog)
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

const double GameSession::GenerateNum(int start, int end)
{
    if (start == end)
        return start;

    if (start > end)
        std::swap(start, end);

    boost::random::mt19937 gen;
    gen.seed(static_cast<unsigned int>(std::time(0)));
    boost::random::uniform_real_distribution<> dist(start, end);
    return dist(gen);
}

void GameSession::SetDefaultPositionDog(Dog&& dog)
{
    auto roads = map_.GetRoads();

    auto road = roads.at(std::round(GenerateNum(1, roads.size())));
    auto x = GenerateNum(road.GetStart().x, road.GetEnd().x);
    auto y = GenerateNum(road.GetStart().y, road.GetEnd().y);
    auto point_x = std::round(x * 100) / 100;            
    auto point_y = std::round(y * 100) / 100;
    
    dog.SetPosition(point_x, point_y);
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
        
void Map::AddOffice(Office office) {
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

const GameSession& Game::ConnectToSession(const std::string& map_id, const std::string& user_name)
{
    if (game_sessions_.size() == 0)
    {
        CreateNewSession(map_id);
    }

    GameSession *game_session_ = FindGameSession(util::Tagged<std::string, GameSession>(map_id));

    if (game_session_ == nullptr)
    {
        game_session_ = &CreateNewSession(map_id);
    }

    auto dog = new Dog(util::Tagged<std::string, Dog>(user_name), game_session_->GetMap().GetDogSpeed());
    
    // TO DO. Remake
    game_session_->AddDog(*dog);

    return *game_session_;
}

GameSession& Game::CreateNewSession(const std::string& map_id)
{
    const Map *data_map = FindMap(util::Tagged<std::string, Map>(map_id));
    GameSession *game_session = new model::GameSession(util::Tagged<std::string, GameSession>(map_id), std::move(*data_map));
    AddGameSession(std::move(*game_session));

    return *game_session;
}

}  // namespace model
