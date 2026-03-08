#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 BUILD_DIR OUTPUT_BINARY" >&2
  exit 1
fi

BUILD_DIR=$1
OUTPUT_BINARY=$2
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

mkdir -p "$BUILD_DIR"
cc -O2 -Wall -Wextra -std=c11 \
  "$SCRIPT_DIR/kms_pattern.c" \
  -o "$OUTPUT_BINARY" \
  $(pkg-config --cflags --libs libdrm)

echo "built $OUTPUT_BINARY"
