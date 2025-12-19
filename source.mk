
CORE_SRCS := \
	srcs/core/Server.cpp \
	srcs/core/EventLoop.cpp

HTTP_SRCS := \
	srcs/http/Parser.cpp \
	srcs/http/Request.cpp \
	srcs/http/Response.cpp

CONFIG_SRCS := \
	srcs/config/ConfigParser.cpp \
	srcs/config/Config.cpp \
	srcs/config/WebservMaster.cpp

IO_SRCS := \
	srcs/io/Socket.cpp \
	srcs/io/File.cpp \
	srcs/io/NonBlocking.cpp

CGI_SRCS := \
	srcs/cgi/CgiHandler.cpp \
	srcs/cgi/CgiProcess.cpp

UTIL_SRCS := \
	srcs/utils/Log.cpp \
	srcs/utils/String.cpp

SRCS := \
	srcs/main.cpp \
	$(CORE_SRCS) \
	$(HTTP_SRCS) \
	$(CONFIG_SRCS) \
	$(IO_SRCS) \
	$(CGI_SRCS) \
	$(UTIL_SRCS)
