#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
TEST_DIR="$ROOT_DIR/tests"

echo "[run_all] webserv Milestone 1 test pack"
echo "[run_all] Expected: server already running on 127.0.0.1:8080"
echo

fail=0
for t in \
  "$TEST_DIR/01_curl_basic.sh" \
  "$TEST_DIR/02_nc_partial.sh" \
  "$TEST_DIR/03_pipelined_two_requests.sh" \
  "$TEST_DIR/04_concurrency.sh" \
  "$TEST_DIR/05_disconnect_mid_request.sh" \
  "$TEST_DIR/06_poll_compliance_trace.sh"
do
  echo "==> $(basename "$t")"
  if sh "$t"; then
    echo "[PASS] $(basename "$t")"
  else
    echo "[FAIL] $(basename "$t")"
    fail=1
  fi
  echo
done

exit "$fail"
