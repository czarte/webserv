#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
TEST_DIR="$ROOT_DIR/tests/milestone3"

HOST="127.0.0.1"
PORT="8080"

printf "[run_all] webserv Milestone 3 test pack\n"
printf "[run_all] Expected: server already running on %s:%s\n\n" "$HOST" "$PORT"

fail=0
for t in \
  "$TEST_DIR/01_keep_alive_two_requests.sh" \
  "$TEST_DIR/02_pipelined_two_requests.sh"
do
  printf "==> %s\n" "$(basename "$t")"
  if sh "$t"; then
    printf "[PASS] %s\n" "$(basename "$t")"
  else
    printf "[FAIL] %s\n" "$(basename "$t")"
    fail=1
  fi
  printf "\n"
done

exit "$fail"
