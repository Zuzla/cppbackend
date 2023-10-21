#pragma once

#include <iostream>
#include <memory>

#include <boost/asio.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/fiber/future/promise.hpp>
#include <boost/serialization/vector.hpp>

#include "model.h"
#include "player.h"
#include "tagged.h"

namespace serialization
{
    namespace net = boost::asio;

    class PlayerSerialization
    {
    public:
        PlayerSerialization() = default;

        PlayerSerialization(PlayerSerialization &&other) = default;

        PlayerSerialization(const app::Player &player) : id_(player.GetId()),
                                                         dog_id_(*player.GetDogId()),
                                                         game_session_id_(*player.GetGameSessionId()),
                                                         token_{*player.GetToken()} {};

        app::Player Restore() const
        {
            return app::Player(id_,
                               util::Tagged<std::string, model::Dog>(dog_id_),
                               util::Tagged<std::string, model::GameSession>(game_session_id_),
                               util::Tagged<std::string, detail::TokenTag>(token_));
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & id_;
            ar & dog_id_;
            ar & game_session_id_;
            ar & token_;
        }

    private:
        uint64_t id_{0};
        std::string dog_id_ = std::string();
        std::string game_session_id_ = std::string();
        std::string token_ = std::string();
    };

    class MapLootSerialization
    {
    public:
        MapLootSerialization() = default;
        MapLootSerialization(MapLootSerialization &&other) = default;

        MapLootSerialization(const model::MapLoot &map_loot) : id_(map_loot.id_), type_(map_loot.type_), value_(map_loot.value_),
                                                               position_x_(map_loot.position_x_), position_y_(map_loot.position_y_){};

        model::MapLoot Restore() const
        {
            return model::MapLoot(id_, type_, value_, position_x_, position_y_);
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & id_;
            ar & type_;
            ar & value_;
            ar & position_x_;
            ar & position_y_;
        }

    private:
        int id_;
        int type_;
        int value_;
        double position_x_;
        double position_y_;
    };

    class DogSerialization
    {
    public:
        DogSerialization() = default;
        DogSerialization(DogSerialization &&other) = default;
        DogSerialization(const model::Dog &dog) : id_(*dog.GetId()),
                                                  direction_(dog.GetDirection()),
                                                  position_(dog.GetPosition()),
                                                  score_(dog.GetScore())
        {
            loots_ = GetMapLootSerializedData(dog);
        };

        std::vector<MapLootSerialization> GetMapLootSerializedData(const model::Dog &dog);
        model::Dog Restore() const;

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & id_;
            ar & direction_;
            ar & position_.x;
            ar & position_.y;
            ar & loots_;
            ar & score_;
        }

    private:
        size_t score_ = 0;

        std::string id_;
        std::string direction_{"U"};
        std::vector<MapLootSerialization> loots_;

        model::Position position_{0, 0};
    };

    class GameSessionSerialization
    {
    public:
        GameSessionSerialization() = default;
        GameSessionSerialization(GameSessionSerialization &&other) = default;

        GameSessionSerialization(const model::GameSession &game_session) : id_(*game_session.GetId()),
                                                                           map_name_(*game_session.GetMap().GetId()),
                                                                           num_loot_(game_session.GetMaxNumLoot())
        {
            dogs_ = GetDogSerializedData(game_session);
            loots_ = GetMapLootSerializedData(game_session);
        };


        std::vector<MapLootSerialization> GetMapLootSerializedData(const model::GameSession &game_session);

        std::vector<DogSerialization> GetDogSerializedData(const model::GameSession &game_session);

        model::GameSession Restore(const model::Game &game) const;

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & id_;
            ar & dogs_;
            ar & map_name_;
            ar & num_loot_;
            ar & loots_;
        }

    private:
        size_t num_loot_{0};
        std::string id_;
        std::string map_name_;
        std::vector<DogSerialization> dogs_;
        std::vector<MapLootSerialization> loots_;
    };

    class GameSerialization : public std::enable_shared_from_this<GameSerialization>
    {
    public:
        GameSerialization() = default;
        GameSerialization(
            std::shared_ptr<model::Game> game,
            std::shared_ptr<app::Players> players) : game_{std::move(game)},
                                                     players_class_{std::move(players)}
        {
            game_sessions_ = GetGameSessionSerializedData();
            players_ = GetPlayerSerializedData();
        };

        std::vector<GameSessionSerialization> GetGameSessionSerializedData();

        std::vector<PlayerSerialization> GetPlayerSerializedData();

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & game_sessions_;
            ar & players_;
        }

        void Restore(model::Game&& new_game_, app::Players&& new_players_);

    private:
        std::shared_ptr<model::Game> game_ = nullptr;
        std::shared_ptr<app::Players> players_class_ = nullptr;

        std::vector<GameSessionSerialization> game_sessions_;
        std::vector<PlayerSerialization> players_;
    };

}