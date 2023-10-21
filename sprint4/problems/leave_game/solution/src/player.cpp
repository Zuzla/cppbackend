#include "player.h"
#include <iomanip>
#include <iostream>
#include <memory>

namespace app
{
    std::string PlayerTokens::GetToken()
    {
        std::ostringstream ss;
        ss << std::hex << generator1_() << generator2_();
        return ss.str();
    }

    const std::string Players::AddPlayer(const std::string &dog_id, const util::Tagged<std::string, model::GameSession> &game_session)
    {
        try
        {
            auto dog_id_tag_ = util::Tagged<std::string, model::Dog>(dog_id);
            std::string token;

            do
            {
                token.clear();
                token = std::make_unique<PlayerTokens>()->GetToken();
            } while (token.size() != 32);

            auto token_tag = util::Tagged<std::string, detail::TokenTag>(token);
            auto new_player_ = std::make_shared<Player>(++count_players_, std::move(dog_id_tag_), game_session, token_tag);

            auto pair = std::make_pair<Token, std::shared_ptr<Player>>(std::move(token_tag), std::move(new_player_));
            players_.insert(pair);

            return token;
        }
        catch (...)
        {
            DeletePlayer(util::Tagged<std::string, model::Dog>(dog_id));
            return std::string();
        }

        return std::string();
    }

    void Players::AddPlayer(std::shared_ptr<app::Player> player)
    {
        try
        {
            auto token = player->GetToken();
            auto pair = std::make_pair<Token, std::shared_ptr<Player>>(std::move(token), std::move(player));
            players_.insert(pair);
        }
        catch (...)
        {
            // DeletePlayer(std::move(player));
        }
    }

    std::shared_ptr<Player> Players::FindByToken(std::string &str) const
    {
        Token token = util::Tagged<std::string, detail::TokenTag>(str);
        return players_.at(token);
    }

    std::shared_ptr<Player> Players::FindByToken(Token &token) const
    {
        return players_.at(token);
    }

    const Token Players::FindTokenByPlayerId(uint64_t id) const
    {
        for (const auto &item : players_)
        {
            if (item.second.get()->GetId() == id)
                return item.first;
        }
    }

    const Token Players::FindTokenByName(const std::string &name) const
    {
        auto name_tag_ = util::Tagged<std::string, model::Dog>(name);
        for (const auto &item : players_)
        {
            if (item.second.get()->GetDogId() == name_tag_)
                return item.first;
        }
    }

    void Players::DeletePlayer(uint64_t id)
    {
        auto token = FindTokenByPlayerId(id);

        auto it = players_.find(token);
        if (it == players_.end())
            return;

        players_.erase(it);
    }

    void Players::DeletePlayer(DogId dog_)
    {
        auto token = FindTokenByName(*dog_);
        auto it = players_.find(token);
        if (it == players_.end())
            return;

        players_.erase(it);
    }
}