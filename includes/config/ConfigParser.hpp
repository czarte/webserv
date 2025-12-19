#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "Config.hpp"

class ConfigParser
{
public:
    ConfigParser();
    Config parse(const char *path);
};

#endif
