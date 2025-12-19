#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}/"

echo "[01] curl basic"
echo "Test: curl request returns HTTP/1.1 200"
echo "Expected: status line contains 'HTTP/1.1 200'"

if ! command -v curl >/dev/null 2>&1; then
  echo "SKIP: curl not installed"
  exit 0
fi

headers=$(curl -sS -D - -o /dev/null "$URL" || true)
echo "$headers" | head -n 1 | grep -q "HTTP/1.1 200"

