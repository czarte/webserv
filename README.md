# Webserv - HTTP/1.1 Web Server

A high-performance, non-blocking HTTP/1.1 web server implementation inspired by nginx, built as part of the 42 school curriculum. This project demonstrates advanced systems programming concepts including event-driven I/O, multiplexing, CGI execution, and HTTP protocol implementation.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [HTTP Features](#http-features)
- [CGI Support](#cgi-support)
- [Session Management](#session-management)
- [Project Structure](#project-structure)
- [Testing](#testing)
- [Examples](#examples)
- [Known Limitations](#known-limitations)
- [Development](#development)

## Overview

Webserv is a fully-featured HTTP/1.1 web server written in C++ that implements:
- **Non-blocking I/O** using `poll()` for event multiplexing
- **Master/Worker architecture** with file descriptor-based workers
- **Static file serving** with directory listing support
- **CGI execution** for dynamic content generation
- **Request body handling** with configurable size limits
- **Session management** with cookie support
- **Port-based virtual hosting**
- **Keep-alive connections** for improved performance

This project adheres to RFC 2616 (HTTP/1.1) specifications and provides a robust, scalable foundation for serving web content.

## Features

### Core Features
- ✅ Non-blocking I/O with `poll()`-based event loop
- ✅ Multiple server blocks (virtual hosts)
- ✅ Name-based virtual host selection via `Host` header
- ✅ Configurable listening ports and IP addresses
- ✅ Static file serving with MIME type detection
- ✅ Directory listing (autoindex)
- ✅ Custom error pages with template substitution
- ✅ Request body size limits
- ✅ Keep-alive connections with configurable timeouts

### HTTP Methods
- ✅ GET - Retrieve resources
- ✅ POST - Submit data
- ✅ PUT - Upload/update resources
- ✅ DELETE - Remove resources
- ✅ HEAD - Retrieve headers only

### Advanced Features
- ✅ CGI execution (Python scripts)
- ✅ Chunked transfer encoding
- ✅ File uploads to configured locations
- ✅ Session management with cookies
- ✅ Content negotiation (Accept-Language, Accept-Charset)
- ✅ Persistent connections
- ✅ Multiple error page customization
- ✅ Location-based routing with prefix matching
- ✅ URL redirections

## Architecture

Webserv uses a **master/worker reactor model**:

```
┌─────────────────────────────────────────────┐
│           Master Event Loop                 │
│         (Single poll() loop)                │
└─────────────────┬───────────────────────────┘
                  │
        ┌─────────┴──────────┐
        │                    │
        ▼                    ▼
┌───────────────┐    ┌───────────────┐
│ Listening FD  │    │  Client FD    │
│   Workers     │    │   Workers     │
└───────────────┘    └───────┬───────┘
                             │
                    ┌────────┴────────┐
                    │                 │
                    ▼                 ▼
            ┌──────────────┐  ┌──────────────┐
            │ CGI Input    │  │ CGI Output   │
            │ Pipe Worker  │  │ Pipe Worker  │
            └──────────────┘  └──────────────┘
```

### Key Design Principles

1. **Event-Driven**: All I/O operations are non-blocking and driven by `poll()` readiness events
2. **FD Workers**: Each file descriptor (socket or pipe) is treated as a worker with associated state
3. **State Machines**: Connection workers maintain state (READING/WRITING) and process requests incrementally
4. **No Blocking**: The master loop never waits for any worker - all operations return immediately
5. **CGI Integration**: CGI processes are spawned with pipes that become workers in the event loop

## Requirements

- **Compiler**: C++98 compliant compiler (g++, clang++)
- **Operating System**: Linux, macOS, or other UNIX-like systems
- **Build Tool**: GNU Make
- **Optional**: Python 3.x for CGI script execution

## Installation

### Clone and Build

```bash
# Clone the repository
git clone <repository-url>
cd webserv

# Build the project
make

# Clean build artifacts (optional)
make clean

# Full rebuild
make re

# Remove executable
make fclean
```

The compilation produces a `webserv` executable in the project root directory.

## Usage

### Basic Usage

```bash
# Start with default configuration
./webserv

# Start with custom configuration file
./webserv path/to/config.conf

# Start with provided example configurations
./webserv conf/default.conf
./webserv conf/demo.conf
```

### Command Line Options

```bash
./webserv [configuration_file]
```

- If no configuration file is specified, the server uses `conf/default.conf`
- Configuration file path can be relative or absolute

### Stopping the Server

Press `Ctrl+C` to gracefully stop the server. The server will:
1. Stop accepting new connections
2. Complete processing of active requests
3. Close all open connections
4. Clean up resources

## Configuration

Webserv uses nginx-style configuration files. The configuration format supports:
- Multiple server blocks
- Location-based routing
- Custom error pages
- CGI configuration
- Upload directories
- Access control

### Quick Configuration Example

```nginx
server {
    listen 8080;
    server_name localhost;
    host 127.0.0.1;
    root www/html;
    client_max_body_size 10M;
    index index.html;
    error_page 404 errors/404.html;

    location / {
        allow_methods GET POST;
        autoindex off;
    }

    location /uploads {
        allow_methods GET POST PUT DELETE;
        autoindex on;
        upload_path www/uploads;
    }

    location /cgi-bin {
        allow_methods GET POST;
        cgi_path /usr/bin/python3;
        cgi_ext .py;
    }
}
```

### Configuration Directives

#### Server Block Directives

| Directive | Description | Example |
|-----------|-------------|---------|
| `listen` | Port to listen on | `listen 8080;` |
| `server_name` | Server hostname | `server_name example.com;` |
| `host` | IP address to bind | `host 127.0.0.1;` |
| `root` | Document root directory | `root www/html;` |
| `index` | Default index file | `index index.html;` |
| `client_max_body_size` | Max request body size | `client_max_body_size 10M;` |
| `error_page` | Custom error page | `error_page 404 /404.html;` |
| `session` | Enable session support | `session on;` |

#### Location Block Directives

| Directive | Description | Example |
|-----------|-------------|---------|
| `allow_methods` | Allowed HTTP methods | `allow_methods GET POST;` |
| `autoindex` | Enable directory listing | `autoindex on;` |
| `root` | Override document root | `root /var/www;` |
| `index` | Override index file | `index home.html;` |
| `upload_path` | Upload directory | `upload_path uploads/;` |
| `cgi_path` | CGI interpreter path | `cgi_path /usr/bin/python3;` |
| `cgi_ext` | CGI file extensions | `cgi_ext .py .sh;` |
| `return` | Redirect to path | `return /new-path;` |

For complete configuration documentation, see [conf/README.md](conf/README.md).

## HTTP Features

### Supported HTTP/1.1 Features

- **Persistent Connections**: Keep-alive connections reduce latency
- **Chunked Transfer Encoding**: Supports chunked request bodies
- **Content Negotiation**: Accept-Language and Accept-Charset headers
- **Range Requests**: Partial content delivery (planned)
- **Virtual Hosting**: Name-based virtual hosts via Host header
- **Custom Error Pages**: Configurable error responses with template variables

### HTTP Status Codes

The server implements proper status codes including:
- **2xx Success**: 200 OK, 201 Created, 204 No Content, 226 IM Used
- **3xx Redirection**: 301 Moved Permanently, 302 Found, 304 Not Modified
- **4xx Client Error**: 400 Bad Request, 403 Forbidden, 404 Not Found, 405 Method Not Allowed, 411 Length Required, 413 Payload Too Large
- **5xx Server Error**: 500 Internal Server Error, 501 Not Implemented, 505 HTTP Version Not Supported

### Error Page Templates

Custom error pages support template substitution:
- `{{STATUS}}` - HTTP status code (e.g., 404)
- `{{REASON}}` - Status reason phrase (e.g., Not Found)
- `{{MESSAGE}}` - Detailed error message

Example error page:
```html
<!DOCTYPE html>
<html>
<head><title>{{STATUS}} {{REASON}}</title></head>
<body>
    <h1>{{STATUS}} - {{REASON}}</h1>
    <p>{{MESSAGE}}</p>
</body>
</html>
```

## CGI Support

### Overview

Webserv supports CGI (Common Gateway Interface) for executing dynamic scripts. Currently, Python scripts are fully supported.

### CGI Configuration

```nginx
location /cgi-bin {
    allow_methods GET POST;
    cgi_path /usr/bin/python3;
    cgi_ext .py;
    upload_path uploads/;
}
```

### CGI Environment Variables

The server sets standard CGI environment variables:
- `REQUEST_METHOD` - HTTP method (GET, POST, etc.)
- `QUERY_STRING` - URL query parameters
- `CONTENT_TYPE` - Request content type
- `CONTENT_LENGTH` - Request body length
- `SCRIPT_FILENAME` - Full path to the CGI script
- `PATH_INFO` - Additional path information
- `SERVER_NAME` - Server hostname
- `SERVER_PORT` - Server port
- `SERVER_PROTOCOL` - HTTP version

### CGI Example

```python
#!/usr/bin/env python3
import os

print("Content-Type: text/html\r")
print("\r")
print("<html><body>")
print("<h1>CGI Test</h1>")
print(f"<p>Request Method: {os.environ.get('REQUEST_METHOD')}</p>")
print(f"<p>Query String: {os.environ.get('QUERY_STRING')}</p>")
print("</body></html>")
```

### CGI Execution Model

1. Client sends request to CGI endpoint
2. Server forks a child process
3. CGI script executed with environment variables
4. Script output piped back through non-blocking pipes
5. Output sent to client when available
6. Process cleaned up after completion

## Session Management

### Overview

Webserv includes built-in session management with cookie support. Sessions are stored in-memory and can persist across requests.

### Enable Sessions

```nginx
server {
    listen 8080;
    session on;  # Enable session support
    # ... other directives
}
```

### Session Endpoint

Access `/session` to interact with the session system:
- **GET /session** - View current session data
- **POST /session** - Create or update session data

### Cookie Handling

Sessions use HTTP cookies:
- Cookie name: `SESSIONID`
- Automatically set via `Set-Cookie` header
- Sent by browser on subsequent requests

## Project Structure

```
webserv/
├── conf/               # Configuration files
│   ├── default.conf    # Default server configuration
│   ├── demo.conf       # Demo configuration
│   └── README.md       # Configuration documentation
├── includes/           # Header files
│   ├── cgi/           # CGI-related headers
│   ├── config/        # Configuration parser headers
│   ├── core/          # Core server headers
│   ├── http/          # HTTP protocol headers
│   ├── io/            # I/O handling headers
│   └── utils/         # Utility headers
├── srcs/              # Source files
│   ├── cgi/           # CGI implementation
│   ├── config/        # Configuration parser
│   ├── core/          # Server core (event loop, workers)
│   ├── http/          # HTTP request/response handling
│   ├── io/            # I/O operations
│   ├── utils/         # Utility functions
│   └── main.cpp       # Entry point
├── www/               # Web content
│   ├── site1/         # Example site 1
│   ├── site2/         # Example site 2
│   └── uploads/       # Upload directory
├── cgi-bin/           # CGI scripts
├── tests/             # Test files
├── Makefile           # Build configuration
└── README.md          # This file
```

## Testing

### Manual Testing

```bash
# Start the server
./webserv conf/default.conf

# In another terminal, test with curl
curl http://localhost:8080/
curl -X POST -d "data=test" http://localhost:8080/
curl -X PUT --upload-file test.txt http://localhost:8080/uploads/test.txt
curl -X DELETE http://localhost:8080/uploads/test.txt
```

### Automated Testing

```bash
# Run the provided tester (if available)
./tester

# Custom tests
cd own_tester
# Follow tester-specific instructions
```

### Browser Testing

Open your browser and navigate to:
- `http://localhost:8080/` - Main page
- `http://localhost:8080/uploads/` - Upload directory (with autoindex)
- `http://localhost:8080/cgi-bin/test.py` - CGI script execution
- `http://localhost:8080/session` - Session management

## Examples

### Example 1: Static File Server

```nginx
server {
    listen 8080;
    server_name localhost;
    root www/static;
    index index.html;

    location / {
        allow_methods GET;
        autoindex off;
    }

    location /files {
        autoindex on;
    }
}
```

### Example 2: Upload Server

```nginx
server {
    listen 8080;
    server_name uploads.local;
    root www/uploads;
    client_max_body_size 50M;

    location / {
        allow_methods GET POST PUT DELETE;
        autoindex on;
        upload_path www/uploads;
    }
}
```

### Example 3: API with CGI

```nginx
server {
    listen 8081;
    server_name api.local;
    root www/api;

    location /api {
        allow_methods GET POST PUT DELETE;
        cgi_path /usr/bin/python3;
        cgi_ext .py;
    }
}
```

### Example 4: Multiple Virtual Hosts

```nginx
# Site 1
server {
    listen 8080;
    server_name site1.local;
    root www/site1;
    index index.html;

    location / {
        allow_methods GET;
    }
}

# Site 2
server {
    listen 8080;
    server_name site2.local;
    root www/site2;
    index index.html;

    location / {
        allow_methods GET POST;
    }
}
```

Access with:
```bash
curl -H "Host: site1.local" http://localhost:8080/
curl -H "Host: site2.local" http://localhost:8080/
```

## Known Limitations

### Current Limitations

- **CGI Interpreters**: Currently supports Python only (easy to extend to other interpreters)
- **Virtual Hosts**: Name-based virtual hosting is supported; IP-based is not implemented
- **File I/O**: Disk file operations are synchronous (allowed by project requirements)
- **SSL/TLS**: HTTPS is not supported
- **HTTP/2**: Only HTTP/1.1 is implemented

### By Design (Per 42 Subject)

- Single-threaded event loop (no multi-threading)
- No external web server libraries
- Synchronous file I/O acceptable for static files
- Focus on correctness over maximum performance

## Development

### Building in Debug Mode

```bash
# Build with debug symbols
make

# For additional debugging, edit Makefile to add -g flag
```

### Code Style

- C++98 standard compliance
- Orthodox Canonical Form for classes
- RAII for resource management
- No memory leaks (validated with valgrind)

### Adding New Features

1. **New CGI Interpreter**: Modify `cgi_path` configuration and CGI execution logic
2. **New HTTP Method**: Extend request parser and add handler in location processing
3. **New Configuration Directive**: Add to config parser and corresponding handler

### Documentation

- [HTTP_HEADER.md](HTTP_HEADER.md) - HTTP/1.1 header handling reference
- [CHANGELOG.md](CHANGELOG.md) - Version history and changes
- [conf/README.md](conf/README.md) - Complete configuration reference
- [plan.md](plan.md) - Development milestone plan

## Contributors

This project was developed as part of the 42 school curriculum.

## License

This project is part of 42 school curriculum and follows their educational guidelines.

---

**Note**: This server is designed for educational purposes and demonstrates core web server concepts. For production use, consider established servers like nginx, Apache, or modern alternatives.