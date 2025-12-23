#ifndef RESPONSE_BUILDER_HPP
#define RESPONSE_BUILDER_HPP

#include <string>

std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type);
std::string buildErrorResponse(int status, bool keep_alive);
std::string contentTypeForPath(const std::string &path);

#endif // RESPONSE_BUILDER_HPP
