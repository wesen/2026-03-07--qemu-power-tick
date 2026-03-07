#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 BUILD_DIR GUEST_BIN OUTPUT_INITRAMFS" >&2
  exit 1
fi

BUILD_DIR=$1
GUEST_BIN=$2
OUTPUT_INITRAMFS=$3
ROOTFS="$BUILD_DIR/rootfs"

rm -rf "$ROOTFS"
mkdir -p "$ROOTFS"/{bin,dev,etc,proc,root,run,sbin,sys,tmp,usr/bin,var/log}

cp /usr/bin/busybox "$ROOTFS/bin/busybox"
(
  cd "$ROOTFS/bin"
  ./busybox --install .
)

cp "$GUEST_BIN" "$ROOTFS/bin/sleepdemo"
cp guest/init "$ROOTFS/init"
chmod +x "$ROOTFS/init" "$ROOTFS/bin/sleepdemo"

(
  cd "$ROOTFS"
  find . -print0 | cpio --null -ov --format=newc | gzip -9 >"$OUTPUT_INITRAMFS"
)

echo "built $OUTPUT_INITRAMFS"
