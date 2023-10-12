#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>

namespace collision_detector
{

    struct CollectionResult
    {
        bool IsCollected(double collect_radius) const
        {
            return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
        }

        // Квадрат расстояния до точки
        double sq_distance;
        // Доля пройденного отрезка
        double proj_ratio;
    };

    // Движемся из точки a в точку b и пытаемся подобрать точку c
    CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

    struct Item
    {
        int id;
        geom::Point2D position;
        double width;
    };

    struct Gatherer
    {
        std::string id;
        geom::Point2D start_pos;
        geom::Point2D end_pos;
        double width;
    };

    class ItemGathererProvider
    {
    protected:
        ~ItemGathererProvider() = default;

    public:
        virtual size_t ItemsCount() const = 0;
        virtual Item GetItem(size_t idx) const = 0;
        virtual size_t GatherersCount() const = 0;
        virtual Gatherer GetGatherer(size_t idx) const = 0;
    };

    struct GatheringEvent
    {
        int item_id;
        std::string gatherer_id;
        double sq_distance;
        double time;
    };

    class Provider : public ItemGathererProvider
    {
    public:
        size_t ItemsCount() const override
        {
            return items_.size();
        }

        Item GetItem(size_t idx) const override
        {
            return items_.at(idx);
        }

        size_t GatherersCount() const override
        {
            return gatherers_.size();
        }

        Gatherer GetGatherer(size_t idx) const override
        {
            return gatherers_.at(idx);
        }

        void AddGatherer(Gatherer &gatherer)
        {
            gatherers_.push_back(gatherer);
        }

        void AddItem(Item &item)
        {
            items_.push_back(item);
        }

    private:
        std::vector<Item> items_;
        std::vector<Gatherer> gatherers_;
    };

    std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider &provider);

} // namespace collision_detector