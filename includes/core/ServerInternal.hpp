#ifndef SERVER_INTERNAL_HPP
#define SERVER_INTERNAL_HPP

#include "core/Connection.hpp"
#include "io/FileSystem.hpp"
#include "utils/Time.hpp"

#include <cctype>
#include <cstdlib>
#include <limits.h>
#include <sstream>
#include <string>
#include <vector>

namespace serverutil
{
    static const char *kDefaultConfigPath = "conf/default.conf";
    static const size_t kHeaderTimeoutMs = 5000;
    static const size_t kIdleTimeoutMs = 15000;

    inline std::string toLower(const std::string &s)
    {
        std::string out = s;
        for (size_t i = 0; i < out.size(); ++i)
            out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
        return out;
    }

    inline bool isMethodAllowed(const std::vector<std::string> &allowed, const std::string &method)
    {
        if (allowed.empty())
            return true;
        std::string want = toLower(method);
        for (size_t i = 0; i < allowed.size(); ++i)
        {
            if (toLower(allowed[i]) == want)
                return true;
        }
        return false;
    }

    inline std::string lastPathSegment(const std::string &path)
    {
        if (path.empty())
            return "";
        std::string::size_type end = path.size();
        if (end > 0 && path[end - 1] == '/')
            return "";
        std::string::size_type pos = path.find_last_of('/');
        if (pos == std::string::npos)
            return path;
        return path.substr(pos + 1);
    }

    inline std::string buildUploadName()
    {
        static size_t counter = 0;
        std::ostringstream oss;
        oss << "upload_" << now_ms() << "_" << counter++ << ".bin";
        return oss.str();
    }

    inline bool resolveRealPath(const std::string &path, std::string &out)
    {
        char buf[PATH_MAX];
        if (!realpath(path.c_str(), buf))
            return false;
        out.assign(buf);
        return true;
    }

    inline bool isPathUnder(const std::string &base, const std::string &path)
    {
        if (base.empty() || path.empty())
            return false;
        if (path.size() < base.size())
            return false;
        if (path.compare(0, base.size(), base) != 0)
            return false;
        if (path.size() == base.size())
            return true;
        return base[base.size() - 1] == '/' || path[base.size()] == '/';
    }

    inline bool isPathWithinRoot(const std::string &root, const std::string &path)
    {
        std::string root_real;
        if (!resolveRealPath(root, root_real))
            return false;

        std::string path_real;
        if (isDirectory(path) || isFile(path))
        {
            if (!resolveRealPath(path, path_real))
                return false;
        }
        else
        {
            std::string::size_type slash = path.find_last_of('/');
            std::string parent = (slash == std::string::npos) ? "." : path.substr(0, slash);
            if (parent.empty())
                parent = "/";
            if (!resolveRealPath(parent, path_real))
                return false;
        }
        return isPathUnder(root_real, path_real);
    }

    inline void resetRequest(Connection &conn)
    {
        conn.request = Request();
        conn.body_bytes_read = 0;
        conn.body_bytes_expected = 0;
    }
}

#endif
