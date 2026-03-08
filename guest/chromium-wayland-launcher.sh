#!/bin/busybox sh
set -eu

URL=${1:-about:blank}
PROFILE_DIR=/var/lib/chromium

mkdir -p "$PROFILE_DIR"

exec /usr/lib/chromium-browser/chrome \
  --enable-features=UseOzonePlatform \
  --ozone-platform=wayland \
  --no-sandbox \
  --disable-dev-shm-usage \
  --disable-gpu \
  --user-data-dir="$PROFILE_DIR" \
  --no-first-run \
  --no-default-browser-check \
  --password-store=basic \
  --disable-crash-reporter \
  --kiosk \
  "$URL"
