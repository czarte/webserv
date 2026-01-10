#include "io/NonBlocking.hpp"
#include <fcntl.h>
#include <cerrno>

int makeNonBlocking(int fd)
{
	// Validate file descriptor
	if (fd < 0)
	{
		errno = EBADF;
		return -1;
	}

	// Get current flags
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		// Error getting flags, errno is already set by fcntl
		return -1;
	}

	// Set non-blocking flag
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1)
	{
		// Error setting flags, errno is already set by fcntl
		return -1;
	}

	return 0;  // Success
}
