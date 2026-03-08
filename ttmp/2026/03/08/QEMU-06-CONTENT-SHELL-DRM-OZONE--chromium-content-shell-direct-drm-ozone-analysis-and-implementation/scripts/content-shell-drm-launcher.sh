#!/bin/busybox sh
set -eu

URL=${1:-file:///root/phase4-smoke.html}
PROFILE_DIR=/var/lib/content-shell
CHROMIUM_DIR=/usr/lib/chromium-direct
CONTENT_SHELL_BIN=$CHROMIUM_DIR/content_shell

mkdir -p "$PROFILE_DIR"

if [ ! -x "$CONTENT_SHELL_BIN" ]; then
  echo "[content-shell-drm-launcher] missing binary: $CONTENT_SHELL_BIN" >&2
  exit 1
fi

export HOME=/root
export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/run/user/0}
export EGL_PLATFORM=${EGL_PLATFORM:-surfaceless}
export XKB_CONFIG_ROOT=${XKB_CONFIG_ROOT:-/usr/share/X11/xkb}
export FONTCONFIG_PATH=${FONTCONFIG_PATH:-/etc/fonts}
export LD_LIBRARY_PATH=$CHROMIUM_DIR:/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu:/lib64

exec "$CONTENT_SHELL_BIN" \
  --enable-features=UseOzonePlatform \
  --ozone-platform=drm \
  --use-gl=egl \
  --no-sandbox \
  --disable-dev-shm-usage \
  --user-data-dir="$PROFILE_DIR" \
  --no-first-run \
  --no-default-browser-check \
  --password-store=basic \
  --enable-logging=stderr \
  --log-level=0 \
  "$URL"
