#!/usr/bin/env bash
# This script finds all files with extension given as first argument
# and processes them in parallel using the command given as second argument.
# Usage: ./parallel.sh <file_extension> <command>
# Example: ./parallel.sh .txt "wc -l"

set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <file_extension> <command>"
  exit 1
fi

FILE_EXT="$1"
COMMAND="$2"

detect_cores() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  elif command -v getconf >/dev/null 2>&1; then
    getconf _NPROCESSORS_ONLN
  elif [[ "$(uname -s)" == "Darwin" ]] && command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.ncpu
  else
    echo 1
  fi
}

PROC_COUNT="$(detect_cores)"
find . -type f -name "*${FILE_EXT}" -print0 | \
  xargs -0 -n 1 -P "${PROC_COUNT}" -I {} bash -c '
    cmd=$1
    file=$2
    eval "$cmd" "\"$file\""
  ' _ "${COMMAND}" {}
