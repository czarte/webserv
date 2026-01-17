#include "utils/Path.hpp"
#include "utils/Logger.hpp"
#include <iostream>

std::pair<std::string, std::string> stripQuery(const std::string &target)
{
	LOG_DBG << target;
    std::string::size_type q = target.find_first_of("?#");
    if (q == std::string::npos)
		return std::make_pair(target.substr(0, q), "");
	return std::make_pair(target.substr(0, q), target.substr(q + 1));
}

std::pair<std::string, std::string> stripFilename(const std::string &target)
{
	std::string::size_type q = target.find_first_of("&#");
	if (q == std::string::npos)
		return std::make_pair(target.substr(0, q), "");
	return std::make_pair(target.substr(0, q), target.substr(q));
}

std::string joinPath(const std::string &root, const std::string &path)
{
	LOG_DBG << root << " " << path;
    if (root.empty())
        return path;
    if (path.empty())
        return root;
    if (root[root.size() - 1] == '/' && path[0] == '/')
        return root.substr(0, root.size() - 1) + path;
    if (root[root.size() - 1] != '/' && path[0] != '/')
        return root + "/" + path;
    return root + path;
}

bool hasTraversal(const std::string &path)
{
    return path.find("..") != std::string::npos;
}
