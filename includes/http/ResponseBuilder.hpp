#ifndef RESPONSE_BUILDER_HPP
#define RESPONSE_BUILDER_HPP

#include <string>
#include "core/Client.hpp"
#include "config/Location.hpp"

std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type);
std::string buildErrorResponse(int status, bool keep_alive, std::string message);
std::string contentTypeForPath(const std::string &path);
bool determineCgiRequest(Client &conn, const Location *loc, const std::string &uri);
std::string getFileExtension(const std::string &path);
std::string buildCgiResponse(const std::string &cgi_headers, const std::string &body,
							 bool keep_alive);

#endif // RESPONSE_BUILDER_HPP
