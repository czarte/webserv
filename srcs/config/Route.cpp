#include "config/Route.hpp"

const Location *matchLocation(const Config &config, const std::string &target)
{
    const std::vector<Location> &locations = config.getLocations();
    const Location *best = 0;
    size_t best_len = 0;
    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &path = locations[i].getPath();
        if (path.empty())
            continue;
        if (target.compare(0, path.size(), path) == 0 && path.size() >= best_len)
        {
            best = &locations[i];
            best_len = path.size();
        }
    }
    return best;
}
