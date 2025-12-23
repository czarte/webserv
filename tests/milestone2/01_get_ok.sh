#!/bin/sh
set -eu

HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}/"

printf "[01] GET returns 200\n"

if ! command -v curl >/dev/null 2>&1; then
  echo "SKIP: curl not installed"
  exit 0
fi

status=$(curl -sS -o /dev/null -w "%{http_code}" "$URL" || true)
[ "$status" = "200" ]
