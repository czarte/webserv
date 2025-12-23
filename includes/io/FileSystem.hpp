#ifndef FILE_SYSTEM_HPP
#define FILE_SYSTEM_HPP

#include <string>

bool isDirectory(const std::string &path);
bool isFile(const std::string &path);
std::string readFile(const std::string &path, bool &ok);
std::string buildAutoindex(const std::string &path, const std::string &uri);

#endif
