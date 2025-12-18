#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}/"

echo "[04] concurrency"
echo "Test: many parallel clients"
echo "Expected: server stays alive and keeps answering 200"

if command -v ab >/dev/null 2>&1; then
  echo "Using ab (ApacheBench)"
  ab -n 200 -c 50 "$URL" >/dev/null
  exit 0
fi

if ! command -v curl >/dev/null 2>&1; then
  echo "SKIP: neither ab nor curl installed"
  exit 0
fi

fail=0
i=0
max=100
concurrency=25

echo "Using curl background loop (max=$max, concurrency=$concurrency)"

while [ "$i" -lt "$max" ]; do
  j=0
  pids=""
  while [ "$j" -lt "$concurrency" ] && [ "$i" -lt "$max" ]; do
    curl -sS -o /dev/null "$URL" &
    pids="$pids $!"
    i=$((i + 1))
    j=$((j + 1))
  done

  for pid in $pids; do
    if ! wait "$pid"; then
      fail=1
    fi
  done
done

# Final liveness check: must still answer HTTP/1.1 200.
headers=$(curl -sS -D - -o /dev/null "$URL" || true)
echo "$headers" | head -n 1 | grep -q "HTTP/1.1 200"

exit "$fail"
