#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 BUILD_DIR OUTPUT_INITRAMFS" >&2
  exit 1
fi

BUILD_DIR=$1
OUTPUT_INITRAMFS=$2
ROOTFS="$BUILD_DIR/rootfs-phase4"
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
OUTPUT_INITRAMFS=$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$OUTPUT_INITRAMFS")
KERNEL_RELEASE=${KERNEL_RELEASE:-$(uname -r)}
CHROMIUM_PAYLOAD_DIR=${CHROMIUM_PAYLOAD_DIR:-}

python3 -c 'import shutil,sys; shutil.rmtree(sys.argv[1], ignore_errors=True)' "$ROOTFS"
mkdir -p "$ROOTFS"
mkdir -p "$ROOTFS"/{bin,dev,etc/fonts,lib,lib64,proc,root,run,sbin,sys,tmp,usr/bin,usr/lib,usr/sbin,var/log,var/lib}
mkdir -p "$ROOTFS/usr/lib/x86_64-linux-gnu" "$ROOTFS/usr/lib/chromium-direct/locales" "$ROOTFS/usr/share/X11" "$ROOTFS/usr/share/fonts"
ln -s ../usr/lib/x86_64-linux-gnu "$ROOTFS/lib/x86_64-linux-gnu"

cp /usr/bin/busybox "$ROOTFS/bin/busybox"
(
  cd "$ROOTFS/bin"
  ./busybox --install .
)

cp "$SCRIPT_DIR/init-phase4-drm" "$ROOTFS/init"
cp "$SCRIPT_DIR/content-shell-drm-launcher.sh" "$ROOTFS/usr/bin/content-shell-drm-launcher"
cp "$SCRIPT_DIR/phase4-smoke.html" "$ROOTFS/root/phase4-smoke.html"
cp "$SCRIPT_DIR/build-kms-pattern.sh" "$ROOTFS/usr/bin/build-kms-pattern.sh"
"$SCRIPT_DIR/build-kms-pattern.sh" "$BUILD_DIR" "$BUILD_DIR/kms_pattern"
cp "$BUILD_DIR/kms_pattern" "$ROOTFS/usr/bin/kms_pattern"
chmod +x "$ROOTFS/init" "$ROOTFS/usr/bin/content-shell-drm-launcher" "$ROOTFS/usr/bin/kms_pattern" "$ROOTFS/usr/bin/build-kms-pattern.sh"

cp -a /etc/fonts "$ROOTFS/etc/fonts"
mkdir -p "$ROOTFS/usr/share/fonts/truetype"
cp -a /usr/share/fonts/truetype/dejavu "$ROOTFS/usr/share/fonts/truetype/" || true
cp -a /usr/share/X11/xkb "$ROOTFS/usr/share/X11/xkb"
cp /usr/bin/udevadm "$ROOTFS/usr/bin/udevadm"
mkdir -p "$ROOTFS/lib/systemd"
ln -sf ../../usr/bin/udevadm "$ROOTFS/lib/systemd/systemd-udevd"
mkdir -p "$ROOTFS/etc/udev" "$ROOTFS/usr/lib/udev"
cp /etc/udev/udev.conf "$ROOTFS/etc/udev/udev.conf"
cp -a /usr/lib/udev/rules.d "$ROOTFS/usr/lib/udev/"
cp /usr/lib/udev/hwdb.bin "$ROOTFS/usr/lib/udev/hwdb.bin"
cp /lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 "$ROOTFS/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"

cat >"$ROOTFS/etc/passwd" <<'EOF'
root:x:0:0:root:/root:/bin/sh
EOF

cat >"$ROOTFS/etc/group" <<'EOF'
root:x:0:
video:x:27:
input:x:105:
EOF

mkdir -p "$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/virtio"
mkdir -p "$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/gpu/drm/virtio"
mkdir -p "$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid"
mkdir -p "$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/usbhid"
mkdir -p "$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/input/mouse"
mkdir -p "$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/usb/host"

zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/virtio/virtio_dma_buf.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/virtio/virtio_dma_buf.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/gpu/drm/virtio/virtio-gpu.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/gpu/drm/virtio/virtio-gpu.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/hid.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/hid.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/hid-generic.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/hid-generic.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/usbhid/usbhid.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/usbhid/usbhid.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/usbhid/usbkbd.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/usbhid/usbkbd.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/usbhid/usbmouse.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/hid/usbhid/usbmouse.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/input/mouse/psmouse.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/input/mouse/psmouse.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/usb/host/xhci-pci-renesas.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/usb/host/xhci-pci-renesas.ko"
zstd -d -f -c "/lib/modules/$KERNEL_RELEASE/kernel/drivers/usb/host/xhci-pci.ko.zst" \
  >"$ROOTFS/lib/modules/$KERNEL_RELEASE/kernel/drivers/usb/host/xhci-pci.ko"

RUNTIME_DEP_ARGS=(
  --rootfs "$ROOTFS"
  --ld-library-path "$ROOTFS/usr/lib/chromium-direct:$ROOTFS/usr/lib/x86_64-linux-gnu:$ROOTFS/lib/x86_64-linux-gnu"
  --binary "$ROOTFS/usr/bin/udevadm"
  --binary "$ROOTFS/usr/bin/kms_pattern"
)

if [[ -n "$CHROMIUM_PAYLOAD_DIR" ]]; then
  for path in \
    content_shell \
    chrome \
    chrome_sandbox \
    chrome_crashpad_handler \
    nacl_helper \
    icudtl.dat \
    resources.pak \
    v8_context_snapshot.bin \
    chrome_100_percent.pak \
    chrome_200_percent.pak
  do
    if [[ -e "$CHROMIUM_PAYLOAD_DIR/$path" ]]; then
      cp -a "$CHROMIUM_PAYLOAD_DIR/$path" "$ROOTFS/usr/lib/chromium-direct/"
    fi
  done
  if [[ -d "$CHROMIUM_PAYLOAD_DIR/locales" ]]; then
    cp -a "$CHROMIUM_PAYLOAD_DIR/locales/." "$ROOTFS/usr/lib/chromium-direct/locales/"
  fi
  if [[ -x "$ROOTFS/usr/lib/chromium-direct/content_shell" ]]; then
    RUNTIME_DEP_ARGS+=(--binary "$ROOTFS/usr/lib/chromium-direct/content_shell")
  fi
  if [[ -x "$ROOTFS/usr/lib/chromium-direct/chrome" ]]; then
    RUNTIME_DEP_ARGS+=(--binary "$ROOTFS/usr/lib/chromium-direct/chrome")
  fi
  if [[ -x "$ROOTFS/usr/lib/chromium-direct/chrome_crashpad_handler" ]]; then
    RUNTIME_DEP_ARGS+=(--binary "$ROOTFS/usr/lib/chromium-direct/chrome_crashpad_handler")
  fi
else
  echo "warning: CHROMIUM_PAYLOAD_DIR is unset; building a kms-only phase-4 initramfs" >&2
fi

python3 "$REPO_ROOT/scripts/copy-runtime-deps.py" "${RUNTIME_DEP_ARGS[@]}"

(
  cd "$ROOTFS"
  find . -print0 | cpio --null -ov --format=newc | gzip -9 >"$OUTPUT_INITRAMFS"
)

echo "built $OUTPUT_INITRAMFS"
