#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"

echo "[02] nc partial"
echo "Test: send partial bytes and still receive a response"
echo "Expected: server replies with 'HTTP/1.1 200' and connection closes (no hang)"

if ! command -v nc >/dev/null 2>&1; then
  echo "SKIP: nc not installed"
  exit 0
fi

# Send only a single byte. Milestone 1 replies on any readable data (no HTTP parsing yet).
out=$(printf 'G' | nc -w 2 "$HOST" "$PORT" || true)
echo "$out" | grep -q "HTTP/1.1 200"

