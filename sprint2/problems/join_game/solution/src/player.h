#pragma once
#include "tagged.h"
#include "model.h"

#include <random>
#include <map>
#include <memory>

namespace detail 
{
struct TokenTag {};
}  // namespace detail


namespace app
{
    using Token = util::Tagged<std::string, detail::TokenTag>;
    using DogId = util::Tagged<std::string, model::Dog>;
    using GameSessionId = util::Tagged<std::string, model::GameSession>;

    class PlayerTokens 
    {
    public:
   
        std::string GetToken();
        
    private:
        std::random_device random_device_;
        std::mt19937_64 generator1_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
        std::mt19937_64 generator2_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
    }; 

    class Player
    {
    public:
        Player(uint64_t id, DogId dog, GameSessionId game_session) : 
            id_{id}, 
            dog_{dog},
            game_session_{game_session} 
        {        };

        const DogId& GetDogId() const noexcept {
            return dog_;
        }

        const GameSessionId& GetGameSessionId() const noexcept 
        {
            return game_session_;
        }

        const uint64_t& GetId() const
        {
            return id_;
        }

    private:
        uint64_t id_;
        DogId dog_;
        GameSessionId game_session_;
    };

    class Players
    {
    public:

        const std::string AddPlayer(const std::string& dog_id, const GameSessionId& game_session);

        void DeletePlayer(Player* player)
        {
            auto token = FindTokenByPlayerId(player->GetId());

            auto it = players_.find(token);

            if (it != players_.end())
                players_.erase(it);

            delete player;
        }

        void DeletePlayer(uint64_t id)
        {
            auto token = FindTokenByPlayerId(id);
            auto player = FindByToken(token);   

            auto it = players_.find(token);

            if (it != players_.end())
                players_.erase(it);

            delete player;
        }

        void DeletePlayer(DogId dog_)
        {
            auto token = FindTokenByName(*dog_);
            auto player = FindByToken(token);            
            auto it = players_.find(token);

            if (it != players_.end())
                players_.erase(it);

            delete player;
        }

        Player* FindByToken(const std::string& str) const;
        Player* FindByToken(const Token& token) const;

        const Token FindTokenByPlayerId(uint64_t id) const;

        const Token FindTokenByName(const std::string& name) const;

        const std::map<Token, std::shared_ptr<Player>>& GetPlayers() const noexcept
        {
            return players_;
        }

    private:
        uint64_t count_players_ = 0;
        std::map<Token, std::shared_ptr<Player>> players_;
        
    };     
}