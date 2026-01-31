#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"

printf "[02] partial header should not get response\n"

if ! command -v nc >/dev/null 2>&1; then
  echo "SKIP: nc not installed"
  exit 0
fi

req="GET / HTTP/1.1\r\nHost: example.com\r\n"
resp=$(printf "%b" "$req" | nc -w 1 "$HOST" "$PORT" || true)

# Expect no response until header completes with \r\n\r\n
[ -z "$resp" ]
