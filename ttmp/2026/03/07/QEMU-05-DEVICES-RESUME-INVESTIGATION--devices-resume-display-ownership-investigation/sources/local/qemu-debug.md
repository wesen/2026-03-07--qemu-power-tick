I went through the `ttmp` tickets, the phase-3 scripts, and the representative logs/screenshots.

The big picture is better than it feels: you already have a working **kernel → initramfs → seatd/udev → Weston DRM/kiosk-shell → Chromium Ozone/Wayland** stack. This is already Chromium on Wayland, not Xwayland. In Chromium’s Ozone docs, Linux desktop defaults to X11 unless you explicitly choose Wayland, and Wayland is expected to run inside a Wayland environment such as Weston; direct DRM/GBM is a separate Ozone platform. Weston’s kiosk-shell is also exactly the single-app fullscreen shell you want. ([Chromium Git Repositories][1])

Your repo proves that bring-up path works:

* `guest/init-phase3` starts `seatd`, Weston DRM, `kiosk-shell.so`, and keeps `xwayland=false`.
* `guest/chromium-wayland-launcher.sh` uses Wayland Ozone flags.
* `results-phase3-smoke3/01-stage3.png` shows Chromium visibly rendered.
* `results-phase3-checkpoints1/00-before-input.png`, `01-after-keyboard.png`, and `02-after-pointer.png` prove keyboard and pointer delivery end-to-end.

So the core “can I boot a kernel and get kiosk Chrome on Wayland?” question is basically **yes** in your current tree.

The catch is that the current working path is **software-rendered**, not the fuller GBM/accelerated path. Your phase-3 init forces `weston --renderer=pixman`, your Chromium wrapper passes `--disable-gpu`, and the serial logs still show `MESA-LOADER: failed to open ... /gbm/dri_gbm.so`. Chromium’s Linux Wayland docs explicitly expect system GBM/DRM pieces to be present, which matches the missing-runtime-assets shape in your rootfs. ([Chromium Git Repositories][1])

The real unresolved problem is not initial Wayland rendering. It is the `pm_test=devices` resume path.

From your files, the split is very clear:

* `pm_test=freezer` preserves the Chromium surface.
* `pm_test=devices` breaks **host-visible QMP capture** after resume.
* On `virtio-vga`, post-resume QMP images fall back to the SeaBIOS/iPXE-looking `720x400` frame, like `results-phase3-suspend3/01-post-resume.png` and `results-phase3-screendump-vga1/post-default.png`.
* On `virtio-gpu-pci`, post-resume QMP images become a black `1280x800` frame with “Display output is not active.”, like `results-phase3-screendump-gpu-pci1/post-default.png`.

What matters most is that your later investigation already found the right witness hierarchy. The guest-side state still looks healthy after resume:

* `results-phase3-guestshot-debug-suspend1/guestshots/guestshot-post.png` is still graphical.
* `results-phase3-drmstate-shm1/drmstate/drmstate-post.txt` shows `active=1`, mode `1280x800`, and a Weston-owned FB on the active plane.
* `results-phase3-kms-pattern4/drmstate/drmstate-post.txt` shows the same thing even when Weston/Chromium are removed and a bare KMS helper owns the framebuffer.

That last point is the smoking gun. Once bare KMS still says “active 1280x800 framebuffer is live” while QMP still shows the wrong thing, this stops being a Chromium problem and mostly stops being a Weston problem. It becomes a **QEMU host-visible capture / scanout restoration** problem.

Your explicit `screendump --device/--head` work also matters. QEMU’s QMP docs say `screendump` uses the primary display by default, and `head` is only valid together with `device`; your harness results match that and rule out a trivial selector mistake. ([QEMU][2])

Your Weston screenshot result also lines up with upstream docs: `--debug` exposes the screenshooter interface, so using `weston-screenshooter` as a debug witness is valid, but only as a debug witness. ([Debian Manpages][3])

There is a related upstream QEMU virtio-gpu S3 black-screen issue, but your case looks narrower and better defined: in that report the guest display itself fails to come back, whereas your own guest-side screenshots and DRM state say the guest came back and QMP is the liar. ([about.gitlab.com][4])

My read is:

1. **Stage-3 Wayland kiosk bring-up is already achieved.**
2. **The current working mode is software-rendered and minimal, not yet the polished/accelerated build.**
3. **The remaining blocker you’re spending the most time on is really a QEMU capture/resume problem, not a Chromium-on-Wayland bring-up problem.**

So I’d split the work cleanly:

First track: **declare stage 3 functionally done** for “Chromium on Wayland under Weston kiosk-shell.” Keep the current software path stable. Only clean obvious runtime debt there: GLib schemas are likely why you still get the `g_settings_schema_source_lookup` warning; DBus is noisy but not a bring-up blocker in your current tests.

Second track: **build a separate acceleration branch**. In that one, copy the GBM/DRI pieces your current rootfs omits, remove `--disable-gpu`, and stop forcing Weston's pixman renderer. Right now your tree is intentionally avoiding that complexity, and the logs prove it. Chromium’s own docs are consistent with that split. ([Chromium Git Repositories][1])

Third track: **keep the resume investigation below the browser layer**. Use `weston-simple-shm` or `kms-pattern` as the fixed guest, and always collect the same three witnesses:

* guest compositor screenshot,
* guest DRM debugfs state,
* host QMP screendump.

Then test one different host-visible path, like VNC/Spice/SDL/GTK, to answer the next important question: is only `screendump` wrong, or is QEMU’s whole post-resume host-visible surface selection wrong?

That’s the main diagnosis: you are no longer “struggling to get Chrome onto Wayland.” You mostly already did that. You are struggling with a **post-resume host capture truth problem** and, separately, you still have an **optional future acceleration/GBM packaging problem**.

A good next step would be turning this into a concrete patch plan for either the acceleration branch or the QEMU-side resume/capture branch.

[1]: https://chromium.googlesource.com/chromium/src/%2B/lkgr/docs/ozone_overview.md "Chromium Docs - Ozone Overview"
[2]: https://www.qemu.org/docs/master/interop/qemu-qmp-ref.html "QEMU QMP Reference Manual — QEMU  documentation"
[3]: https://manpages.debian.org/experimental/weston/weston.1.en.html?utm_source=chatgpt.com "weston(1) — weston — Debian experimental"
[4]: https://gitlab.com/qemu-project/qemu/-/issues/1860 "virtio-gpu: Only black screen observed after resuming when guest vm do S3 (#1860) · Issues · QEMU / QEMU · GitLab"

