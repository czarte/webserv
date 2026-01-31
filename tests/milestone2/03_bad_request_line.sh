#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"

printf "[03] bad request line returns 400\n"

if ! command -v nc >/dev/null 2>&1; then
  echo "SKIP: nc not installed"
  exit 0
fi

req="BADREQUEST\r\n\r\n"
resp=$(printf "%b" "$req" | nc -w 1 "$HOST" "$PORT" || true)

printf "%s" "$resp" | head -n 1 | grep -q "HTTP/1.1 400"
