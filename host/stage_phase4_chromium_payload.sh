#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF' >&2
usage: stage_phase4_chromium_payload.sh [--src-out DIR] [--dest DIR] [--allow-missing]

Copies the first-pass Chromium phase-4 runtime payload out of a built Chromium
output directory into a repo-local staging directory suitable for
guest/build-phase4-rootfs.sh.

Defaults:
  --src-out  $HOME/chromium/src/out/Phase4DRM
  --dest     build/phase4/chromium-direct
EOF
  exit 1
}

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
SRC_OUT=${SRC_OUT:-"$HOME/chromium/src/out/Phase4DRM"}
DEST_DIR=${DEST_DIR:-"$REPO_ROOT/build/phase4/chromium-direct"}
ALLOW_MISSING=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --src-out)
      SRC_OUT=$2
      shift 2
      ;;
    --dest)
      DEST_DIR=$2
      shift 2
      ;;
    --allow-missing)
      ALLOW_MISSING=1
      shift
      ;;
    *)
      usage
      ;;
  esac
done

SRC_OUT=$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$SRC_OUT")
DEST_DIR=$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$DEST_DIR")

mkdir -p "$DEST_DIR"
mkdir -p "$DEST_DIR/locales"

required=(
  content_shell
  chrome_sandbox
  icudtl.dat
  content_shell.pak
  snapshot_blob.bin
)

optional=(
  chrome
  chrome_crashpad_handler
  nacl_helper
  resources.pak
  v8_context_snapshot.bin
  chrome_100_percent.pak
  chrome_200_percent.pak
  libminigbm.so
  libEGL.so
  libGLESv2.so
  libvk_swiftshader.so
  libvulkan.so.1
)

missing=()

copy_one() {
  local name=$1
  if [[ -e "$SRC_OUT/$name" ]]; then
    cp -a "$SRC_OUT/$name" "$DEST_DIR/"
  else
    missing+=("$name")
  fi
}

for name in "${required[@]}"; do
  copy_one "$name"
done

for name in "${optional[@]}"; do
  if [[ -e "$SRC_OUT/$name" ]]; then
    cp -a "$SRC_OUT/$name" "$DEST_DIR/"
  fi
done

if [[ -d "$SRC_OUT/locales" ]]; then
  cp -a "$SRC_OUT/locales/." "$DEST_DIR/locales/"
fi

if (( ${#missing[@]} > 0 )) && (( ALLOW_MISSING == 0 )); then
  printf 'missing required Chromium payload entries in %s:\n' "$SRC_OUT" >&2
  printf '  %s\n' "${missing[@]}" >&2
  exit 1
fi

probe_args=(--payload-dir "$DEST_DIR")
if (( ALLOW_MISSING == 1 )); then
  probe_args+=(--allow-missing-payload)
fi
python3 "$SCRIPT_DIR/probe_phase4_chromium_payload.py" "${probe_args[@]}"

echo "staged Chromium phase-4 payload into $DEST_DIR"
