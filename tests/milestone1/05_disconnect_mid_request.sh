#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}/"

echo "[05] disconnect mid-request"
echo "Test: connect, send partial bytes, and close early"
echo "Expected: server does not crash and still answers new clients afterwards"

if ! command -v nc >/dev/null 2>&1; then
  echo "SKIP: nc not installed"
  exit 0
fi

# Send a partial request and close quickly.
printf 'GET / HTTP/1.1\r\nHost: x\r\n' | nc -w 1 "$HOST" "$PORT" >/dev/null 2>&1 || true

# Server must remain alive; verify it can still answer.
if command -v curl >/dev/null 2>&1; then
  headers=$(curl -sS -D - -o /dev/null "$URL" || true)
  echo "$headers" | head -n 1 | grep -q "HTTP/1.1 200"
else
  # Fallback: just ensure we can connect and read a response.
  out=$(printf 'X' | nc -w 2 "$HOST" "$PORT" || true)
  echo "$out" | grep -q "HTTP/1.1 200"
fi
