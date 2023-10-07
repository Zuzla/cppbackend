#include "loots.h"

namespace add_data
{

    std::vector<Loot> GameLoots::GetLoot(const std::string &map_name)
    {
        auto pos = loot_.find(map_name);
        if (pos == loot_.end())
        {
            return std::vector<Loot>();
        }
        else
        {
            return pos->second;
        }
    }
}