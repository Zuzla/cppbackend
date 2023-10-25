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

    void GameLoots::AddLoot(const std::string &map_name, Loot &&new_loot)
    {
        auto it = loot_.find(map_name);
        if (it == loot_.end())
        {
            std::vector<Loot> vec;
            vec.push_back(new_loot);
            auto pair = std::pair<std::string, std::vector<Loot>>(map_name, vec);
            loot_.insert(pair);
        }
        else
        {
            it->second.push_back(new_loot);
        }
    }
}