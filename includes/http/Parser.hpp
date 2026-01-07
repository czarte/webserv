#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "Request.hpp"
#include <string>

class Parser
{
public:
    enum Result
    {
        NEED_MORE,
        PARSED_OK,
        PARSE_ERROR
    };

    Parser();
    bool hasCompleteHeaders(const std::string &buf) const;
    Result parseOne(std::string &in_buf, Request &req, int &status, std::string &err) const;
private:
    int parseRequestLine(const std::string &line, Request &out) const;
};

#endif // HTTP_PARSER_HPP
