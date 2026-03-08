#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF' >&2
usage: configure_phase4_chromium_build.sh [--src DIR] [--out DIR] [--write-only]

Writes a first-pass args.gn for the QEMU-06 direct DRM/Ozone experiment and,
unless --write-only is given, runs gn gen for that output directory.

Defaults:
  --src  $HOME/chromium/src
  --out  <src>/out/Phase4DRM

The generated configuration is intentionally minimal and content_shell-first.
EOF
  exit 1
}

SRC_DIR=${SRC_DIR:-"$HOME/chromium/src"}
OUT_DIR=
WRITE_ONLY=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --src)
      SRC_DIR=$2
      shift 2
      ;;
    --out)
      OUT_DIR=$2
      shift 2
      ;;
    --write-only)
      WRITE_ONLY=1
      shift
      ;;
    *)
      usage
      ;;
  esac
done

SRC_DIR=$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$SRC_DIR")
OUT_DIR=${OUT_DIR:-"$SRC_DIR/out/Phase4DRM"}
OUT_DIR=$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$OUT_DIR")

if [[ ! -f "$SRC_DIR/.gn" ]]; then
  echo "Chromium source tree missing .gn at $SRC_DIR" >&2
  exit 1
fi

mkdir -p "$OUT_DIR"

cat >"$OUT_DIR/args.gn" <<'EOF'
use_ozone = true
target_os = "chromeos"
ozone_auto_platforms = false
ozone_platform_drm = true
ozone_platform_headless = true
ozone_platform_wayland = false
ozone_platform_x11 = false
ozone_platform = "drm"
use_system_minigbm = false
use_xkbcommon = true
use_pulseaudio = false
is_debug = false
is_component_build = false
symbol_level = 0
EOF

cat <<EOF
Wrote $OUT_DIR/args.gn

Initial phase-4 targets to build:
  content_shell
  chrome_sandbox
  chrome_crashpad_handler
EOF

if [[ "$WRITE_ONLY" == "1" ]]; then
  exit 0
fi

export PATH="$HOME/depot_tools:$PATH"
GN_BIN=
if command -v gn >/dev/null 2>&1 && gn --version >/dev/null 2>&1; then
  GN_BIN=$(command -v gn)
elif [[ -x "$SRC_DIR/buildtools/linux64/gn" ]]; then
  GN_BIN="$SRC_DIR/buildtools/linux64/gn"
else
  echo "No usable gn found via depot_tools or $SRC_DIR/buildtools/linux64/gn" >&2
  exit 1
fi

(
  cd "$SRC_DIR"
  "$GN_BIN" gen "$OUT_DIR"
)
