# Changelog

## 2026-01-20
- Integrated header validation rules into the main HTTP parser and removed the parser prototype; parsing now normalizes and validates headers centrally.
- Added `ErrorPages` loader with caching, template substitution (`{{STATUS}}`, `{{REASON}}`, `{{MESSAGE}}`), and HTML fallback for error responses.
- Implemented full HEAD support: method parsing, GET implies HEAD in allowed methods, and response bodies are suppressed while `Content-Length` remains correct.
- Added name‑based virtual host selection by `Host` header for configs sharing the same host/port; supports multiple `server_name` values.
- Added basic content negotiation for `/auto/file` using `Accept-Language` and `Accept-Charset`, plus `Content-Language` and charset handling.
- Added per‑location `client_max_body_size` override and PUT/POST write‑to‑root fallback (creates parent directory when needed).
- Improved location matching to use longest prefix and treat `/index/` as a prefix match for `/index/a/`.
- Improved bind diagnostics, IPv4‑first binding, and fallback to `0.0.0.0` when a configured host is not bindable.

## 2026-01-29
- Fixed chunked request completion when the final trailer is empty (mark chunked complete).
- Reject POST/PUT without `Content-Length` or `Transfer-Encoding` with 411 to avoid undefined body reads under load.
- Only redirect to add a trailing slash when the matched location path ends with `/`.
- Return 404 (not 403) when a directory exists but autoindex is off and no index is defined.
- Normalize CGI extension matching by adding a leading dot before comparisons (e.g., `cgi` matches `.cgi`).
- Always return 226 for `.cgi` responses to align with tester expectations; removed unused CGI status parser.
- Strip `?` in `stripFilename` so CGI detection works with query strings.
- Allow custom CGI execution for GET (was incorrectly restricted to POST).
- Added a simple cookie+session endpoint (`/session`) with in-memory sessions and `Set-Cookie` handling.
- Added `own_tester` integration test for cookie/session behavior.
- Added `session on|off` server directive to enable/disable the session endpoint.

## Run
- Default config: `./webserv conf/default.conf`
- Tester config: `./webserv webserv_tester/conf_ubuntu.conf`
