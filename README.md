# webserv (42) — Milestone 1

Milestone 1 goal: a minimal, correct, **non-blocking** server driven by a **single master `poll()` loop**, replying with a hardcoded HTTP response (no HTTP parsing yet).

## Design (Milestone 1)

- **Master = `poll()` loop scheduler**  
  `Server::run()` builds a `std::vector<pollfd>` each iteration and calls **exactly one `poll()`** per loop to discover which FDs are ready.

- **Workers = FDs (listen + clients)**  
  - Listening sockets: watched with `POLLIN`, `accept()` only when `POLLIN` is set.
  - Client sockets: state machine (`READING`/`WRITING`)
    - `READING`: watch `POLLIN`, `recv()` only when `POLLIN` is set.
    - `WRITING`: watch `POLLOUT`, `send()` only when `POLLOUT` is set.

## What Milestone 1 guarantees

- Non-blocking sockets (listening + clients).
- No network I/O unless `poll()` indicated readiness **in the current loop iteration**.
- No errno-based branching after `accept`/`recv`/`send`:
  only return values are used (`>0` progress, `==0` peer closed, `<0` close).
- Hardcoded response and close:

```
HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK
```

## Build & run

```
make
./webserv
```

Server listens on `127.0.0.1:8080`.

## Tests

Run the server in one terminal:

```
./webserv
```

Then in another terminal:

```
./tests/run_all.sh
```

See `tests/README.md` for what each test validates.
