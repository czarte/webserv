# Own Tester (Cookies & Sessions)

This tester validates the cookie/session bonus via a simple built-in endpoint:

- `GET /session` creates a session and returns `Set-Cookie: session_id=...`
- `GET /session?value=hello` stores a value in the session
- `GET /session` returns the same session and value

## Run

Start the server in another terminal (any config, e.g. on port 8080):

```
./webserv conf/test.conf
```

Then run:

```
python3 /home/antoine/webser_06/own_tester/main.py
```

Exit code is non-zero if a test fails.
