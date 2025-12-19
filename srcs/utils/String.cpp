#include "utils/String.hpp"
#include <cctype>

std::string strutil::trim(const std::string &s)
{
    std::string::size_type start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;
    std::string::size_type end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;
    return s.substr(start, end - start);
}
