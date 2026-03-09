The shortest path to the answer is: your symptom is much more consistent with **Ozone DRM never binding the window to an enabled display controller** than with “Chromium can’t render.” In the DRM code, `DrmThread::CreateBuffer()` can allocate buffers even when there is no controller attached to the window; `ScreenManager::UpdateControllerToWindowMapping()` only maps **enabled** controllers to windows whose bounds **exactly equal** `origin + mode size`; and if `DrmWindow::SchedulePageFlip()` runs with no controller, it ACKs the swap but returns presentation failure instead of performing a real flip. That is exactly the pattern “buffers exist, connector/CRTC never becomes active from the app’s point of view.” ([Chromium Git Repositories][1])

That also makes your **814x669** observation suspicious. Chromium’s own Ozone docs still say `content_shell` “always runs at 800x600 resolution,” and the DRM mapping logic wants an exact bounds match between the window and the active controller’s mode rectangle. So on a `1280x800` display, `content_shell --start-fullscreen` is a bad probe for “did I really enter direct scanout/fullscreen KMS?” A size around 814x669 is not what I would treat as evidence of a real `1280x800` direct-DRM scanout path. ([Chromium Git Repositories][2])

With `drm_kms_helper.fbdev_emulation=0`, the first real modeset is supposed to come from **Chromium’s Ozone DRM display-configuration path**, not fbdev: `ScreenManager::ConfigureDisplayControllers()` calls `TestAndModeset()`, which does `TestModeset()` and then `Modeset()`, building the commit through `GetModesetControllerProps()` / `HardwareDisplayController::GetModesetProps()` and finally sending it through the DRM plane manager with `DRM_MODE_ATOMIC_ALLOW_MODESET`. So the right question is not “who replaced fbdev?” but “did Ozone DRM’s `TestModeset` + `Modeset` path actually succeed for this controller?” ([Chromium Git Repositories][3])

For the two deliverables you asked for, here is the focused set I would hand a researcher.

**1) Exact Chromium/Ozone DRM files to inspect, plus a targeted `--vmodule` setup**

The key files are:
`ui/ozone/platform/drm/gpu/screen_manager.cc` for display config, test-modeset, real modeset, and exact window↔controller matching;
`ui/ozone/platform/drm/gpu/drm_window.cc` for window bounds and the “ACK but no controller / presentation failure” path;
`ui/ozone/platform/drm/gpu/drm_thread.cc` for buffer allocation, window creation, and page-flip entry;
`ui/ozone/platform/drm/gpu/hardware_display_controller.cc` for enable/modeset/page-flip commit behavior;
`ui/ozone/platform/drm/gpu/crtc_controller.cc` for per-CRTC plane assignment and enable state;
`ui/ozone/platform/drm/gpu/drm_gpu_display_manager.cc` for display discovery/configuration orchestration;
`ui/ozone/platform/drm/gpu/hardware_display_plane_manager_atomic.cc` (and possibly `..._legacy.cc`) for the actual atomic commit/test layer;
then `drm_device.cc` / `drm_framebuffer.cc` if you need to go one layer deeper on framebuffer creation. Those paths are all present in the Chromium DRM tree listings, and Chromium’s logging docs confirm `--vmodule` is the right mechanism for targeted per-module verbosity. ([groups.google.com][4])

A focused logging config I would use is:

```bash
--enable-logging=stderr --v=1 \
--vmodule=*ui/ozone/platform/drm/gpu/screen_manager*=2,\
*ui/ozone/platform/drm/gpu/drm_window*=2,\
*ui/ozone/platform/drm/gpu/drm_thread*=2,\
*ui/ozone/platform/drm/gpu/hardware_display_controller*=2,\
*ui/ozone/platform/drm/gpu/crtc_controller*=2,\
*ui/ozone/platform/drm/gpu/drm_gpu_display_manager*=2,\
*ui/ozone/platform/drm/gpu/hardware_display_plane_manager*=2,\
*ui/ozone/platform/drm/gpu/drm_device*=2,\
*ui/ozone/platform/drm/gpu/drm_framebuffer*=2
```

That is a code-driven recommendation, not a random guess: it is aimed exactly at the modules that decide buffer allocation, window/controller binding, test-modeset, real modeset, and the final atomic commit. ([Chromium Git Repositories][3])

**2) QEMU virtio-gpu caveats relevant to “buffers exist, connector stays disabled”**

I did **not** find a primary-source Chromium statement that `virtio-gpu-pci + Ozone DRM` is generally unsupported in the specific sense “buffers can render but the connector will never activate.” QEMU’s virtio-gpu docs say virtio-gpu paravirtualizes both the GPU and display controller, and the default 2D backend expects the guest to use a **software renderer** such as Mesa or SwiftShader. That means “software rendering exists” is normal on virtio-gpu 2D and does not by itself explain the no-modeset symptom. ([qemu.readthedocs.io][5])

What I *did* find is that QEMU’s virtio-gpu / host-display plumbing is still moving. There is active 2025–2026 work around DRM native context support and fixes for host display backends where a refresh timer could wrongly disable active scanout in GTK/SDL. That does **not** prove your guest-side failure is a QEMU bug, but it does mean host-visible output and capture paths are noisy enough that I would not use them as the ground truth for guest KMS state. ([patchew.org][6])

On the ANGLE / `kms_swrast` question, I did not find a primary-source bug or design doc tying that specific combination to “render succeeds but connector activation does not.” The stronger evidence points elsewhere: QEMU documents software rendering on virtio-gpu 2D as expected, while Chromium’s DRM code makes controller enablement and exact bounds matching the decisive conditions for scanout. So I would treat ANGLE/`kms_swrast` as secondary unless logs point there directly. ([QEMU Documentation][5])

For **QMP `screendump`**, the QMP docs say it “captures the contents of a screen” from a QEMU display device/head, defaulting to the primary display. It is not documented as reading guest `/dev/fb0`, so turning off fbdev emulation does **not** automatically make `screendump` meaningless. But because there are active QEMU scanout/display-backend issues, I would treat `screendump` as evidence about the **host display pipeline**, not as authoritative proof that guest KMS is healthy. ([QEMU Documentation][7])

So the best guest-trusted proof is: first, inspect **guest atomic/KMS state**; second, do a **guest-side readback/capture** of the active framebuffer/plane; and only third, look at QMP `screendump`. That ranking matches Chromium’s own code path, because the Chromium modeset logic is driven by guest-side atomic test/commit and by whether the controller is enabled and exactly mapped to the window. ([Chromium Git Repositories][3])

If I had to debug this fast with one minimal run, I would **not** start with `content_shell --start-fullscreen`, because Chromium’s own docs say `content_shell` is fixed at 800x600. I would instead use full Chrome with a trivial page and the focused DRM logs:

```bash
chrome \
  --ozone-platform=drm \
  --start-fullscreen \
  --app='data:text/html,<body style="margin:0;background:#00ff00"></body>' \
  --enable-logging=stderr --v=1 \
  --vmodule=*ui/ozone/platform/drm/gpu/screen_manager*=2,*ui/ozone/platform/drm/gpu/drm_window*=2,*ui/ozone/platform/drm/gpu/drm_thread*=2,*ui/ozone/platform/drm/gpu/hardware_display_controller*=2,*ui/ozone/platform/drm/gpu/crtc_controller*=2,*ui/ozone/platform/drm/gpu/hardware_display_plane_manager*=2
```

That removes the `content_shell` 800x600 trap, uses a known-good page, and turns up exactly the modules that decide whether you got real controller binding and a real modeset. ([Chromium Git Repositories][2])

My overall read is: the most likely culprit is **window size / controller mapping mismatch**, with `content_shell` making that especially likely, and the second most likely culprit is a failure earlier in Chromium’s `TestModeset` / `Modeset` path. The QEMU/virtio side is worth keeping in mind, especially for host-visible capture, but I would debug Chromium’s DRM mapping and modeset path first. ([Chromium Git Repositories][3])

[1]: https://chromium.googlesource.com/chromium/src/%2B/61a4d502504bc1d928aafc13bf8369458e1a267b/ui/ozone/platform/drm/gpu/drm_thread.cc "ui/ozone/platform/drm/gpu/drm_thread.cc - chromium/src - Git at Google"
[2]: https://chromium.googlesource.com/chromium/src/%2B/HEAD/docs/ozone_overview.md "Chromium Docs - Ozone Overview"
[3]: https://chromium.googlesource.com/chromium/src/%2B/61a4d502504bc1d928aafc13bf8369458e1a267b/ui/ozone/platform/drm/gpu/screen_manager.cc "ui/ozone/platform/drm/gpu/screen_manager.cc - chromium/src - Git at Google"
[4]: https://groups.google.com/a/chromium.org/g/ozone-reviews/c/jUGH_iuOHbo "Update copyright headers in ui/, url/ [chromium/src : main]"
[5]: https://qemu.readthedocs.io/en/v9.2.4/system/devices/virtio-gpu.html "virtio-gpu — QEMU 9.2.4 documentation"
[6]: https://patchew.org/QEMU/20260306195048.2869788-1-alex.bennee%40linaro.org/20260306195048.2869788-4-alex.bennee%40linaro.org/?utm_source=chatgpt.com "[PULL 03/20] ui/gtk: Don't disable scanout when display is ..."
[7]: https://qemu.readthedocs.io/en/master/interop/qemu-qmp-ref.html "QEMU QMP Reference Manual — QEMU 10.2.50 documentation"
