#include "stringutils.hpp"

frg::vector<frg::string<frg_allocator>, frg_allocator> SplitString(const frg::string<frg_allocator> &str, char separator)
{
    frg::vector<frg::string<frg_allocator>, frg_allocator> outValues;

    int64_t lastIndex = -1;

    for(uint64_t i = 0; i < str.size(); i++)
    {
        if(str[i] == separator)
        {
            frg::string<frg_allocator> out;

            if(i != lastIndex)
            {
                for(uint64_t j = lastIndex + 1; j < i; j++)
                {
                    out += str[j];
                }
            }

            outValues.push_back(out);

            lastIndex = i;
        }
    }

    if(lastIndex < str.size())
    {
        frg::string<frg_allocator> out;

        for(uint64_t i = lastIndex + 1; i < str.size(); i++)
        {
            out += str[i];
        }

        outValues.push_back(out);
    }

    return outValues;
}
