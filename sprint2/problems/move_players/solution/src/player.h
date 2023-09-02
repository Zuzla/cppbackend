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
        Player(uint64_t id, DogId dog, std::shared_ptr<model::GameSession> game_session) : 
            id_{id}, 
            dog_{dog},
            game_session_{game_session} 
        {        };

        const DogId& GetDogId() const noexcept {
            return dog_;
        }

        const model::GameSession& GetGameSession() const noexcept 
        {
            return *game_session_.get();
        }

        const uint64_t& GetId() const
        {
            return id_;
        }

    private:
        uint64_t id_;
        DogId dog_;
        std::shared_ptr<model::GameSession> game_session_;
    };

    class Players
    {
    public:

        const std::string AddPlayer(const std::string& dog_id, std::shared_ptr<model::GameSession> game_session);

        const Player* FindByToken(const std::string& str) const;

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