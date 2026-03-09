#!/bin/busybox sh
set -eu

URL=${1:-file:///root/phase4-smoke.html}
STATE_DIR=/var/lib/content-shell
PROFILE_DIR=$STATE_DIR/profile
HOME_DIR=$STATE_DIR/home
CACHE_DIR=$STATE_DIR/cache
CHROMIUM_DIR=/usr/lib/chromium-direct
CONTENT_SHELL_BIN=$CHROMIUM_DIR/content_shell
WINDOW_SIZE=${CONTENT_SHELL_WINDOW_SIZE:-1280,800}
FULLSCREEN=${CONTENT_SHELL_FULLSCREEN:-1}

mkdir -p "$PROFILE_DIR" "$HOME_DIR" "$CACHE_DIR"
chmod 700 "$STATE_DIR" "$PROFILE_DIR" "$HOME_DIR" "$CACHE_DIR" 2>/dev/null || true

if [ ! -e "$CONTENT_SHELL_BIN" ]; then
  echo "[content-shell-drm-launcher] missing binary: $CONTENT_SHELL_BIN" >&2
  ls -l "$CHROMIUM_DIR" >&2 || true
  exit 1
fi

echo "[content-shell-drm-launcher] launching $CONTENT_SHELL_BIN url=$URL" >&2
ls -l "$CONTENT_SHELL_BIN" "$CHROMIUM_DIR"/content_shell.pak "$CHROMIUM_DIR"/snapshot_blob.bin >&2 || true

export HOME=$HOME_DIR
export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/run/user/0}
export XDG_CACHE_HOME=$CACHE_DIR
export XKB_CONFIG_ROOT=${XKB_CONFIG_ROOT:-/usr/share/X11/xkb}
export FONTCONFIG_PATH=${FONTCONFIG_PATH:-/etc/fonts}
export FONTCONFIG_FILE=${FONTCONFIG_FILE:-/etc/fonts/fonts.conf}
export LIBGL_DRIVERS_PATH=${LIBGL_DRIVERS_PATH:-/usr/lib/x86_64-linux-gnu/dri}
export __EGL_VENDOR_LIBRARY_DIRS=${__EGL_VENDOR_LIBRARY_DIRS:-/usr/share/glvnd/egl_vendor.d}
export LD_LIBRARY_PATH=$CHROMIUM_DIR:/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu:/lib64

if [ -n "${CONTENT_SHELL_EGL_PLATFORM:-}" ]; then
  export EGL_PLATFORM=$CONTENT_SHELL_EGL_PLATFORM
fi

echo "[content-shell-drm-launcher] env EGL_PLATFORM=${EGL_PLATFORM:-<unset>} LD_LIBRARY_PATH=$LD_LIBRARY_PATH CONTENT_SHELL_VERBOSE=${CONTENT_SHELL_VERBOSE:-<unset>} CONTENT_SHELL_VMODULE=${CONTENT_SHELL_VMODULE:-<unset>} WINDOW_SIZE=$WINDOW_SIZE FULLSCREEN=$FULLSCREEN" >&2

set -- "$CONTENT_SHELL_BIN" \
  --enable-features=UseOzonePlatform \
  --ozone-platform=drm \
  --use-gl=angle \
  --use-angle=default \
  --no-sandbox \
  --user-data-dir="$PROFILE_DIR" \
  "--content-shell-host-window-size=${WINDOW_SIZE/,/x}" \
  --no-first-run \
  --no-default-browser-check \
  --password-store=basic \
  --enable-logging=stderr \
  --log-level=0

if [ "$FULLSCREEN" = "1" ]; then
  set -- "$@" --start-fullscreen
fi

if [ -n "${CONTENT_SHELL_VERBOSE:-}" ]; then
  set -- "$@" "--v=$CONTENT_SHELL_VERBOSE"
fi

if [ -n "${CONTENT_SHELL_VMODULE:-}" ]; then
  set -- "$@" "--vmodule=$CONTENT_SHELL_VMODULE"
fi

exec "$@" "$URL"
