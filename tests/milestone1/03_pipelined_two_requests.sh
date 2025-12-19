#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"
REQ_FILE="$(CDPATH= cd "$(dirname "$0")" && pwd)/requests/two_requests.txt"

echo "[03] pipelined two requests (Milestone 1 safety check)"
echo "Test: send two requests in a single TCP stream"
echo "Expected: server does not hang/crash; at least one 'HTTP/1.1 200' appears"
echo "Note: full pipelining correctness is a later milestone; this test is about robustness."

if ! command -v nc >/dev/null 2>&1; then
  echo "SKIP: nc not installed"
  exit 0
fi

out=$(cat "$REQ_FILE" | nc -w 2 "$HOST" "$PORT" || true)
echo "$out" | grep -q "HTTP/1.1 200"
