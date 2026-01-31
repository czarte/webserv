#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
TEST_DIR="$ROOT_DIR/tests/milestone2"

HOST="127.0.0.1"
PORT="8080"

printf "[run_all] webserv Milestone 2 test pack\n"
printf "[run_all] Expected: server already running on %s:%s\n\n" "$HOST" "$PORT"

fail=0
for t in \
  "$TEST_DIR/01_get_ok.sh" \
  "$TEST_DIR/02_partial_header_no_response.sh" \
  "$TEST_DIR/03_bad_request_line.sh" \
  "$TEST_DIR/04_method_not_allowed.sh" \
  "$TEST_DIR/05_header_without_colon.sh"
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
