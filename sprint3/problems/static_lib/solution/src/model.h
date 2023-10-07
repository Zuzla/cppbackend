#pragma once
#include <boost/random.hpp>
#include <boost/asio.hpp>

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <memory>

#include "tagged.h"
#include "loots.h"
#include "loot_generator.h"

namespace model
{
    using namespace std::literals;
    using Dimension = int;
    using Coord = Dimension;

    struct Point
    {
        Coord x, y;
    };

    struct Size
    {
        Dimension width, height;
    };

    struct Rectangle
    {
        Point position;
        Size size;
    };

    struct Position
    {
        double x = 0, y = 0;
    };

    struct Speed
    {
        float x = 0, y = 0;
    };

    enum class Direction
    {
        NORTH,
        SOUTH,
        WEST,
        EAST
    };

    struct Offset
    {
        Dimension dx, dy;
    };

    struct MapLoot
    {
    public:
        int id_;
        int type_;
        double position_x_;
        double position_y_;


        MapLoot(int id, int type, double position_x, double position_y) : id_(id), type_(type), position_x_(position_x), position_y_(position_y){};

        friend bool operator==(const MapLoot &left, const MapLoot &right)
        {
            return (left.id_ == right.id_ && left.type_ == right.type_ && left.position_x_ == right.position_x_ && left.position_y_ == right.position_y_);
        }

        friend bool operator!=(const MapLoot &left, const MapLoot &right)
        {
            return (left.id_ != right.id_ && left.type_ != right.type_ && left.position_x_ != right.position_x_ && left.position_y_ != right.position_y_);
        }
    };

    class Road
    {
        struct HorizontalTag
        {
            HorizontalTag() = default;
        };

        struct VerticalTag
        {
            VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{start}, end_{end_x, start.y}
        {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{start}, end_{start.x, end_y}
        {
        }

        bool IsHorizontal() const noexcept
        {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept
        {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept
        {
            return start_;
        }

        Point GetEnd() const noexcept
        {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building
    {
    public:
        explicit Building(Rectangle bounds) noexcept
            : bounds_{bounds}
        {
        }

        const Rectangle &GetBounds() const noexcept
        {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office
    {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}, position_{position}, offset_{offset}
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        Point GetPosition() const noexcept
        {
            return position_;
        }

        Offset GetOffset() const noexcept
        {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map
    {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;
        using Loots = std::vector<MapLoot>;

        Map(Id id, std::string name) noexcept
            : id_(std::move(id)), name_(std::move(name))
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        const std::string &GetName() const noexcept
        {
            return name_;
        }

        const Buildings &GetBuildings() const noexcept
        {
            return buildings_;
        }

        const Roads &GetRoads() const noexcept
        {
            return roads_;
        }

        const Offices &GetOffices() const noexcept
        {
            return offices_;
        }

        void AddRoad(const Road &road)
        {
            roads_.emplace_back(road);
        }

        void SetDogSpeed(const uint32_t &speed)
        {
            dog_speed_ = speed;
        }

        const unsigned int GetDogSpeed() const
        {
            return dog_speed_;
        }

        void AddBuilding(const Building &building)
        {
            buildings_.emplace_back(building);
        }

        void AddOffice(const Office &office);

        void AddLoot(const MapLoot& new_map_loot)
        {
            map_loot_.push_back(new_map_loot);
        }

        std::vector<MapLoot> GetLoots() const
        {
            return map_loot_;
        }

        void DeleteMapLoot(const MapLoot &loot)
        {
            map_loot_.erase(std::find(map_loot_.begin(), map_loot_.end(), loot));
        }

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;
        Loots map_loot_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;

        uint32_t dog_speed_ = 0;
    };

    class Dog
    {
    public:
        using Id = util::Tagged<std::string, Dog>;

        Dog(Id id, std::shared_ptr<Map> map) noexcept
            : id_(std::move(id)), direction_{"U"}, map_{map}
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        const Position &GetPosition() const noexcept
        {
            return position_;
        }

        const Speed &GetSpeed() const noexcept
        {
            return speed_;
        }

        const std::string &GetDirection() const noexcept
        {
            return direction_;
        }

        void Tick(std::chrono::milliseconds time);
        void UpdateHorizontal(const Road &&road, const double &x);
        void UpdateVertical(const Road &&road, const double &y);
        void SetDirection(const std::string &direction);
        void SetPosition(const double &x, const double &y);

    private:
        Id id_;
        Speed speed_;
        Position position_;
        std::shared_ptr<Map> map_;
        std::string direction_;

        const int32_t kMillisecondsToSeconds = 1000;
        const int32_t kRoadWidth = 0.4;

        const std::string kLeftDirection = "L";
        const std::string kRightDirection = "R";
        const std::string kDownDirection = "D";
        const std::string kUpDirection = "U";

        void SetSpeed(const int32_t &x, const int32_t &y);
    };

    class GameSession
    {
    public:
        using Id = util::Tagged<std::string, GameSession>;
        using DogId = util::Tagged<std::string, Dog>;
        using Dogs = std::vector<Dog>;

        GameSession(Id id, const Map &&map) noexcept
            : id_(std::move(id)), map_(std::move(map))
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        const Dogs &GetDogs() const noexcept
        {
            return dogs_;
        }

        const Map &GetMap() const noexcept
        {
            return map_;
        }

        void AddDog(Dog &&dog, const bool default_spawn);

        void DeleteDog(const Dog::Id &id)
        {
        }

        void LootGenerator(add_data::GameLoots &game_loots, std::chrono::milliseconds delta)
        {
            auto current_map_loot = map_.GetLoots();
            auto all_loot = game_loots.GetLoot(map_.GetName());

            auto count_ = game_loots.GetGenerator()->Generate(delta, current_map_loot.size(), dogs_.size());

            for (size_t i = 0; i < count_; ++i)
            {
                auto roads = map_.GetRoads();
                auto road = roads.at(GenerateNum(1, roads.size()));
                auto x = GenerateNum(road.GetStart().x, road.GetEnd().x);
                auto y = GenerateNum(road.GetStart().y, road.GetEnd().y);
                auto type = GenerateNum(0, all_loot.size() - 1);

                MapLoot new_map_loot(current_map_loot.size(), type, x, y);
                map_.AddLoot(new_map_loot);
            }
        }

        Dog *FindDog(const Dog::Id &id) const noexcept;

        void Tick(std::chrono::milliseconds time)
        {
            for (auto &dog_ : dogs_)
            {
                dog_.Tick(time);
            }
        }

    private:
        using DogIdHasher = util::TaggedHasher<Dog::Id>;
        using DogIdToIndex = std::unordered_map<Dog::Id, size_t, DogIdHasher>;

        std::vector<Dog> dogs_;
        DogIdToIndex dog_id_to_index_;

        const Id id_;
        Map map_;

        const int32_t GenerateNum(int32_t start, int32_t end);
        void SetPositionDog(Dog &&dog, const bool default_spawn);
    };

    class Game
    {
    public:
        using Maps = std::vector<Map>;
        using Session = std::vector<GameSession>;

        Game(bool is_debug, bool default_spawn) : is_debug_(is_debug), default_spawn_(default_spawn)
        {
        }

        Game(Game &&) = default;
        Game(Game const &) = default;
        Game operator=(Game const &) = delete;
        Game operator=(Game &&) = delete;

        void AddMap(Map &&map);
        void AddGameSession(GameSession &&game_session_);

        const Map *FindMap(const Map::Id &id) const noexcept;

        void Tick(std::chrono::milliseconds time, add_data::GameLoots &game_loots);
        void DisconnectSession(GameSession *game_session_, Dog *dog_);

        GameSession *FindGameSession(const GameSession::Id &id) const noexcept;
        GameSession *ConnectToSession(const std::string &map_id, const std::string &user_name);
        GameSession *CreateNewSession(const std::string &map_id);

        const Maps &GetMaps() const noexcept
        {
            return maps_;
        }

        const Session &GetGameSessions() const noexcept
        {
            return game_sessions_;
        }

        const bool IsDebug() const noexcept
        {
            return is_debug_;
        }

        const bool IsDefaultSpawn() const noexcept
        {
            return default_spawn_;
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        using GameSessionIdHasher = util::TaggedHasher<GameSession::Id>;
        using GameSessionIdToIndex = std::unordered_map<GameSession::Id, size_t, GameSessionIdHasher>;

        Maps maps_;
        MapIdToIndex map_id_to_index_;

        Session game_sessions_;
        GameSessionIdToIndex game_session_id_to_index_;

        const bool is_debug_;
        const bool default_spawn_;
    };

} // namespace model
