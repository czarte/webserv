#ifndef PATH_UTILS_HPP
#define PATH_UTILS_HPP

#include <string>

std::pair<std::string, std::string> stripQuery(const std::string &target);
std::pair<std::string, std::string> stripFilename(const std::string &target);
std::string joinPath(const std::string &root, const std::string &path);
bool hasTraversal(const std::string &path);

#endif 
