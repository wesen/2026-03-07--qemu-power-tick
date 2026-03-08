#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 BUILD_DIR OUTPUT_INITRAMFS" >&2
  exit 1
fi

BUILD_DIR=$1
OUTPUT_INITRAMFS=$2
ROOTFS="$BUILD_DIR/rootfs-phase3"
PKG_CACHE="$BUILD_DIR/pkg-cache"
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
OUTPUT_INITRAMFS=$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$OUTPUT_INITRAMFS")
KERNEL_RELEASE=${KERNEL_RELEASE:-$(uname -r)}
CHROMIUM_SNAP=${CHROMIUM_SNAP:-/snap/chromium/current}

python3 -c 'import shutil,sys; shutil.rmtree(sys.argv[1], ignore_errors=True)' "$ROOTFS"
mkdir -p "$ROOTFS" "$PKG_CACHE"
mkdir -p "$ROOTFS"/{bin,dev,etc/xdg/weston,home/root,lib,lib64,proc,root,run,sbin,sys,tmp,usr/bin,usr/lib,usr/sbin,var/log,var/lib}
mkdir -p "$ROOTFS/usr/lib/x86_64-linux-gnu" "$ROOTFS/usr/lib/udev" "$ROOTFS/etc/udev"
mkdir -p "$ROOTFS/usr/share/libinput" "$ROOTFS/usr/share/X11" "$ROOTFS/usr/share/fonts"
ln -s ../usr/lib/x86_64-linux-gnu "$ROOTFS/lib/x86_64-linux-gnu"

cp /usr/bin/busybox "$ROOTFS/bin/busybox"
(
  cd "$ROOTFS/bin"
  ./busybox --install .
)

apt download -o=dir::cache::archives="$PKG_CACHE" weston libweston-13-0 libseat1 seatd >/dev/null
dpkg-deb -x "$PKG_CACHE"/weston_*_amd64.deb "$ROOTFS"
dpkg-deb -x "$PKG_CACHE"/libweston-13-0_*_amd64.deb "$ROOTFS"
dpkg-deb -x "$PKG_CACHE"/libseat1_*_amd64.deb "$ROOTFS"
dpkg-deb -x "$PKG_CACHE"/seatd_*_amd64.deb "$ROOTFS"

cp "$SCRIPT_DIR/init-phase3" "$ROOTFS/init"
cp "$SCRIPT_DIR/weston.ini" "$ROOTFS/etc/xdg/weston/weston.ini"
cp "$SCRIPT_DIR/chromium-wayland-launcher.sh" "$ROOTFS/usr/bin/chromium-wayland-launcher"
chmod +x "$ROOTFS/init" "$ROOTFS/usr/bin/chromium-wayland-launcher"

cp -a /usr/share/X11/xkb "$ROOTFS/usr/share/X11/xkb"
cp -a /etc/fonts "$ROOTFS/etc/fonts"
mkdir -p "$ROOTFS/usr/share/fonts/truetype"
cp -a /usr/share/fonts/truetype/dejavu "$ROOTFS/usr/share/fonts/truetype/" || true
cp /usr/bin/udevadm "$ROOTFS/usr/bin/udevadm"
ln -sf ../../usr/bin/udevadm "$ROOTFS/lib/systemd/systemd-udevd"
cp /etc/udev/udev.conf "$ROOTFS/etc/udev/udev.conf"
cp -a /usr/lib/udev/rules.d "$ROOTFS/usr/lib/udev/"
cp /usr/lib/udev/hwdb.bin "$ROOTFS/usr/lib/udev/hwdb.bin"
cp -a /usr/share/libinput/. "$ROOTFS/usr/share/libinput/"
cp /lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 "$ROOTFS/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"

mkdir -p "$ROOTFS/usr/lib/chromium-browser/locales"
for path in \
  chrome \
  chrome-sandbox \
  chrome_100_percent.pak \
  chrome_200_percent.pak \
  chrome_crashpad_handler \
  icudtl.dat \
  libEGL.so \
  libffmpeg.so \
  libGLESv2.so \
  libvk_swiftshader.so \
  libvulkan.so.1 \
  resources.pak \
  v8_context_snapshot.bin \
  vk_swiftshader_icd.json
do
  cp -a "$CHROMIUM_SNAP/usr/lib/chromium-browser/$path" "$ROOTFS/usr/lib/chromium-browser/"
done
cp -a "$CHROMIUM_SNAP/usr/lib/chromium-browser/locales"/en-US* "$ROOTFS/usr/lib/chromium-browser/locales/" || true
cp -a "$CHROMIUM_SNAP/usr/share/X11/XErrorDB" "$ROOTFS/usr/share/X11/XErrorDB"

cat >"$ROOTFS/etc/passwd" <<'EOF'
root:x:0:0:root:/root:/bin/sh
EOF

cat >"$ROOTFS/etc/group" <<'EOF'
root:x:0:
video:x:27:
input:x:105:
seat:x:106:
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

python3 "$REPO_ROOT/scripts/copy-runtime-deps.py" \
  --rootfs "$ROOTFS" \
  --ld-library-path "$ROOTFS/usr/lib/chromium-browser:$ROOTFS/usr/lib/x86_64-linux-gnu:$ROOTFS/usr/lib/x86_64-linux-gnu/weston" \
  --binary "$ROOTFS/usr/bin/weston" \
  --binary "$ROOTFS/usr/sbin/seatd" \
  --binary "$ROOTFS/usr/bin/udevadm" \
  --binary "$ROOTFS/usr/lib/x86_64-linux-gnu/libweston-13/drm-backend.so" \
  --binary "$ROOTFS/usr/lib/x86_64-linux-gnu/weston/kiosk-shell.so" \
  --binary "$ROOTFS/usr/lib/chromium-browser/chrome" \
  --binary "$ROOTFS/usr/lib/chromium-browser/chrome_crashpad_handler"

(
  cd "$ROOTFS"
  find . -print0 | cpio --null -ov --format=newc | gzip -9 >"$OUTPUT_INITRAMFS"
)

echo "built $OUTPUT_INITRAMFS"
