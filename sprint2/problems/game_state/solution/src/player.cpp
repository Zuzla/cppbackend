#include "player.h"
#include <iomanip>
#include <iostream>
#include <memory>

namespace app
{
    std::string PlayerTokens::GetToken()
    {
        std::ostringstream ss;
        while(ss.str().size() != 32)
        {
            ss.clear();
            ss << std::hex << generator1_() << generator2_();
        }
        return ss.str();
    }

    const std::string Players::AddPlayer(const std::string& dog_id, const util::Tagged<std::string, model::GameSession>& game_session)
    {
        try
        {            
            auto dog_id_tag_ = util::Tagged<std::string, model::Dog>(dog_id);

            auto token = std::make_unique<PlayerTokens>()->GetToken();
            //auto token = "8c1883ccf6c3ac6af1d9b47b567ec010";
            auto new_player_ = std::make_shared<Player>(++count_players_, std::move(dog_id_tag_), game_session);

            auto pair = std::make_pair<Token, std::shared_ptr<Player>>(util::Tagged<std::string, detail::TokenTag>(token), std::move(new_player_));
            players_.insert(pair);

            return token;
        }
        catch(...)
        {
            DeletePlayer(util::Tagged<std::string, model::Dog>(dog_id));
            return std::string();
        }

        return std::string();
    }

    Player* Players::FindByToken(const std::string& str) const
    {
        Token token = util::Tagged<std::string, detail::TokenTag>(str);
        try
        {
            return players_.at(token).get();
        }
        catch(...)
        {
            return nullptr;
        }        
    }

    Player* Players::FindByToken(const Token& token) const
    {
        try
        {
            return players_.at(token).get();
        }
        catch(...)
        {
            return nullptr;
        }        
    }

    const Token Players::FindTokenByPlayerId(uint64_t id) const
    {
        for (const auto& item : players_)
        {
            if (item.second.get()->GetId() == id)
                return item.first;
        }
    }

    const Token Players::FindTokenByName(const std::string& name) const
    {
        auto name_tag_ = util::Tagged<std::string, model::Dog>(name);
        for (const auto& item : players_)
        {
            if (item.second.get()->GetDogId() == name_tag_)
                return item.first;
        }  
    }
}