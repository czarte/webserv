# Webserv Configuration Format

This document describes the configuration file format for the webserv HTTP server.

## Overview

The configuration file uses a nginx-like syntax with hierarchical blocks and directives. Each configuration file can contain multiple `server` blocks, each defining a virtual host.

## General Syntax Rules

- Directives are case-sensitive
- Each directive ends with a semicolon (`;`) or is followed by a block (`{`)
- Comments start with `#` and continue to the end of the line
- Whitespace (spaces, tabs, newlines) is generally ignored except within quoted strings
- String values can be enclosed in double quotes (`"`) if they contain spaces

## Server Block

A server block defines a virtual host configuration.

```
server {
    # Server directives go here
}
```

### Server Directives

#### `listen`

Specifies the port and optionally the host address for the server.

```
listen <port>;
listen <host>:<port>;
```

**Examples:**
```
listen 8080;
listen 127.0.0.1:8080;
```

#### `server_name`

Sets the server name (hostname).

```
server_name <name>;
```

**Example:**
```
server_name example.com;
```

#### `host`

Sets the IP address to bind to.

```
host <ip_address>;
```

**Examples:**
```
host 127.0.0.1;
host 0.0.0.0;
```

#### `root`

Sets the root directory for serving files.

```
root <path>;
```

**Example:**
```
root /var/www/html;
root www/site1;
```

#### `client_max_body_size`

Sets the maximum allowed size of the client request body.

```
client_max_body_size <size>;
```

The size can be specified in bytes or with suffixes:
- `K` or `k` for kilobytes
- `M` or `m` for megabytes

**Examples:**
```
client_max_body_size 1M;
client_max_body_size 1024K;
client_max_body_size 1048576;
```

#### `index`

Sets the default index file.

```
index <filename>;
```

**Example:**
```
index index.html;
```

#### `error_page`

Maps an HTTP error code to a custom error page.

```
error_page <code> <path>;
```

**Example:**
```
error_page 404 /errors/404.html;
error_page 500 /errors/500.html;
```

## Location Block

Location blocks define configuration for specific URI paths within a server.

```
location <path> {
    # Location directives go here
}
```

**Example:**
```
location /api {
    allow_methods GET POST;
}
```

### Location Directives

#### `root`

Overrides the server root for this location.

```
root <path>;
```

**Example:**
```
location /static {
    root /var/www/static;
}
```

#### `index`

Overrides the default index file for this location.

```
index <filename>;
```

**Example:**
```
location /docs {
    index readme.html;
}
```

#### `autoindex`

Enables or disables directory listing.

```
autoindex <on|off>;
```

**Examples:**
```
autoindex on;
autoindex off;
```

#### `allow_methods` or `allowed_methods`

Specifies which HTTP methods are allowed for this location.

```
allow_methods <method1> [method2] [method3] ...;
```

**Examples:**
```
allow_methods GET;
allow_methods GET POST PUT DELETE;
```

Common methods: `GET`, `POST`, `PUT`, `DELETE`, `HEAD`

#### `return` or `redirect`

Sets up a redirect for this location.

```
return <target_path>;
redirect <target_url>;
```

**Examples:**
```
return /new-page;
redirect https://example.com;
```

#### `cgi_path` or `cgi`

Specifies the path(s) to CGI interpreters.

```
cgi_path <interpreter1> [interpreter2] ...;
```

**Examples:**
```
cgi_path /usr/bin/python3;
cgi_path /usr/bin/python3 /bin/bash;
```

#### `cgi_ext`

Specifies file extensions that should be processed as CGI scripts.

```
cgi_ext <ext1> [ext2] ...;
```

**Examples:**
```
cgi_ext .py;
cgi_ext .py .sh .pl;
```

#### `upload_path` or `upload`

Specifies the directory where uploaded files should be stored.

```
upload_path <path>;
```

**Example:**
```
upload_path /var/www/uploads;
```

## Complete Configuration Example

```
# Main website
server {
    listen 8080;
    server_name example.com;
    host 127.0.0.1;
    root www/html;
    client_max_body_size 10M;
    index index.html;
    error_page 404 /errors/404.html;

    location / {
        allow_methods GET POST;
        autoindex off;
    }

    location /uploads {
        allow_methods GET POST DELETE;
        autoindex on;
        upload_path ./uploads;
    }

    location /cgi-bin {
        root ./;
        allow_methods GET POST;
        cgi_path /usr/bin/python3;
        cgi_ext .py;
    }
}

# API server
server {
    listen 8081;
    server_name api.example.com;
    host 127.0.0.1;
    root www/api;
    client_max_body_size 5M;
    index index.html;

    location / {
        allow_methods GET POST PUT DELETE;
        autoindex off;
    }
}
```

## Notes

- All paths can be relative or absolute
- Relative paths are resolved from the working directory where webserv is executed
- If a directive is specified in both server and location blocks, the location directive takes precedence
- Unrecognized directives will cause a parse error
- Missing required values will cause a parse error
- The parser is strict about syntax - misplaced braces or semicolons will cause errors