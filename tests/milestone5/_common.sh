#!/bin/sh
set -eu

CONF="${CONF:-conf/default.conf}"
HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-8080}"

if [ ! -f "$CONF" ]; then
  echo "SKIP: config not found: $CONF"
  exit 0
fi

read UPLOAD_LOC UPLOAD_PATH ALLOW_METHODS <<EOF
$(awk '
  BEGIN {loc=""; upload=""; allow=""}
  $1=="location" {loc=$2; upload=""; allow=""; next}
  ($1=="allow_methods" || $1=="allowed_methods") {
    for (i=2; i<=NF; i++) {
      m=$i; gsub(/;/, "", m);
      if (m != "") allow=allow " " m
    }
  }
  ($1=="upload_path" || $1=="upload") {upload=$2; gsub(/;/, "", upload)}
  $1=="}" {
    if (upload != "") {print loc, upload, substr(allow, 2); exit}
    loc=""; upload=""; allow=""
  }
' "$CONF")
EOF

if [ -z "${UPLOAD_LOC:-}" ] || [ -z "${UPLOAD_PATH:-}" ]; then
  echo "SKIP: no upload_path location in $CONF"
  exit 0
fi

has_method() {
  want="$1"
  for m in $ALLOW_METHODS; do
    if [ "$m" = "$want" ]; then
      return 0
    fi
  done
  return 1
}

parse_size() {
  val="$1"
  case "$val" in
    *[Mm])
      num=${val%[Mm]}
      echo $((num * 1024 * 1024))
      ;;
    *[Kk])
      num=${val%[Kk]}
      echo $((num * 1024))
      ;;
    *)
      echo "$val"
      ;;
  esac
}

join_url() {
  base="$1"
  name="$2"
  case "$base" in
    "/") echo "/$name" ;;
    */) echo "${base}${name}" ;;
    *) echo "${base}/${name}" ;;
  esac
}
