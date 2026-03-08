#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 BUILD_DIR OUTPUT_BIN" >&2
  exit 1
fi

BUILD_DIR=$1
OUTPUT_BIN=$2
mkdir -p "$BUILD_DIR"

cc -std=c11 -Wall -Wextra -O2 \
  guest/suspendctl.c \
  -o "$OUTPUT_BIN"
