#!/bin/sh
set -eu

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

./tests/milestone5/01_post_upload.sh
./tests/milestone5/02_delete_upload.sh
./tests/milestone5/03_body_too_large.sh
