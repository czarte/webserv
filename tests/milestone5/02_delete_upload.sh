#!/bin/sh
set -eu

. ./tests/milestone5/_common.sh

printf "[02] DELETE uploaded file\n"

if ! command -v curl >/dev/null 2>&1; then
  echo "SKIP: curl not installed"
  exit 0
fi

if ! has_method "DELETE"; then
  echo "SKIP: DELETE not allowed in upload location"
  exit 0
fi

mkdir -p "$UPLOAD_PATH"

name="m5_delete.txt"
url_path=$(join_url "$UPLOAD_LOC" "$name")
printf "to-delete" > "$UPLOAD_PATH/$name"

code=$(curl -s -o /tmp/m5_resp.txt -w "%{http_code}" \
  -X DELETE "http://$HOST:$PORT${url_path}")

[ "$code" -eq 200 ]
[ ! -f "$UPLOAD_PATH/$name" ]
