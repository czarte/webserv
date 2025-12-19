#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "Request.hpp"

class Parser
{
public:
    Parser();
    Request parse(const char *data, int len);
};

#endif // HTTP_PARSER_HPP
