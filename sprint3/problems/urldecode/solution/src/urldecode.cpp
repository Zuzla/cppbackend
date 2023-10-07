#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <vector>

std::string UrlDecode(std::string_view str) {
    
    std::string res;
    std::vector<char> character = {'!', '#', '$', '&', '\'', '(', ')', '*', '+', ',', '/', ':', ';', '=', '?', '@', '[', ']', '-', '_', '.', '~', ' '};

    for (size_t i = 0; i < str.size(); i++)
    {
        std::string::value_type c = str[i];

        if (std::find(character.begin(), character.end(), c) != character.end())
        {
            res.push_back(c);
            continue;
        }

        else if (isalnum(c)) 
        {
            res.push_back(c);
            continue;
        }

        else if (c == '+')
        {
            res.push_back(' ');
            continue;
        }

        else if (c == '%' && isxdigit(str[i + 1]) && isalnum(str[i + 2]))
        {
            int32_t num;
            sscanf(str.substr(i + 1, 2).data(), "%x", &num);
            
            if (num < 32 || num > 127)
                throw new std::invalid_argument("err");
            res += static_cast<char>(num);
            
            i += 2;
            continue;
        }
        else
        {
            throw new std::invalid_argument("err");
        }
    }

    return res;
}
