#include "http/Parser.hpp"

Parser::Parser()
{
}

Request Parser::parse(const char *data, int len)
{
    (void)data;
    (void)len;
    return Request();
}
