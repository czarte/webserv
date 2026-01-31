#include "core/Fd.hpp"

#include <unistd.h>

Fd::Fd() : _fd(-1)
{
}

Fd::Fd(int fd) : _fd(fd)
{
}

Fd::~Fd()
{
    if (_fd >= 0)
        close(_fd);
}

int Fd::get() const
{
    return _fd;
}

int Fd::release()
{
    int tmp = _fd;
    _fd = -1;
    return tmp;
}
