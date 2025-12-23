#include "utils/Path.hpp"

std::string stripQuery(const std::string &target)
{
    std::string::size_type q = target.find_first_of("?#");
    if (q == std::string::npos)
        return target;
    return target.substr(0, q);
}

std::string joinPath(const std::string &root, const std::string &path)
{
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
