#include "serialization.h"

namespace serialization
{
    std::vector<MapLootSerialization> DogSerialization::GetMapLootSerializedData(const model::Dog &dog)
    {
        std::vector<MapLootSerialization> map_loot_ser;

        for (auto &loot : dog.GetBag())
        {
            map_loot_ser.push_back(std::move(MapLootSerialization(loot)));
        };

        return map_loot_ser;
    };

    model::Dog DogSerialization::Restore() const
    {
        model::Dog restore_dog{util::Tagged<std::string, model::Dog>(id_)};
        restore_dog.SetDirection(direction_, 0);
        restore_dog.SetPosition(position_.x, position_.y);

        for (auto &loot : loots_)
        {
            restore_dog.AddItemToBag(loot.Restore());
        }

        return restore_dog;
    }

    std::vector<MapLootSerialization> GameSessionSerialization::GetMapLootSerializedData(const model::GameSession &game_session)
    {
        std::vector<MapLootSerialization> map_loot_ser;

        for (auto &loot : game_session.GetMap().GetLoots())
        {
            map_loot_ser.push_back(std::move(MapLootSerialization(loot)));
        };

        return map_loot_ser;
    };

    std::vector<DogSerialization> GameSessionSerialization::GetDogSerializedData(const model::GameSession &game_session)
    {
        std::vector<DogSerialization> dog_ser;

        for (auto &dog : game_session.GetDogs())
        {
            dog_ser.push_back(std::move(DogSerialization(*dog.get())));
        };

        return dog_ser;
    };

    model::GameSession GameSessionSerialization::Restore(const model::Game &game) const
    {
        auto map_ = game.FindMap(util::Tagged<std::string, model::Map>(map_name_));

        for (auto &loot : loots_)
        {
            map_->AddLoot(loot.Restore());
        }

        model::GameSession game_session_(util::Tagged<std::string, model::GameSession>(id_), std::move(*map_));
        game_session_.SetMaxNumLoot(num_loot_);

        for (auto &dog : dogs_)
        {
            auto shared = std::make_shared<model::Dog>(dog.Restore());
            game_session_.AddDog(shared);
        }

        return game_session_;
    }

    std::vector<GameSessionSerialization> GameSerialization::GetGameSessionSerializedData()
    {
        std::vector<GameSessionSerialization> sessions_ser;

        for (auto &session_ptr : game_->GetGameSessions())
        {
            auto session_ser = GameSessionSerialization(*session_ptr.get());
            sessions_ser.push_back(std::move(session_ser));
        };

        return sessions_ser;
    };

    std::vector<PlayerSerialization> GameSerialization::GetPlayerSerializedData()
    {
        std::vector<PlayerSerialization> players_ser;

        for (auto &player : players_class_->GetPlayers())
        {
            players_ser.push_back(std::move(PlayerSerialization(*player.second.get())));
        };

        return players_ser;
    };

    void GameSerialization::Restore(model::Game &&new_game_, app::Players &&new_players_)
    {
        for (auto &game_session_ : game_sessions_)
        {
            auto session_restore = game_session_.Restore(new_game_);
            auto shared = std::make_shared<model::GameSession>(session_restore);
            new_game_.AddGameSession(shared);
        }

        for (auto &player_ : players_)
        {
            new_players_.AddPlayer(std::make_shared<app::Player>(player_.Restore()));
        }
    }

}