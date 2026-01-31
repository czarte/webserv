#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"

printf "[02] pipelined two requests\n"

if ! command -v nc >/dev/null 2>&1; then
  echo "SKIP: nc not installed"
  exit 0
fi

req="GET / HTTP/1.1\r\nHost: example.com\r\n\r\nGET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n"
resp=$(printf "%b" "$req" | nc -w 2 "$HOST" "$PORT" || true)
count=$(printf "%s" "$resp" | grep -c "HTTP/1.1 200" || true)
[ "$count" -eq 2 ]
