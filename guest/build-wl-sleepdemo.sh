#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 BUILD_DIR OUTPUT_BIN" >&2
  exit 1
fi

BUILD_DIR=$1
OUTPUT_BIN=$2
mkdir -p "$BUILD_DIR"

wayland-scanner client-header \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  "$BUILD_DIR/xdg-shell-client-protocol.h"

wayland-scanner private-code \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  "$BUILD_DIR/xdg-shell-protocol.c"

cc -std=c11 -Wall -Wextra -O2 \
  -I"$BUILD_DIR" \
  guest/wl_app_core.c \
  guest/wl_render.c \
  guest/wl_net.c \
  guest/wl_suspend.c \
  guest/wl_wayland.c \
  guest/wl_sleepdemo.c \
  "$BUILD_DIR/xdg-shell-protocol.c" \
  -o "$OUTPUT_BIN" \
  $(pkg-config --cflags --libs wayland-client)
