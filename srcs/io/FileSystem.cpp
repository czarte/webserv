#include "io/FileSystem.hpp"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

bool isDirectory(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool isFile(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISREG(st.st_mode);
}

std::string readFile(const std::string &path, bool &ok)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
        ok = false;
        return "";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    ok = true;
    return ss.str();
}

std::string buildAutoindex(const std::string &path, const std::string &uri)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return "";
    std::ostringstream out;
    out << "<html><head><title>Index of " << uri << "</title></head><body>";
    out << "<h1>Index of " << uri << "</h1><ul>";
    struct dirent *ent;
    while ((ent = readdir(dir)) != 0)
    {
        std::string name = ent->d_name;
        if (name == "." || name == "..")
            continue;
        out << "<li><a href=\"" << uri;
        if (!uri.empty() && uri[uri.size() - 1] != '/')
            out << "/";
        out << name << "\">" << name << "</a></li>";
    }
    out << "</ul></body></html>";
    closedir(dir);
    return out.str();
}
