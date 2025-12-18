#!/bin/sh
set -eu

echo "[06] poll compliance trace (optional)"
echo "Test: provide a manual tracing recipe showing that accept/recv/send are gated by poll readiness."
echo "Expected: if strace is missing, print 'skipped' and exit 0."
echo

if ! command -v strace >/dev/null 2>&1; then
  echo "skipped: strace not installed"
  exit 0
fi

echo "strace is available."
echo "Manual example (run in another terminal):"
echo
echo "  strace -f -tt -e trace=poll,accept,accept4,recv,recvfrom,send,sendto -s 0 -o /tmp/webserv_strace.txt ./webserv"
echo "  # then in another terminal:"
echo "  curl -s http://127.0.0.1:8080/ >/dev/null"
echo "  # inspect /tmp/webserv_strace.txt:"
echo "  # - poll(...) indicates readiness"
echo "  # - accept/recv/send only happen after poll returns"
echo
echo "This script does not force tracing automatically to avoid interfering with your running server."

