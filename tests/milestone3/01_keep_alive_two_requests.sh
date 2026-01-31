#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"

printf "[01] keep-alive two sequential requests on same connection\n"

if ! command -v nc >/dev/null 2>&1; then
  echo "SKIP: nc not installed"
  exit 0
fi

req="GET / HTTP/1.1\r\nHost: example.com\r\nConnection: keep-alive\r\n\r\n"
req2="GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n"
resp=$(printf "%b%b" "$req" "$req2" | nc -w 2 "$HOST" "$PORT" || true)
count=$(printf "%s" "$resp" | grep -c "HTTP/1.1 200" || true)
[ "$count" -eq 2 ]
