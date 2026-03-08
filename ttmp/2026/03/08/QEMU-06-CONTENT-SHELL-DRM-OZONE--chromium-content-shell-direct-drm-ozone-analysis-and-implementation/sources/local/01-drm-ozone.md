---
Title: Imported DRM Ozone Research Note
Ticket: QEMU-06-CONTENT-SHELL-DRM-OZONE
Status: active
Topics:
    - qemu
    - linux
    - drm
    - chromium
    - graphics
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-08T12:20:00-04:00
WhatFor: ""
WhenToUse: ""
---

Yes. The Chromium path you want is **Ozone DRM/GBM**: direct Linux **DRM/KMS + GBM**, without X11 or a Wayland compositor in front of Chromium. Chromium’s own Ozone docs describe that backend as direct rendering via DRM/KMS and say it is in production use on Chrome OS. The catch is that on generic Linux, current GN defaults auto-enable **X11 and Wayland**, while **`ozone_platform_drm` is not enabled by default**; the Chromium tree has a separate “Running ChromeOS UI on Linux” doc for the DRM path, and that doc explicitly warns that experimentation may be necessary. ([Chromium Git Repositories][1])

There are a few nearby projects, but they split into two camps. The closest Chromium-family thing is **Chromium Ozone/DRM** itself, plus the ChromeOS-style “ash-chrome on Linux” bring-up doc. If you want a **direct-to-KMS web runtime** but do not strictly need Chromium, **WPE WebKit + Cog** is a very real embedded path: WPE’s FAQ says Cog can run fullscreen directly on **KMS/DRM** without a running Wayland compositor. If you want a **single fullscreen web app on embedded Linux**, **Qt WebEngine + EGLFS/KMS** is another established route; Qt documents EGLFS as running without X11 or Wayland and notes that many embedded systems use DRM/GBM through `eglfs_kms`. If you decide you mainly want a kiosk and not direct DRM ownership by the browser itself, **Cage** is the minimal Wayland kiosk compositor route. Also telling: the current OpenEmbedded `meta-chromium` layer exposes `chromium-ozone-wayland` and `chromium-x11`, not a mainstream packaged ozone-drm recipe, which is a good clue that the packaged ecosystem is still more Wayland-centric than raw DRM-centric for Chromium. ([WPE WebKit][2])

For **your repo specifically**, this is not a one-flag change. Right now your tree boots **Weston DRM**, forces **`--renderer=pixman`**, launches Chromium with **`--ozone-platform=wayland`**, and passes **`--disable-gpu`**. That is a valid “Chromium on Wayland kiosk” stack, but it is not direct-DRM Chromium. To get what you want, I would treat this as a **new phase** rather than mutating phase 3 in place.

Here is the shortest practical path.

1. **Freeze your current Wayland branch and start a new direct-DRM branch.**

Keep your current phase-3 code as the known-good fallback. Create a new phase, something like:

* `guest/init-phase4-drm`
* `guest/build-phase4-rootfs.sh`
* `guest/content-shell-drm-launcher.sh`
* `guest/ash-chrome-drm-launcher.sh`

That keeps the direct-DRM experiments from tangling with the already-working Weston path.

2. **Use the cleaner QEMU display device for direct KMS bring-up.**

In your lab, I would default the new phase to:

* `-vga none`
* `-device virtio-gpu-pci`

instead of `virtio-vga`.

That gives you the cleaner KMS-only shape. Keep your existing USB keyboard/tablet devices and serial/QMP setup.

3. **Build a custom Chromium with the DRM backend enabled.**

This is the big one. A stock distro Chromium build is usually the wrong starting point here because Linux desktop defaults do not auto-enable the DRM backend, while the Chromium DRM bring-up doc uses a ChromeOS-style build configuration for this path. Chromium’s own doc uses `target_os="chromeos"`, `ozone_platform_drm=true`, and `ozone_platform="drm"`. ([Chromium Git Repositories][3])

A sane first pass is:

```bash
cd ~/chromium/src

gn gen out/DRM --args='
use_ozone=true
target_os="chromeos"
ozone_platform_drm=true
ozone_platform="drm"
use_system_minigbm=false
use_xkbcommon=true
is_debug=false
is_chrome_branded=false
is_official_build=false
use_pulseaudio=false
'

autoninja -C out/DRM chrome chrome_sandbox nacl_helper content_shell
```

That is intentionally a **pared-down** version of the published doc: I kept the core DRM bits and dropped obviously environment-specific pieces like Google-internal tooling and the old hardcoded sysroot from the example. The Chromium doc itself says this configuration has no builder and may need experimentation, so expect a few rounds here. ([Chromium Git Repositories][3])

4. **Do not start with full Chrome. Start with `content_shell`.**

Chromium’s Ozone docs explicitly use `content_shell --ozone-platform=drm` as the direct DRM/KMS smoke test. That is the right first milestone because it proves “browser engine can own KMS directly” before you drag in the rest of Chrome. They also note that for the DRM platform you may need to stop any other display server first. ([Chromium Git Repositories][1])

So your first launcher should be very small:

```sh
#!/bin/busybox sh
set -eu

URL=${1:-file:///root/index.html}
export HOME=/root
export XDG_RUNTIME_DIR=/run/user/0
export EGL_PLATFORM=surfaceless

exec /usr/lib/chromium-direct/content_shell \
  --ozone-platform=drm \
  --no-sandbox \
  "$URL"
```

Use a **local file** or `data:` URL first, not a network page. That removes DNS, certs, NSS, and font surprises from the first milestone.

5. **Build a new rootfs that copies your custom Chromium build, not the snap Chromium payload.**

Your current `build-phase3-rootfs.sh` copies the snap Chromium runtime and Wayland/Weston bits. For phase 4:

* remove Weston and seatd from the critical path
* copy your custom `out/DRM` artifacts into something like `/usr/lib/chromium-direct`
* keep using your `copy-runtime-deps.py` helper
* keep the XKB data, fontconfig, fonts, `udevadm`, and your existing module-loading logic

At minimum, copy:

* `chrome`
* `content_shell`
* `chrome_sandbox`
* `nacl_helper`
* `chrome_crashpad_handler` if your build emits it
* `icudtl.dat`
* `resources.pak`
* `v8_context_snapshot.bin`
* `chrome_100_percent.pak`
* `chrome_200_percent.pak`
* `locales/`

Then run your dependency copier against both `content_shell` and `chrome`.

This is also where you fix the runtime side of the graphics stack. In your current phase-3 logs, Weston is complaining about missing GBM/DRI pieces; for direct DRM Chromium, missing GBM/EGL/DRI runtime bits will kill you even faster.

6. **Make PID 1 bring up DRM/input and then hand the machine straight to Chromium.**

Your new `init-phase4-drm` should look like “phase 3 minus compositor”:

* mount `proc`, `sysfs`, `devtmpfs`, `tmpfs`
* create `/run/user/0`
* start `systemd-udevd`
* `udevadm trigger` for `drm` and `input`
* load the same virtio/input modules you already load now if they are not built in
* optionally unbind fbcon, exactly like your current helper does
* launch `content_shell-drm-launcher`

Skeleton:

```sh
#!/bin/busybox sh
set -eu

export PATH=/bin:/usr/bin:/usr/sbin

mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /run
mount -t tmpfs tmpfs /tmp
mount -t debugfs debugfs /sys/kernel/debug 2>/dev/null || true
mkdir -p /dev/pts /run/user/0 /var/log /var/lib /root
mount -t devpts devpts /dev/pts || true
chmod 0700 /run/user/0

# keep your existing module-load block here

/lib/systemd/systemd-udevd --daemon || true
udevadm trigger --action=add --subsystem-match=input || true
udevadm trigger --action=add --subsystem-match=drm || true
udevadm settle || true

for vtcon in /sys/class/vtconsole/vtcon*; do
  [ -r "$vtcon/name" ] || continue
  case "$(cat "$vtcon/name" 2>/dev/null || true)" in
    *frame\ buffer*)
      [ -w "$vtcon/bind" ] && echo 0 >"$vtcon/bind" 2>/dev/null || true
      ;;
  esac
done

exec /usr/bin/content-shell-drm-launcher file:///root/index.html \
  >/var/log/chromium.log 2>&1
```

7. **Once `content_shell` works, move to the ChromeOS-style direct-DRM Chrome path.**

For full Chrome on direct DRM, the published Chromium doc gives the closest thing to an official recipe. Its run command uses:

* `--ozone-platform=drm`
* `EGL_PLATFORM=surfaceless`
* `--enable-running-as-system-compositor`
* `--login-profile=user`
* `--use-gl=egl`
* `--enable-wayland-server`
* `--login-manager`

That is not “plain desktop Chrome in kiosk mode”; it is much closer to **ash-chrome / ChromeOS UI bring-up**. That matters. The doc is basically telling you that the well-trodden direct-DRM full-Chrome path is the ChromeOS-style one. ([Chromium Git Repositories][3])

So your second launcher should mirror that:

```sh
#!/bin/busybox sh
set -eu

PROFILE_DIR=/var/lib/chromium
mkdir -p "$PROFILE_DIR"

export HOME=/root
export XDG_RUNTIME_DIR=/run/user/0
export EGL_PLATFORM=surfaceless

exec /usr/lib/chromium-direct/chrome \
  --ozone-platform=drm \
  --enable-running-as-system-compositor \
  --login-profile=user \
  --user-data-dir="$PROFILE_DIR" \
  --use-gl=egl \
  --enable-wayland-server \
  --login-manager \
  --ash-constrain-pointer-to-root \
  --default-tile-width=512 \
  --default-tile-height=512 \
  --system-developer-mode
```

The important caveat is that Chromium’s own doc says this setup is old and may require experimentation. I would assume that up front instead of treating any first failure as surprising. ([Chromium Git Repositories][3])

8. **Only after that should you try “single-page Chromium kiosk direct to DRM.”**

I am less certain about the long-term support story for **plain**:

```bash
chrome --ozone-platform=drm --kiosk --app=https://...
```

on this raw direct-KMS path, because the public Chromium doc focuses on the ChromeOS/Ash-style route, not that minimal kiosk invocation. So I would not make that the first target. I would do the milestones in this order:

* `kms_pattern` works
* `content_shell --ozone-platform=drm` works
* ash-chrome direct-DRM works
* only then try a plain browser/app kiosk invocation

That keeps your debugging tree narrow.

9. **Debug in this order.**

When it fails, check in this order:

* `/dev/dri/card0` exists
* `/dev/input/event*` exists
* fbcon or another owner is not still holding the display
* Chromium log does not complain about missing GBM/EGL/DRI bits
* `content_shell` can render before `chrome`
* local `file:///root/index.html` works before external URLs

Chromium’s DRM doc explicitly calls out device-node access for `/dev/dri/*` and `/dev/input/*`, and the Ozone docs explicitly call out the DRM platform as a distinct runtime target that may require other display servers to be out of the way. ([Chromium Git Repositories][1])

The practical verdict is this:

* If you want the thing that is **closest to ChromeOS**, do **custom Chromium + Ozone DRM/GBM**, and think of it as **ChromeOS-style bring-up**, not “desktop Chromium with one extra flag.” ([Chromium Git Repositories][1])
* If you want the thing that is **most likely to ship quickly as a direct-to-KMS web kiosk**, **WPE/Cog DRM** or **Qt EGLFS/KMS** is the lower-friction route. ([WPE WebKit][2])

If you want, I can turn this into a concrete patch plan against your `qemu-attempt` tree with the exact new files and the minimal diff from phase 3.

[1]: https://chromium.googlesource.com/chromium/src/%2B/lkgr/docs/ozone_overview.md "https://chromium.googlesource.com/chromium/src/%2B/lkgr/docs/ozone_overview.md"
[2]: https://wpewebkit.org/about/faq.html "https://wpewebkit.org/about/faq.html"
[3]: https://chromium.googlesource.com/chromium/src/%2Bshow/refs/heads/main/docs/linux/ozone_drm.md "https://chromium.googlesource.com/chromium/src/%2Bshow/refs/heads/main/docs/linux/ozone_drm.md"
---
Title: Imported DRM Ozone Research Note
Ticket: QEMU-06-CONTENT-SHELL-DRM-OZONE
Status: active
Topics:
    - qemu
    - linux
    - drm
    - chromium
    - graphics
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-08T12:20:00-04:00
WhatFor: ""
WhenToUse: ""
---
