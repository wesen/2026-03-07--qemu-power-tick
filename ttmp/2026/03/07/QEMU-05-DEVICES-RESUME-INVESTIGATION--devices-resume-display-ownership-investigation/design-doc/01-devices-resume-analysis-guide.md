---
Title: Devices Resume Analysis Guide
Ticket: QEMU-05-DEVICES-RESUME-INVESTIGATION
Status: active
Topics:
    - qemu
    - linux
    - power-management
    - wayland
    - chromium
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: guest/display_probe.sh
      Note: Guest-side display ownership probe sampled by the investigation
    - Path: guest/init-phase2
      Note: Phase-2 probe and fbcon-unbind wiring used in the shared-control runs
    - Path: guest/init-phase3
      Note: Phase-3 probe and fbcon-unbind wiring used in the stage-3 controls
    - Path: guest/kms_pattern.c
      Note: Bare-KMS dumb-buffer helper used to test post-resume scanout without Weston or Chromium
    - Path: host/capture_phase3_suspend_checkpoints.py
      Note: Host QMP screenshot harness used for pre/post suspend captures
    - Path: host/capture_qmp_screendump_variants.py
      Note: Reusable capture harness for explicit QMP device/head selection experiments
    - Path: host/capture_qmp_state_snapshots.py
      Note: Reusable state snapshot harness for QEMU monitor-visible resume comparisons
    - Path: host/probe_screendump_support.py
      Note: QMP schema probe used to confirm `screendump` device/head support on the active QEMU build
    - Path: host/run_phase3_qmp_state_capture.sh
      Note: Wrapper used to reproduce the QEMU/HMP state capture experiments
    - Path: host/run_phase3_screendump_variant_capture.sh
      Note: Wrapper used to reproduce the explicit device/head experiment matrix
    - Path: host/run_phase3_suspend_capture.sh
      Note: Concurrent stage-3 QEMU plus capture wrapper used for the corrected reruns
    - Path: ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/scripts/compare_image_ae.py
      Note: Ticket-local image difference helper used to compare corrected baseline and follow-on device-variant screenshots
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-07T22:12:20.785397822-05:00
WhatFor: ""
WhenToUse: ""
---





# Devices Resume Analysis Guide

## Executive Summary

This ticket is a narrow continuation of the stage-2 and stage-3 suspend/resume work. The problem is not “Chromium suspend is broken” in a vague sense. The current evidence says something more specific:

- Stage 2 and stage 3 both show a shared post-resume screenshot fallback after `pm_test=devices`.
- On `virtio-vga`, that fallback still looks like a firmware or early-console plane in QMP screenshots.
- On `virtio-gpu-pci` with default VGA disabled, the post-resume QMP screenshot no longer collapses to `720x400`, but it is still almost entirely black rather than showing the expected graphical scene.
- In the corrected bare-KMS control, the guest programs a `kms_pattern`-owned `1280x800` framebuffer after resume and QMP still returns a `720x400` frame, which proves the divergence can happen even without Weston or Chromium.
- QEMU 8.2.2 exposes `screendump` `device`, `head`, and `format` arguments in QMP schema, and explicit targeting has now been tested with stable ids on both `virtio-vga` and `virtio-gpu-pci`. It does not repair the bad post-resume capture.
- QEMU monitor-visible pre/post state is effectively stable across resume: `info pci` does not change, `info qtree` only changes USB enumeration order, and `x-query-virtio-status` for the virtio-gpu backend only resets `isr` and `queue-sel` in the same way on both display-device models.

The goal of this guide is to let a new intern understand the whole stack quickly, reproduce the existing evidence, and continue with smaller experiments rather than starting over.

## Problem Statement

We need to determine which layer is responsible for the visible post-resume fallback after `pm_test=devices`:

- framebuffer console ownership (`vtconsole` / fbcon),
- DRM scanout or plane restoration,
- Weston output recovery,
- client buffer reuse,
- or QEMU `screendump` capturing a fallback plane that is not the one Weston intends to present.

The mistake to avoid is collapsing all of those into one “graphics resume bug.” They are different failure domains and need different experiments.

## System Overview

### Stage 1

Stage 1 is the simplest power-management harness. A minimal guest boots, runs a custom test app, enters `pm_test` suspend, wakes on RTC, reconnects to a host socket, redraws, logs metrics, and powers off.

Relevant files:
- [guest/sleepdemo.c](../../../../../../guest/sleepdemo.c)
- [guest/init](../../../../../../guest/init)
- [guest/run-qemu.sh](../../../../../../guest/run-qemu.sh)

What stage 1 proves:
- basic suspend orchestration works,
- timing metrics work,
- the core problem is not “QEMU cannot suspend at all.”

### Stage 2

Stage 2 adds a real Wayland compositor and a custom Wayland client:

```text
QEMU
  -> virtio-vga / virtio-gpu + input devices
    -> Linux DRM + fbdev emulation + vtconsole
      -> Weston DRM backend
        -> wl_sleepdemo
```

Relevant files:
- [guest/init-phase2](../../../../../../guest/init-phase2)
- [guest/wl_sleepdemo.c](../../../../../../guest/wl_sleepdemo.c)
- [guest/wl_wayland.c](../../../../../../guest/wl_wayland.c)
- [guest/wl_render.c](../../../../../../guest/wl_render.c)
- [guest/wl_suspend.c](../../../../../../guest/wl_suspend.c)
- [guest/run-qemu-phase2.sh](../../../../../../guest/run-qemu-phase2.sh)

What stage 2 proves:
- Weston can boot on DRM in the minimal guest,
- pointer and keyboard input can be injected from the host,
- suspend/resume can be measured with a real Wayland stack.

### Stage 3

Stage 3 keeps the same lower layers and swaps in Chromium:

```text
QEMU
  -> virtio-vga / virtio-gpu + input devices
    -> Linux DRM + fbdev emulation + vtconsole
      -> Weston DRM backend
        -> Chromium Wayland client
```

Relevant files:
- [guest/init-phase3](../../../../../../guest/init-phase3)
- [guest/chromium-wayland-launcher.sh](../../../../../../guest/chromium-wayland-launcher.sh)
- [guest/suspendctl.c](../../../../../../guest/suspendctl.c)
- [guest/build-phase3-rootfs.sh](../../../../../../guest/build-phase3-rootfs.sh)
- [guest/run-qemu-phase3.sh](../../../../../../guest/run-qemu-phase3.sh)

What stage 3 changes:
- a more complex buffer lifecycle,
- more client-side GPU/GBM/EGL interactions,
- more opportunities for resume-time resource invalidation.

## Why QMP Screenshots Matter

The host capture helpers use QMP and HMP facilities to obtain screenshots and inject input. For this ticket, the critical operation is the QMP/HMP screenshot path used by [host/capture_phase3_suspend_checkpoints.py](../../../../../../host/capture_phase3_suspend_checkpoints.py).

Conceptually:

```text
host helper
  -> QMP socket
    -> QEMU screendump
      -> current visible guest output plane
```

This does **not** guarantee that the screenshot corresponds to the highest-level app surface. It only shows what QEMU believes is the current visible output. That distinction matters here, because the guest may still report a stable DRM/fb state while QEMU captures what looks like a fallback text plane.

The newer QMP schema probe reduces one uncertainty. On this QEMU 8.2.2 build, `screendump` is not fixed to a single implicit target. Its schema exposes:

- `filename`
- optional `device`
- optional `head`
- optional `format`

That means we can now plan explicit head/device-targeted follow-up runs instead of guessing whether QEMU can select among multiple scanout sources.

## Current Hypotheses

### Hypothesis 1: fbcon ownership fully explains the fallback

Prediction:
- if framebuffer console ownership is the main problem,
- unbinding the framebuffer `vtconsole` should preserve the graphical pre/post path.

Status:
- weakened by the corrected phase-2 unbind experiment.

### Hypothesis 2: guest ownership indicators stay stable, but visible scanout changes underneath

Prediction:
- `fb0`, `/proc/fb`, `vtconsole bind`, and DRM connector state remain stable,
- but QMP screenshots still switch planes after resume.

Status:
- strongly supported by `results-phase2-probe1`.

### Hypothesis 3: stage 3 adds an extra resource invalidation bug on top of the shared fallback

Prediction:
- stage 2 and stage 3 share the screenshot fallback,
- but stage 3 may additionally show `virtio_gpu_dequeue_ctrl_func ... response 0x1203`.

Status:
- partially supported.
- phase-3 `weston-simple-shm` also showed `0x1203` in `results-phase3-probe-shm1`, which weakens the earlier “Chromium-only” framing.

### Hypothesis 4: legacy VGA compatibility fully explains the bad post-resume capture

Prediction:
- switching from `virtio-vga` to `virtio-gpu-pci` with default VGA disabled should make the post-resume host screenshot correct.

Status:
- weakened, not confirmed.
- `virtio-gpu-pci` avoids the `720x400` firmware-looking frame, but the default post-resume screenshot becomes an almost-black `1280x800` frame instead of the correct graphical scene.

### Correction: a phase-3 parser bug invalidated part of the earlier evidence

The first phase-3 probe/unbind runs were later found to be partially invalid. In [guest/init-phase3](../../../../../../guest/init-phase3), `DISPLAY_PROBE` and `UNBIND_FBCON` were assigned inside a command substitution:

```sh
CONFIG=$(build_browser_args)
```

That runs the function in a subshell. The returned config string survived, but the parent shell never received the `DISPLAY_PROBE=1` or `UNBIND_FBCON=1` side effects. After fixing that bug and rerunning the stage-3 controls, the phase-3 evidence changed:

- `@@DISPLAY` now appears correctly in serial logs
- `display_unbind_fbcon=1` now really unbinds `vtcon0`
- the corrected reruns do not reproduce the old `0x1203` DRM errors

So any intern reading this ticket should treat the old `results-phase3-probe-shm1` and `results-phase3-probe-chromium1` interpretations as provisional, not final.

## Probe Design

The probe was designed to avoid modifying the visible runtime path.

Relevant file:
- [guest/display_probe.sh](../../../../../../guest/display_probe.sh)

Pseudo-code:

```text
loop every second:
  sample /proc/uptime
  sample /proc/fb
  sample /sys/class/graphics/fb0/name
  sample each /sys/class/vtconsole/vtcon*/{name,bind}
  sample each /sys/class/drm/card*-*/{status,enabled,dpms}
  append one @@DISPLAY line to /var/log/display_probe.log

on shutdown:
  tail display_probe.log to serial
```

Why this matters:
- it avoids writing to `/dev/console` continuously,
- it preserves an audit trail in the guest,
- it lets us compare guest ownership state to host-visible screenshots.

## Experiment Matrix

### Control A: Stage 2 probe baseline

Command:
```bash
bash guest/run-qemu-phase2.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase2.img \
  --results-dir results-phase2-probe1 \
  --append-extra 'display_probe=1 phase2_idle_seconds=5 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=20'
```

Observed:
- `00-pre.png`: `1280x800`, graphical plane
- `01-post.png`: `720x400`, firmware-looking text plane
- probe state stable before and after resume:
  - `fb0=virtio_gpudrmfb`
  - `vtcon0 bind=1`
  - `vtcon1 bind=0`
  - `Virtual-1 status=connected enabled=enabled dpms=On`

Interpretation:
- this is strong evidence against a simplistic “fbcon rebound” explanation.

### Control B: Stage 3 `weston-simple-shm`

Command:
```bash
bash guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-probe-shm1 \
  --append-extra 'display_probe=1 phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

Observed:
- same pre/post screenshot dimension pattern
- `virtio_gpu_dequeue_ctrl_func ... response 0x1203 (command 0x105)`
- `virtio_gpu_dequeue_ctrl_func ... response 0x1203 (command 0x104)`

Interpretation:
- the extra DRM error may not be Chromium-specific after all.

### Control C: Stage 3 Chromium

Command:
```bash
bash guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-probe-chromium1 \
  --append-extra 'display_probe=1 phase3_runtime_seconds=40 phase3_url=data:text/html;base64,... phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

Observed:
- same screenshot dimension pattern
- current probe run did not visibly dump `@@DISPLAY`
- truncated inspection did not show `0x1203` lines in the same obvious way

Interpretation:
- stage 3 still reproduces the shared visual fallback,
- but this particular run is weaker for fine-grained diagnosis than the phase-2 baseline.

### Intervention D: Stage 2 corrected `display_unbind_fbcon=1`

Command:
```bash
bash guest/run-qemu-phase2.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase2.img \
  --results-dir results-phase2-probe-unbind2 \
  --append-extra 'display_probe=1 display_unbind_fbcon=1 phase2_idle_seconds=5 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=20'
```

Observed:
- unbound the real framebuffer console:
  - `vtcon0 bind=0`
  - `vtcon1 bind=1`
- `00-pre.png` changed dramatically and showed visible graphical content
- `01-post.png` still fell back to the `720x400` firmware-looking plane

Interpretation:
- fbcon binding affects what QMP sees before suspend,
- but unbinding fbcon alone does not fix the post-resume fallback.

### Control E: Corrected stage-3 `weston-simple-shm`

Command:
```bash
bash host/run_phase3_suspend_capture.sh \
  --results-dir results-phase3-probe-shm2 \
  --append-extra 'display_probe=1 phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

Observed:
- `display_probe pid=...` and `@@DISPLAY` are present
- `00-pre.png`: `1280x800`
- `01-post.png`: `720x400`
- stable guest-side state across resume:
  - `vtcon0 bind=1`
  - `vtcon1 bind=0`
  - `fb0=virtio_gpudrmfb`
  - `Virtual-1 status=connected enabled=enabled dpms=On`
- no `0x1203` DRM error lines in this corrected run

Interpretation:
- the shared visual fallback survives the corrected phase-3 rerun,
- but the old “stage 3 also reliably throws `0x1203`” claim is now too strong.

### Control F: Corrected stage-3 Chromium baseline

Command:
```bash
bash host/run_phase3_suspend_capture.sh \
  --results-dir results-phase3-probe-chromium2 \
  --append-extra 'display_probe=1 phase3_runtime_seconds=40 phase3_url=data:text/html;base64,... phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

Observed:
- `@@DISPLAY` now appears correctly
- stable guest-side state across resume:
  - `vtcon0 bind=1`
  - `vtcon1 bind=0`
  - `fb0=virtio_gpudrmfb`
  - `Virtual-1 status=connected enabled=enabled dpms=On`
- `00-pre.png`: `1280x800`
- `01-post.png`: `720x400`
- no `0x1203` DRM error lines

Interpretation:
- corrected Chromium still reproduces the same visual fallback,
- but without the old probe/unbind bug or the old `0x1203` error signature.

### Intervention G: Corrected stage-3 Chromium with `display_unbind_fbcon=1`

Command:
```bash
bash host/run_phase3_suspend_capture.sh \
  --results-dir results-phase3-probe-chromium-unbind2 \
  --append-extra 'display_probe=1 display_unbind_fbcon=1 phase3_runtime_seconds=40 phase3_url=data:text/html;base64,... phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

Observed:
- `[init-phase3] unbound vtcon0 framebuffer console`
- probe shows:
  - `vtcon0 bind=0`
  - `vtcon1 bind=1`
- screenshots are unchanged compared with the corrected Chromium baseline:
  - `AE(pre_baseline, pre_unbind) = 0`
  - `AE(post_baseline, post_unbind) = 0`
- `00-pre.png`: `1280x800`
- `01-post.png`: `720x400`

Interpretation:
- in corrected stage 3, changing fbcon ownership does **not** change the visible screenshots,
- which differs from corrected phase 2, where unbinding changed the pre-suspend visible plane.

### Intervention H: Corrected stage-3 `weston-simple-shm` with `display_unbind_fbcon=1`

Command:
```bash
bash host/run_phase3_suspend_capture.sh \
  --results-dir results-phase3-probe-shm-unbind1 \
  --append-extra 'display_probe=1 display_unbind_fbcon=1 phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

Observed:
- `[init-phase3] unbound vtcon0 framebuffer console`
- probe shows:
  - `vtcon0 bind=0`
  - `vtcon1 bind=1`
- screenshots compared with corrected `weston-simple-shm` baseline:
  - `AE(pre_baseline, pre_unbind) = 942400`
  - `AE(post_baseline, post_unbind) = 0`

Interpretation:
- corrected stage-3 `weston-simple-shm` behaves like corrected phase 2:
  - unbinding changes the visible pre-suspend frame,
  - but does not change the post-resume fallback.
- That means the stage-3 difference is not generic to the whole stack. It is client-dependent.

### Intervention I: Guest `/dev/fb0` Readback vs Host `screendump`

Command:
```bash
bash host/run_phase3_suspend_capture.sh \
  --results-dir results-phase3-fbshot-shm1 \
  --pre-delay-seconds 8.0 \
  --post-resume-delay-seconds 1.0 \
  --append-extra 'phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices phase3_fb_capture=1 phase3_fb_capture_pre_delay_seconds=8 phase3_fb_capture_post_delay_seconds=17'

python3 host/extract_fbshot_from_serial.py \
  --serial-log results-phase3-fbshot-shm1/guest-serial.log \
  --output-dir results-phase3-fbshot-shm1/fbshots
```

Observed:
- guest-side `fb0` metadata before and after resume stayed:
  - `name=virtio_gpudrmfb`
  - `visible_width=1280`
  - `visible_height=800`
  - `stride=5120`
  - `bpp=32`
- guest-side framebuffer payload hashes were identical:
  - `sha256_raw(pre)  = 2acef41c4128716e648faccbc5c40a24d53862c57037cd362f31c83fad2237cb`
  - `sha256_raw(post) = 2acef41c4128716e648faccbc5c40a24d53862c57037cd362f31c83fad2237cb`
- guest-side framebuffer image diff:
  - `AE(fbshot-pre-bgrx, fbshot-post-bgrx) = 0`
- host-side QMP screenshots in the same run still showed:
  - `00-pre.png`: `1280x800`
  - `01-post.png`: `720x400`
  - `resume_ae = 1.024e+06`
- guest-vs-host pre-suspend comparison:
  - `AE(00-pre.png, fbshot-pre-bgrx.png) = 1023981`
  - guest `fb0` image mean luminance: `0.59`
  - host pre-suspend screenshot mean luminance: `137.4`

Interpretation:
- `/dev/fb0` is **not** the same visible plane that QMP `screendump` shows in the graphical pre-suspend state.
- `/dev/fb0` also remains unchanged across the suspend/resume interval in this test, while host-visible output changes drastically.
- That makes `/dev/fb0` a useful negative control, but not a valid guest-visible ground truth for this stack.
- The next readback attempt needs to move below fbdev, toward a DRM/KMS-oriented capture path or another compositor-authorized capture route.

### Intervention J: Validate That Weston Screenshot Authorization Depends on `--debug`, Not `kiosk-shell.so`

Command:
```bash
bash guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-guestshot-debug1 \
  --append-extra 'phase3_client=weston-simple-shm phase3_runtime_seconds=15 phase3_guest_capture=1 phase3_guest_capture_pre_delay_seconds=6 phase3_weston_debug=1'

python3 host/extract_guestshot_from_serial.py \
  --serial-log results-phase3-guestshot-debug1/guest-serial.log \
  --output-dir results-phase3-guestshot-debug1/guestshots
```

Observed:
- earlier no-debug run under the same kiosk shell failed with:
  - `Output capture error: unauthorized`
  - `Error: screenshot or protocol failure`
- the debug-enabled run logged:
  - `WARNING: debug protocol has been enabled. This is a potential denial-of-service attack vector and information leak.`
  - `Command line: /usr/bin/weston --backend=drm --renderer=pixman --shell=kiosk-shell.so --socket=wayland-1 --continue-without-input --debug --log=/var/log/weston.log`
- the same kiosk-shell session then produced a guest screenshot successfully:
  - `@@GUESTSHOT tag=pre path=/var/log/guestshots/wayland-screenshot-...png size=287651 sha256=decb3e03a100ded172651674d26cc7cd88510fd3f0880b4f885df339bd223431`
  - extracted file: `results-phase3-guestshot-debug1/guestshots/guestshot-pre.png`

Interpretation:
- `kiosk-shell.so` was **not** the thing rejecting `weston-screenshooter`.
- The real gate is Weston compositor debug/capture policy.
- In this environment, `weston-screenshooter` becomes available once Weston is launched with `--debug`, even with kiosk shell unchanged.
- This matches Weston’s own manual, which states that `--debug` exposes the `weston-screenshooter` interface and should not be used in production.

### Intervention K: Compare Pre/Post DRM Debugfs State Across `pm_test=devices`

Command:
```bash
bash host/run_phase3_suspend_capture.sh \
  --results-dir results-phase3-drmstate-shm1 \
  --pre-delay-seconds 8.0 \
  --post-resume-delay-seconds 1.0 \
  --append-extra 'phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices phase3_drm_state_capture=1 phase3_drm_state_capture_pre_delay_seconds=8 phase3_drm_state_capture_post_delay_seconds=17'

python3 host/extract_drmstate_from_serial.py \
  --serial-log results-phase3-drmstate-shm1/guest-serial.log \
  --output-dir results-phase3-drmstate-shm1/drmstate
```

Observed:
- host QMP screenshots in the same run still show:
  - `00-pre.png`: `1280x800`
  - `01-post.png`: `720x400`
  - `resume_ae = 1.024e+06`
- guest DRM state before resume:
  - `plane[31] -> fb=43`
  - `allocated by = weston`
  - `format = XR24`
  - `size = 1280x800`
  - `connector[36]: Virtual-1`
  - `crtc[35]: enable=1 active=1`
- guest DRM state after resume:
  - `plane[31] -> fb=42`
  - `allocated by = weston`
  - `format = XR24`
  - `size = 1280x800`
  - `connector[36]: Virtual-1`
  - `crtc[35]: enable=1 active=1`
- the meaningful diff is only a weston buffer flip:
```text
plane[31] fb: 43 -> 42
framebuffer[43] refcount: 2 -> 1
framebuffer[42] refcount: 1 -> 2
```
- the fbcon framebuffer remains present but not selected as the active plane:
  - `framebuffer[39]: allocated by = [fbcon]`

Interpretation:
- Guest-side KMS state does **not** collapse to a `720x400` mode or text-like plane after resume.
- The active plane remains bound to a Weston-owned `1280x800` XR24 framebuffer across the suspend/resume cycle.
- The strongest current interpretation is that QMP `screendump` is not reflecting the compositor-visible plane after `pm_test=devices`.

### Intervention L: Compare Guest Compositor Screenshots to Host QMP Screenshots Across Resume

Command:
```bash
bash host/run_phase3_suspend_capture.sh \
  --results-dir results-phase3-guestshot-debug-suspend1 \
  --pre-delay-seconds 8.0 \
  --post-resume-delay-seconds 1.0 \
  --append-extra 'phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices phase3_weston_debug=1 phase3_guest_capture=1 phase3_guest_capture_pre_delay_seconds=8 phase3_guest_capture_post_delay_seconds=17'

python3 host/extract_guestshot_from_serial.py \
  --serial-log results-phase3-guestshot-debug-suspend1/guest-serial.log \
  --output-dir results-phase3-guestshot-debug-suspend1/guestshots
```

Observed:
- guest compositor screenshots:
  - `guestshot-pre.png`: `1280x800`
  - `guestshot-post.png`: `1280x800`
  - `AE(guestshot-pre, guestshot-post) = 942400`
  - both are fully nonzero graphical images
  - mean luminance:
    - pre: `138.93`
    - post: `138.19`
- host QMP screenshots in the same run:
  - `00-pre.png`: `1280x800`
  - `01-post.png`: `720x400`
  - post mean luminance: `2.12`
- guest pre and QMP pre are same-size and close enough to be recognizably the same scene class:
  - `AE(00-pre.png, guestshot-pre.png) = 942400`

Interpretation:
- The guest compositor-visible output remains graphical after resume.
- The host QMP screenshot path still falls back to the firmware-looking `720x400` plane.
- Combined with Intervention K, this is near-smoking-gun evidence that the post-resume QMP `screendump` path is the layer that is out of sync with the guest-visible scene.

### Control M: Probe the Active QEMU Build for `screendump` Targeting Support

Command:
```bash
bash guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-qmp-schema2 \
  --append-extra 'phase3_client=weston-simple-shm phase3_runtime_seconds=12 phase3_no_suspend=1'

python3 host/probe_screendump_support.py \
  --socket results-phase3-qmp-schema2/qmp.sock \
  --output results-phase3-qmp-schema2/screendump-support.json
```

Observed:
- QEMU version: `8.2.2`
- `query-qmp-schema` is available
- `screendump` resolves to an argument object containing:
  - `filename`
  - optional `device`
  - optional `head`
  - optional `format`
- `query-display-options` reports:
  - `"type": "none"`

Interpretation:
- The current QEMU build can target explicit display devices and heads, but that selector is not enough to recover the correct post-resume scene.
- The next host-side follow-up should move below target selection and treat QEMU scanout/capture behavior as the primary suspect, because even monitor-visible device state stays effectively stable across resume.

### Control N: Compare `virtio-vga` and `virtio-gpu-pci`

Commands:
```bash
timeout 90 bash host/run_phase3_suspend_capture.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-device-vga1 \
  --append-extra 'phase3_client=weston-simple-shm display_probe=1 phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices phase3_drm_state_capture=1 phase3_drm_state_capture_pre_delay_seconds=4 phase3_drm_state_capture_post_delay_seconds=11'

timeout 90 bash host/run_phase3_suspend_capture.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-device-gpu-pci1 \
  --display-device virtio-gpu-pci \
  --disable-default-vga \
  --append-extra 'phase3_client=weston-simple-shm display_probe=1 phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices phase3_drm_state_capture=1 phase3_drm_state_capture_pre_delay_seconds=4 phase3_drm_state_capture_post_delay_seconds=11'
```

Observed:
- `virtio-vga`:
  - `00-pre.png`: `1280x800`
  - `01-post.png`: `720x400`
  - post mean luminance: `2.12`
  - post DRM state still points at a Weston-owned `1280x800` framebuffer
- `virtio-gpu-pci` + `-vga none`:
  - `00-pre.png`: `1280x800`
  - `01-post.png`: `1280x800`
  - post mean luminance: `0.11`
  - post image is almost entirely black
  - post DRM state still points at a Weston-owned `1280x800` framebuffer

Interpretation:
- Removing VGA compatibility changes the host-visible failure mode, so the legacy VGA path is part of the story.
- It does **not** make the default post-resume capture correct.
- The deeper problem is still that QEMU’s default `screendump` target is wrong or stale after resume.

### Control O: Corrected Bare-KMS Post-Resume Control

Commands:
```bash
timeout 90 bash host/run_phase3_suspend_capture.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-kms-pattern4 \
  --append-extra 'phase3_client=kms-pattern display_probe=1 phase3_runtime_seconds=25 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices phase3_drm_state_capture=1 phase3_drm_state_capture_pre_delay_seconds=4 phase3_drm_state_capture_post_delay_seconds=17'

python3 host/extract_drmstate_from_serial.py \
  --serial-log results-phase3-kms-pattern4/guest-serial.log \
  --output-dir results-phase3-kms-pattern4/drmstate
```

Observed:
- suspend starts at about `13.0 s` uptime
- resume completes at about `18.7 s` uptime
- the post KMS pattern is programmed **after** resume:
  - `@@KMSPATTERN ... pattern=post`
- post DRM state shows:
  - `plane[31]`
  - `fb=41`
  - `allocated by = kms_pattern`
  - mode still `1280x800`
- host QMP screenshots still show:
  - pre: `1280x800`
  - post: `720x400`
  - `size_mismatch (1280, 800) (720, 400)`

Interpretation:
- This is the strongest control in the ticket so far.
- It removes Weston and Chromium from the experiment.
- Even after a direct post-resume KMS plane update owned by `kms_pattern`, QMP still captures the wrong post-resume image on `virtio-vga`.

## What the Intern Should Conclude Right Now

Do **not** conclude:
- “Chromium is the root cause.”
- “fbcon is definitely rebinding after resume.”
- “Weston failed to resume” based only on screenshots.
- “phase 3 definitely throws `0x1203` on every corrected rerun.”

Do conclude:
- there is a shared lower-layer post-resume visibility problem,
- fbcon ownership influences the visible pre-suspend plane in corrected phase 2,
- corrected phase 3 shows client-dependent pre-suspend sensitivity to fbcon unbinding:
  - `weston-simple-shm`: yes
  - Chromium: no visible change in the current test page setup
- `/dev/fb0` is not the same plane as the visible graphical scene in corrected stage 3, even before suspend,
- a raw `fb0` readback therefore cannot be used as “what the guest is visibly showing” in this environment,
- `weston-screenshooter` is available in this stack when Weston is started with `--debug`,
- the earlier `unauthorized` failure was not a kiosk-shell-specific denial,
- guest DRM debugfs state remains healthy across resume,
- guest compositor screenshots remain healthy across resume under explicit debug capture,
- the host QMP `screendump` path is now the leading suspect for the `720x400` fallback,
- `virtio-gpu-pci` changes the failure shape, but does not make the default post-resume capture correct,
- the corrected bare-KMS control proves that the divergence can happen below Weston and Chromium,
- the stage-2 probe evidence points below simple `vtconsole` ownership,
- the earlier phase-3 probe gap was a shell bug, not an inherent logging limitation.

## Design Decisions

### Decision: split the investigation into a new ticket

Reason:
- the prior tickets were too broad for a new contributor to trust or continue cleanly.

### Decision: use small controls instead of restarting from scratch

Reason:
- stage 1 still reproduces cleanly,
- stage 2 and stage 3 already give useful separation,
- rewinding would destroy evidence without increasing clarity.

### Decision: prefer file-backed probes over console-tail debugging

Reason:
- log spam to `/dev/console` can perturb the graphics path being debugged.

### Decision: treat `weston-screenshooter` as a debug-only capture path

Reason:
- the same kiosk-shell configuration succeeds once Weston is launched with `--debug`,
- Weston documents that `--debug` exposes the screenshot interface,
- therefore screenshot authorization here is a compositor debug-policy knob, not a shell-specific capability.

### Decision: prioritize QMP / `virtio-vga` capture-path investigation over guest-side KMS restoration

Reason:
- guest DRM debugfs state still shows an active Weston-owned `1280x800` XR24 plane after resume,
- guest compositor screenshots still show a graphical `1280x800` scene after resume,
- bare-KMS post-resume control now also shows an active `kms_pattern` framebuffer while QMP still captures the wrong frame,
- so the strongest remaining inconsistency is in the host-visible capture path.

## Alternatives Considered

### Restart stage 2 from scratch

Rejected because:
- the current evidence is already structured enough to continue,
- the problem is narrowed better by controls than by a reset.

### Keep running full Chromium tests only

Rejected because:
- Chromium adds too many moving parts,
- a shared lower-layer issue can be hidden by higher-layer noise.

### Rely only on screenshots

Rejected because:
- screenshots alone cannot distinguish ownership state from captured output state.

### Assume `kiosk-shell.so` blocks screenshots

Rejected because:
- the same kiosk-shell session allows screenshots once Weston is started with `--debug`.

## Implementation Plan

### Immediate next tests

1. Re-run stage-3 `display_unbind_fbcon=1` with capture attached concurrently, not post-hoc.
2. Use the DRM debugfs state dump as the primary guest-side KMS truth source.
3. If a guest-visible screenshot comparison is needed, use the explicit investigation-only `phase3_weston_debug=1` knob instead of assuming kiosk shell prevents capture.
4. Investigate QEMU `screendump` / display-head selection next, using explicit `device/head` arguments on a guest launched with stable display-device ids.

### Likely follow-up experiments

1. Compare `pm_test=freezer` vs `pm_test=devices` under the same DRM debugfs and screenshot instrumentation.
2. Add a tighter immediate-post-resume DRM state capture to reduce the blind window between wake and the scheduled post dump.
3. Investigate QEMU scanout ownership or `screendump` plane selection directly.
4. Compare `virtio-vga` against `virtio-gpu-pci` with explicit `device/head` selection, because the default post-resume capture source still appears wrong in both configurations.
5. Only return to app-level hypotheses if the host capture path explanation fails.

Pseudo-code for the next disciplined loop:

```text
for each hypothesis in [fbcon_ownership, drm_scanout_restore, client_resource_invalidation]:
  choose the smallest test that isolates that hypothesis
  run one control and one intervention
  capture:
    - pre screenshot
    - post screenshot
    - serial log
    - probe dump
  update ticket diary immediately
  only then choose the next hypothesis
```

## Open Questions

- Why does QEMU `screendump` show a `720x400` firmware-looking plane when guest DRM debugfs reports an active `kms_pattern`-owned or Weston-owned `1280x800` framebuffer?
- Is `virtio-vga` maintaining a legacy VGA text/firmware surface that QMP `screendump` prefers after `pm_test=devices` resume?
- Why does `virtio-gpu-pci` avoid the `720x400` fallback but still produce an almost-black `1280x800` default post-resume screenshot?
- Why do explicit `screendump --device/--head` captures land on the same bad post-resume surfaces as the default capture on both `virtio-vga` and `virtio-gpu-pci`, even though QEMU monitor-visible device state remains effectively unchanged across resume?
- Are the `virtio_gpu_dequeue_ctrl_func ... 0x1203` lines in `results-phase3-probe-shm1` caused by the same lower-layer issue as the screenshot fallback, or were they primarily tied to the earlier broken runs?
- Is there a resume-time host-side display handoff event occurring in a narrower window than the current capture schedule can observe?

## References

- Ticket diary: [../reference/01-investigation-diary.md](../reference/01-investigation-diary.md)
- Ticket index: [../index.md](../index.md)
- Stage-2 ticket: [../../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md](../../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md)
- Stage-3 ticket: [../../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md](../../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)

### API and subsystem references

- Linux `pm_test`: `/sys/power/pm_test`
- Linux suspend entry: `/sys/power/state`
- Linux RTC wakealarm: `/sys/class/rtc/rtc0/wakealarm`
- Linux framebuffer info: `/proc/fb`
- Linux vtconsole state: `/sys/class/vtconsole/vtcon*/`
- Linux DRM connector state: `/sys/class/drm/card*-*/`
- Linux framebuffer device: `/dev/fb0`
- QMP screenshot/input harness: [host/capture_phase3_suspend_checkpoints.py](../../../../../../host/capture_phase3_suspend_checkpoints.py)
- QMP schema probe: [host/probe_screendump_support.py](../../../../../../host/probe_screendump_support.py)
- Guest framebuffer dump helper: [guest/dump_fb0_snapshot.sh](../../../../../../guest/dump_fb0_snapshot.sh)
- Host framebuffer extractor: [host/extract_fbshot_from_serial.py](../../../../../../host/extract_fbshot_from_serial.py)
- Guest DRM state dump helper: [guest/dump_drm_state.sh](../../../../../../guest/dump_drm_state.sh)
- Host DRM state extractor: [host/extract_drmstate_from_serial.py](../../../../../../host/extract_drmstate_from_serial.py)
- Guest bare-KMS helper: [guest/kms_pattern.c](../../../../../../guest/kms_pattern.c)
- Weston manual for `--debug` and screenshot exposure: https://manpages.debian.org/testing/weston/weston.1.en.html
