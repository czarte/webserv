# Webserv — Milestone Plan (Master / FD-Workers Architecture)

This project is built around a **master/worker reactor model**:
- the **master** is a single, poll-driven event loop,
- **workers** are file descriptors (fds) with associated state,
- all progress is driven by I/O readiness, never by blocking calls.

Each milestone extends worker logic without changing the master architecture.

---

## Milestone 1 — Master Loop & FD Workers (Foundation)

**Goal**
Establish a correct, non-blocking core where **each file descriptor is a worker** scheduled by a single `poll()` loop.

**What is implemented**
- One master event loop driven by `poll()`
- Non-blocking listening sockets
- Non-blocking client sockets
- One worker per fd:
  - listening fd → accept worker
  - client fd → connection worker
- Per-connection state machine (READING / WRITING)
- Strict rule: no socket or pipe I/O without poll readiness

**What this milestone proves**
- The server is fully event-driven
- No blocking behavior is possible
- The scheduling logic is correct and future-proof

**Status**: mandatory foundation

---

## Milestone 2 — HTTP Request Framing (Client Worker Logic)

**Goal**
Turn client fd workers from raw byte handlers into **HTTP-aware workers**.

**What is added**
- Incremental HTTP parsing:
  - request line
  - headers
- Detection of request completeness (`\r\n\r\n`)
- Basic HTTP error handling (400, 405, etc.)

**What does NOT change**
- Master loop
- FD-based worker model
- Poll-driven scheduling

**Key idea**
> Worker abstraction stays the same; only its internal logic evolves.

---

## Milestone 3 — HTTP Responses & Keep-Alive

**Goal**
Allow a single client fd worker to process **multiple requests over time**.

**What is added**
- Proper HTTP responses with headers and body
- Keep-alive support
- Request state reset on the same connection
- Idle and phase timeouts

**Key idea**
> A worker represents a connection, not a single request.

---

## Milestone 4 — Routing & Static Content Workers

**Goal**
Serve static resources and directories using routing rules.

**What is added**
- Configuration-based routing:
  - root
  - index
  - autoindex
- Static file serving
- Path traversal protection

**Important note**
- Disk file I/O is synchronous (allowed by subject)
- Socket I/O remains non-blocking and poll-driven

---

## Milestone 5 — Request Bodies, Uploads & DELETE, POST, GET

**Goal**
Extend client workers to handle HTTP bodies safely.

**What is added**
- `Content-Length` body handling
- `client_max_body_size` enforcement
- File uploads to configured locations
- DELETE method support

**Key idea**
> Body handling is a phase of the client worker, not a new execution model.

---

## Milestone 6 — CGI Workers (Process-Backed Workers)

**Goal**
Introduce **external workers (CGI)** without changing the master architecture.

**What is added**
- CGI execution via `fork()` + `execve()`
- Pipes to CGI stdin/stdout
- CGI pipes treated as **fd workers**
- Integration into the same `poll()` loop
- Dechunking for chunked requests before CGI
- EOF-based CGI input/output handling

**Worker types now present**
- Client socket fd → network worker
- CGI stdin pipe fd → CGI input worker
- CGI stdout pipe fd → CGI output worker

**Key idea**
> CGI introduces new worker types, not a new architecture.

---

## Milestone 7 — Timeouts, Cleanup & Resilience

**Goal**
Ensure the server remains responsive under all conditions.

**What is added**
- Timeouts for:
  - header read
  - body read
  - keep-alive idle
  - CGI execution
- CGI termination on timeout
- Proper cleanup of fds and child processes (`waitpid`)
- Stress-test stability

**Core rule**
> The master must never wait for a worker.

---

## Bonus Milestone — Cookies, Sessions & Multiple CGI Types

**Goal**
Add higher-level features without modifying the execution model.

**Possible additions**
- Cookie parsing and `Set-Cookie`
- Simple session management
- Multiple CGI interpreters (e.g. Python, PHP)

**Constraint**
- Bonus is evaluated only if all mandatory milestones are correct.

---

## Architectural Summary

- **Master**: single `poll()`-driven event loop
- **Workers**: file descriptors with state
- **Scheduling**: readiness-based, non-blocking
- **CGI**: external process workers integrated via pipes

This structure ensures correctness, scalability, and defendability during evaluation.


Server = Master
  vec<Worker> workers

Worker = Server
  vec<Connection = Client> connections
  vec<Config> configs

Connection
  client_fd
  buf_in
  buf_out
  vec<Request>

Config
  vec<Location> locations