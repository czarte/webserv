
CORE_SRCS := \
	srcs/core/server/ServerCore.cpp \
	srcs/core/server/ServerPoll.cpp \
	srcs/core/Fd.cpp \
	srcs/core/Client.cpp \
	srcs/core/EventLoop.cpp \
	srcs/core/Worker.cpp

HTTP_SRCS := \
	srcs/http/ErrorPages.cpp \
	srcs/http/header_rules.cpp \
	srcs/http/Parser.cpp \
	srcs/http/Request.cpp \
	srcs/http/Response.cpp \
	srcs/http/ResponseBuilder.cpp

CONFIG_SRCS := \
	srcs/config/ConfigParser.cpp \
	srcs/config/Config.cpp \
	srcs/config/Location.cpp \
	srcs/config/Route.cpp \
	srcs/config/WebservMaster.cpp

IO_SRCS := \
	srcs/io/Socket.cpp \
	srcs/io/NonBlocking.cpp \
	srcs/io/FileSystem.cpp

CGI_SRCS := \
	srcs/cgi/CgiHandler.cpp \
	srcs/cgi/CgiProcess.cpp

UTIL_SRCS := \
	srcs/utils/Log.cpp \
	srcs/utils/Logger.cpp \
	srcs/utils/String.cpp \
	srcs/utils/Time.cpp \
	srcs/utils/Path.cpp

SRCS := \
	srcs/main.cpp \
	$(CORE_SRCS) \
	$(HTTP_SRCS) \
	$(CONFIG_SRCS) \
	$(IO_SRCS) \
	$(CGI_SRCS) \
	$(UTIL_SRCS)
