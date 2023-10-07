#include "urlencode.h"

#include <vector>

std::string UrlEncode(std::string_view str) 
{
    std::string res;

    if (str.empty())
        return std::string();

    for(size_t i = 0; i < str.size(); i++)
    {
        std::string::value_type c = str[i];
        char bufHex[10];

        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') 
        {
            res.push_back(c);
            continue;
        }
        else {
            sprintf(bufHex,"%X",c);
            if((int)c < 16) 
                res += "%0"; 
            else
                res += "%";
            res += bufHex;
        }

    }

    return res;
}
