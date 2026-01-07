#!/bin/sh
set -eu

. ./tests/milestone5/_common.sh

printf "[01] POST upload to upload_path\n"

if ! command -v curl >/dev/null 2>&1; then
  echo "SKIP: curl not installed"
  exit 0
fi

if ! has_method "POST"; then
  echo "SKIP: POST not allowed in upload location"
  exit 0
fi

mkdir -p "$UPLOAD_PATH"

name="m5_upload.txt"
url_path=$(join_url "$UPLOAD_LOC" "$name")
body_file="$(mktemp)"
cleanup() { rm -f "$body_file"; }
trap cleanup EXIT
printf "milestone5-upload" > "$body_file"

code=$(curl -s -o /tmp/m5_resp.txt -w "%{http_code}" \
  -X POST --data-binary @"$body_file" "http://$HOST:$PORT${url_path}")

[ "$code" -eq 201 ]
[ -f "$UPLOAD_PATH/$name" ]
diff -q "$body_file" "$UPLOAD_PATH/$name" >/dev/null
