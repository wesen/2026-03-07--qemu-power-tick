# Changelog

## 2026-03-07

- Initial workspace created.
- Copied the imported `lab2.md` brief into the stage-3 ticket.
- Identified the local Chromium packaging constraint: `chromium-browser` is only a snap launcher, while the actual browser payload is available under `/snap/chromium/current/usr/lib/chromium-browser/`.
- Added the first phase-3 guest builder, init path, browser launcher, and QEMU run script using the installed Chromium snap payload as the guest browser source.
- Hit and fixed an oversized initramfs failure by trimming the browser asset set and reducing fonts to a minimal DejaVu subset.
- Booted Chromium visibly under Weston and captured the first successful stage-3 screenshot in `results-phase3-smoke3/01-stage3.png`.
- Added deterministic stage-3 input-validation helpers:
  - `host/make_phase3_test_url.py`
  - `host/capture_phase3_checkpoints.py`
- Validated host keyboard injection into Chromium with `results-phase3-checkpoints1/01-after-keyboard.png`.
- Validated host pointer injection into Chromium with `results-phase3-checkpoints1/02-after-pointer.png`.
- Added stage-3 suspend plumbing with:
  - `guest/suspendctl.c`
  - `guest/build-suspendctl.sh`
  - `host/capture_phase3_suspend_checkpoints.py`
- Verified stage-3 suspend metrics and continuity split:
  - `pm_test=freezer` preserves the Chromium surface (`results-phase3-suspend-freezer1`)
  - `pm_test=devices` records good suspend metrics but loses visible display continuity after resume (`results-phase3-suspend2`, `results-phase3-suspend4`)
- Added a detailed stage-3 postmortem review doc assessing the result quality, process quality, struggles, and recommended next steps.
- Compared the stage-2 and stage-3 `pm_test=devices` resume logs and narrowed the new regression to stage-3-specific virtio-gpu / Chromium interaction rather than generic xHCI resume noise.
- Switched to smaller stage-3 control experiments by making the phase-3 rootfs launch either Chromium or `weston-simple-shm`, then discovered that continuous Weston-log writes to `/dev/console` were polluting the graphics-resume investigation.
- Cleaned the phase-3 debug path by removing the continuous Weston-log tail, which removed the earlier `virtio_gpu_dequeue_ctrl_func` invalid-resource errors in the `weston-simple-shm` `pm_test=devices` isolate while leaving a narrower display-continuity failure to investigate.
- Ran a corrected current-tree phase-2 `pm_test=devices` control with `wl_sleepdemo` present in the rootfs and found the same surviving post-resume visual fallback pattern as the cleaned stage-3 control, shifting the remaining diagnosis toward a shared scanout/console ownership issue.
- Re-ran cleaned stage 3 with Chromium and confirmed that the screenshot-level post-resume fallback matches the simpler controls, but Chromium still reintroduces the `virtio_gpu_dequeue_ctrl_func` invalid-resource errors, splitting the investigation into a shared base-layer display issue plus a narrower Chromium-associated DRM issue.
