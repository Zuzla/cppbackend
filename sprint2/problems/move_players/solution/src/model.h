#pragma once
#include <boost/random.hpp>

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <memory>

#include "tagged.h"

namespace model 
{

    using namespace std::literals;
    using Dimension = int;
    using Coord = Dimension;

    struct Point {
        Coord x, y;
    };

    struct Size {
        Dimension width, height;
    };

    struct Rectangle {
        Point position;
        Size size;
    };

    struct Position {
        float x ,y;
    };

    struct Speed {
        float x = 0 , y = 0;
    };

    enum class Direction
    {
        NORTH,
        SOUTH,
        WEST,
        EAST
    };

    struct Offset {
        Dimension dx, dy;
    };

    class Road {
        struct HorizontalTag {
            explicit HorizontalTag() = default;
        };

        struct VerticalTag {
            explicit VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{start}
            , end_{end_x, start.y} {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{start}
            , end_{start.x, end_y} {
        }

        bool IsHorizontal() const noexcept {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept {
            return start_;
        }

        Point GetEnd() const noexcept {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building {
    public:
        explicit Building(Rectangle bounds) noexcept
            : bounds_{bounds} {
        }

        const Rectangle& GetBounds() const noexcept {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}
            , position_{position}
            , offset_{offset} {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        Point GetPosition() const noexcept {
            return position_;
        }

        Offset GetOffset() const noexcept {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name) noexcept
            : id_(std::move(id))
            , name_(std::move(name)) {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        const std::string& GetName() const noexcept {
            return name_;
        }

        const Buildings& GetBuildings() const noexcept {
            return buildings_;
        }

        const Roads& GetRoads() const noexcept {
            return roads_;
        }

        const Offices& GetOffices() const noexcept {
            return offices_;
        }

        void AddRoad(const Road& road) {
            roads_.emplace_back(road);
        }

        void SetDogSpeed(const uint32_t& speed)
        {
            dog_speed_ = speed;
        }
        
        const unsigned int GetDogSpeed() const
        {
            return dog_speed_;
        }

        void AddBuilding(const Building& building) {
            buildings_.emplace_back(building);
        }

        void AddOffice(Office office);

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;

        uint32_t dog_speed_;
    };

    class Dog
    {
    public:
        using Id = util::Tagged<std::string, Dog>;

        Dog(Id id, unsigned int map_speed) noexcept
            : id_(std::move(id)), direction_{"U"}, map_speed_(map_speed)
            {        }

        const Id& GetId() const noexcept {
            return id_;
        }

        const Position& GetPosition() const noexcept {
            return position_;
        }

        const Speed& GetSpeed() const noexcept {
            return speed_;
        }

        const std::string& GetDirection() const noexcept {
            return direction_;
        }

        void SetPosition(const float& x, const float& y);

        void SetDirection(const std::string& direction)
        {
            direction_ = direction;

            if (direction_ == "L")
            {
                SetSpeed(-map_speed_, 0);
            }
            else if (direction_ == "R")
            {
                SetSpeed(map_speed_, 0);
            }
            else if (direction_ == "U")
            {
                SetSpeed(0, -map_speed_);
            }
            else if (direction_ == "D")
            {
                SetSpeed(0, map_speed_);
            }
            else if (direction_ == "")
            {
                SetSpeed(0, 0);
            }
        }

    private:
        Id id_;
        Position position_;
        Speed speed_;
        std::string direction_;
        const unsigned int map_speed_;

        void SetSpeed(const float& x, const float& y);
    };

    class GameSession
    {
    public:
        using Id = util::Tagged<std::string, GameSession>;
        using DogId = util::Tagged<std::string, Dog>;
        using Dogs = std::vector<Dog>;

        GameSession(Id id, const Map&& map) noexcept
            : id_(std::move(id)), map_(std::move(map)) {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        const Dogs& GetDogs() const noexcept {
            return dogs_;
        }

        const Map& GetMap() const noexcept {
            return map_;
        }

        void AddDog(Dog dog);

        Dog* FindDog(const Dog::Id& id) const noexcept;

    private:

        using DogIdHasher = util::TaggedHasher<Dog::Id>;
        using DogIdToIndex = std::unordered_map<Dog::Id, size_t, DogIdHasher>;

        std::vector<Dog> dogs_;
        DogIdToIndex dog_id_to_index_;

        const Id id_;
        const Map map_;

        const double GenerateNum(int start, int end);
        // TO DO. Remake
        void SetDefaultPositionDog(Dog&& dog);
    };

    class Game {
    public:
        using Maps = std::vector<Map>;
        using Session = std::vector<GameSession>;

        void AddMap(Map&& map);
        void AddGameSession(GameSession&& game_session_);

        const Maps& GetMaps() const noexcept {
            return maps_;
        }

        const Session& GetGameSessions() const noexcept {
            return game_sessions_;
        }

        const GameSession& ConnectToSession(const std::string& map_id, const std::string& user_name);
        const Map* FindMap(const Map::Id& id) const noexcept;
        GameSession* FindGameSession(const GameSession::Id& id) const noexcept ;

    private:
        GameSession& CreateNewSession(const std::string& map_id);

        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        using GameSessionIdHasher = util::TaggedHasher<GameSession::Id>;
        using GameSessionIdToIndex = std::unordered_map<GameSession::Id, size_t, GameSessionIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;

        std::vector<GameSession> game_sessions_;
        GameSessionIdToIndex game_session_id_to_index_;
    };

}  // namespace model