#include "htmldecode.h"

#include <vector>
#include <map>
#include <string>
#include <iostream>

std::string HtmlDecode(std::string_view str)
{
    if (str.empty())
        return std::string();

    std::map<std::vector<std::string>, std::string> mnemonics{
        {{"&lt", "&LT"}, "<"},
        {{"&gt", "&GT"}, ">"},
        {{"&amp", "&AMP"}, "&"},
        {{"&apos", "&APOS"}, "\'"},
        {{"&quot", "&QUOT"}, "\""},
    };

    std::string res{str.begin(), str.end()};

    size_t pos = res.find("&", 0);
    while (pos != std::string::npos)
    {
        for (const auto &item : mnemonics)
        {
            for (const auto &sub_item : item.first)
            {
                size_t new_pos = res.find(sub_item, pos);

                if (new_pos == std::string::npos)
                    continue;

                if ((res.size() >= pos + sub_item.size() + 1) && (res.at(pos + sub_item.size()) == ';'))
                    res.erase(pos + sub_item.size(), 1);

                res.replace(new_pos, sub_item.size(), item.second);
            };
        }

        pos = res.find("&", pos+1);
    }

    //std::cout << str << "\t-\t" << res << std::endl;
    return {res.begin(), res.end()};
}
