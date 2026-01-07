#!/bin/sh
set -eu

. ./tests/milestone5/_common.sh

printf "[03] 413 when body exceeds client_max_body_size\n"

if ! command -v curl >/dev/null 2>&1; then
  echo "SKIP: curl not installed"
  exit 0
fi

if ! has_method "POST"; then
  echo "SKIP: POST not allowed in upload location"
  exit 0
fi

size_line=$(awk '
  $1=="client_max_body_size" {print $2; exit}
' "$CONF")
size_line=${size_line%;} 

if [ -z "$size_line" ]; then
  echo "SKIP: client_max_body_size not set"
  exit 0
fi

limit_bytes=$(parse_size "$size_line")
if [ "$limit_bytes" -le 0 ]; then
  echo "SKIP: client_max_body_size is 0"
  exit 0
fi

mkdir -p "$UPLOAD_PATH"

name="m5_large.bin"
url_path=$(join_url "$UPLOAD_LOC" "$name")
tmp="$(mktemp)"
cleanup() { rm -f "$tmp"; }
trap cleanup EXIT

count=$((limit_bytes / 1024 + 1))
dd if=/dev/zero of="$tmp" bs=1024 count="$count" status=none

code=$(curl -s -o /tmp/m5_resp.txt -w "%{http_code}" \
  --max-time 5 -X POST --data-binary @"$tmp" "http://$HOST:$PORT${url_path}" || true)

[ "$code" -eq 413 ]
