#include "stringutils.hpp"

vector<string> SplitString(const frg::string<frg_allocator> &str, char separator)
{
    vector<string> outValues;

    int64_t lastIndex = -1;

    for(uint64_t i = 0; i < str.size(); i++)
    {
        if(str[i] == separator)
        {
            string out;

            if(lastIndex < 0 || i != (uint64_t)lastIndex)
            {
                for(uint64_t j = (uint64_t)lastIndex + 1; j < i; j++)
                {
                    out += str[j];
                }
            }

            outValues.push_back(out);

            lastIndex = i;
        }
    }

    if(lastIndex >= 0 && (uint64_t)lastIndex < str.size())
    {
        string out;

        for(uint64_t i = (uint64_t)lastIndex + 1; i < str.size(); i++)
        {
            out += str[i];
        }

        outValues.push_back(out);
    }

    return outValues;
}
