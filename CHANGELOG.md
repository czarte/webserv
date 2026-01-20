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

## Run
- Default config: `./webserv conf/default.conf`
- Tester config: `./webserv webserv_tester/conf_ubuntu.conf`
