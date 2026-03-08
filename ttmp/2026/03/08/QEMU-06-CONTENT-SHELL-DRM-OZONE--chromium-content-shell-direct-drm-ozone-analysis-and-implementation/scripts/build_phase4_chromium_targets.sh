#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF' >&2
usage: build_phase4_chromium_targets.sh [--src DIR] [--out DIR] [--skip-hooks] [--skip-gn]

Runs the first direct DRM/Ozone Chromium build flow for QEMU-06:
  1. gclient runhooks
  2. configure_phase4_chromium_build.sh
  3. autoninja for the initial target set

Defaults:
  --src  $HOME/chromium/src
  --out  <src>/out/Phase4DRM

Initial targets:
  content_shell
  chrome_sandbox
  chrome_crashpad_handler
EOF
  exit 1
}

SRC_DIR=${SRC_DIR:-"$HOME/chromium/src"}
OUT_DIR=
SKIP_HOOKS=0
SKIP_GN=0

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
    --skip-hooks)
      SKIP_HOOKS=1
      shift
      ;;
    --skip-gn)
      SKIP_GN=1
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

export PATH="$HOME/depot_tools:$PATH"
if ! command -v gclient >/dev/null 2>&1; then
  echo "gclient not found in PATH" >&2
  exit 1
fi

if [[ "$SKIP_HOOKS" != "1" ]]; then
  (
    cd "$SRC_DIR"
    gclient runhooks
  )
fi

if [[ "$SKIP_GN" != "1" ]]; then
  "$(cd "$(dirname "$0")" && pwd)/configure_phase4_chromium_build.sh" --src "$SRC_DIR" --out "$OUT_DIR"
fi

if ! command -v autoninja >/dev/null 2>&1; then
  echo "autoninja not found in PATH" >&2
  exit 1
fi

(
  cd "$SRC_DIR"
  autoninja -C "$OUT_DIR" content_shell chrome_sandbox chrome_crashpad_handler
)
