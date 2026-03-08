---
Title: Imported KMS Research Answers
Ticket: QEMU-05-DEVICES-RESUME-INVESTIGATION
Status: active
Topics:
    - qemu
    - linux
    - power-management
    - wayland
    - chromium
DocType: source
Intent: reference
Owners: []
RelatedFiles: []
ExternalSources:
    - /tmp/chat-gpt-research-kms.md
Summary: Imported external research note answering the current DRM/KMS, QMP screendump, and scanout-trust-boundary questions for the devices-resume investigation.
LastUpdated: 2026-03-07T23:46:37.961670038-05:00
WhatFor: Preserve the imported KMS/QMP research note inside the ticket in normalized form.
WhenToUse: Use this when checking whether the local display-capture conclusions align with the imported QEMU, DRM/KMS, and Weston research.
---

The highest-value answer is: **treat guest atomic KMS state as ground truth for whether the guest resumed correctly, and treat QEMU `screendump` as ground truth only for what the host is currently showing.** QEMU’s `screendump` captures a **QEMU display device/head**, not “the Linux framebuffer” as such, and QEMU’s display console code can source that image from the console’s current scanout backing, including a surface, dmabuf, or texture. So when guest DRM state and host screenshots disagree, the strongest interpretation is usually “the guest’s intended display state and QEMU’s current host-visible scanout have diverged somewhere after KMS state was established.”

On your first question: **`screendump` is closest to “whatever QEMU currently considers the active scanout for that display head.”** The QMP docs say it captures “the contents of a screen” for a selected display device/head, and QEMU’s console code switches among different scanout backings rather than reading a specific guest abstraction like `/dev/fb0` or a DRM primary plane directly. So the most accurate mental model is not “DRM primary plane” or “fbdev emulation,” but **the current QEMU scanout-backed display surface for that head**.

On `/dev/fb0` in a Weston DRM session: **yes, it is entirely plausible for `/dev/fb0` to look stale, black, or otherwise unrepresentative while Weston is correctly presenting through KMS.** Weston’s DRM backend uses the Linux KMS API and does composition primarily through EGL/GLES, and it can also use direct scanout/overlays. The fbdev helper is mainly compatibility/fallback plumbing; kernel helper docs explicitly describe fbdev restore as a mechanism to keep users from getting a black screen when the compositor releases the display, which is a strong hint that fbdev is not the authoritative path while the compositor is actively driving KMS.

After `pm_test=devices`, the guest-side object you should treat as authoritative for the visible scene is **not the connector alone**. The connector tells you where the signal is routed, but the actual image comes from **plane state feeding a CRTC**, with the relevant `FB_ID` values on the active plane(s). In practice, the minimum meaningful tuple is: **connector → CRTC link, CRTC active/mode state, and the `FB_ID` on each active plane for that CRTC**. If Weston is using only the primary plane, the primary plane’s `FB_ID` may be enough; if it is using overlays/cursor/direct scanout, you need all active planes.

For the smallest correct guest-side API, I would use **DRM/KMS ioctls**, with **`modetest` as the fastest practical frontend**. That gives you the current connector/CRTC/plane/framebuffer state that the kernel is actually exposing through KMS. Weston-specific logging is useful, and debugfs can be very informative, but for “what does the guest kernel think the display state is right now?” the cleanest oracle is the KMS object model itself. ([Kernel Documentation][1])

For the “firmware-looking 720x400 plane” question: I did **not** find a canonical documented virtio-gpu/virtio-vga resume quirk in the sources I checked that exactly says “resume falls back to 720x400 while guest DRM stays healthy.” I would therefore not treat that as an established named bug from the documentation alone. But that symptom is **much more consistent with a stale or fallback host-visible scanout path** than with Weston genuinely still displaying the resumed scene, especially if your guest KMS state still shows the expected CRTC/plane/`FB_ID` assignments.

`pm_test=devices` differs from `pm_test=freezer` in exactly the way that matters here: **`freezer` only freezes and thaws tasks**, while **`devices` actually suspends and resumes devices**. Kernel PM docs say that the `devices` test level freezes processes, suspends devices, waits, resumes devices, then thaws processes. For DRM helper-based drivers, the suspend/resume helpers include atomic suspend/resume, output polling disable/enable, and fbdev helper suspend/resume. So this is the level where your virtio-gpu display stack, fbcon/fbdev helper state, and effective scanout handoff can genuinely change; `freezer` does not exercise that path.

If scanout restoration is working correctly in Weston’s DRM backend, the resume path you want to see is basically: **the output/CRTC remains or becomes enabled again, repaint resumes, and atomic commits/page-flips start happening again with sensible plane assignments and `FB_ID`s.** Weston’s docs describe `weston_output` as the thing responsible for framebuffer management and repaint state, corresponding to a CRTC when enabled, and Weston’s `drm-backend` debug scope is documented as reporting view-to-plane transitions, current assignments, compositing mode, and plane properties like `CRTC_ID`, `FB_ID`, and `FORMAT` during commits/page-flips. So the healthy trace is not “fbdev wakes up,” but “Weston’s DRM output resumes committing sane KMS state.” ([wayland.pages.freedesktop.org][2])

For screenshot capture inside Weston: I did **not** find a single official Weston man-page-style “canonical minimal screenshot command” that is clearly better than the other options. What I *did* find is that Weston has a private `weston-debug` protocol when run with `--debug`, and there is also a Weston output-capture protocol for image capture. So there are compositor-side capture mechanisms, but I would not over-invest in them for this bisect unless you already have them wired up; they are useful as a second witness, not as the first oracle. ([wayland.pages.freedesktop.org][2])

The earlier `virtio_gpu_dequeue_ctrl_func ... 0x1203` messages do **not** look like harmless generic Chromium noise. In the virtio-gpu headers, `0x1203` is `VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER`, and the commands around `0x0103`/`0x0104`/`0x0105` are `SET_SCANOUT`, `RESOURCE_FLUSH`, and `TRANSFER_TO_HOST_2D`. The Linux virtio-gpu dequeue path logs exactly these error responses. So those messages are evidence that the virtio-gpu 2D/scanout path was receiving invalid parameters at that point. Chromium may have been the workload that provoked them, but the codes themselves point at the scanout/flush protocol path, not just random browser chatter.

If I had one afternoon to bisect this, the single control experiment I would trust most is: **after resume, bypass Weston/Chromium entirely and perform one deliberately simple KMS test on the guest—set or flip the same CRTC/primary plane to a known dumb-buffer pattern—while simultaneously capturing QEMU `screendump`.** If guest KMS state updates cleanly and the known pattern is what KMS says should be on the active plane, but QEMU still shows the bogus 720x400-ish image, then the bug is downstream of guest KMS, in the virtio/QEMU scanout path. If guest KMS cannot establish or retain the expected connector/CRTC/plane/`FB_ID` state, then the bug is guest-side. That experiment minimizes moving parts and directly compares the two layers you care about most.

So the meta-answer, stated bluntly, is this: **for correctness of the resumed guest display stack, trust guest KMS; for what the host actually captured, trust QEMU `screendump`; never treat `/dev/fb0` as the tie-breaker in a Weston DRM session.** A disagreement between KMS and `screendump` is exactly the signature of a broken handoff between “guest’s authoritative display state” and “host-visible scanout state.”

I can turn this into a very tight bisect checklist with exact commands and expected outputs for `modetest`, Weston debug scopes, and QEMU capture.

[1]: https://docs.kernel.org/gpu/drm-uapi.html?utm_source=chatgpt.com "Userland interfaces"
[2]: https://wayland.pages.freedesktop.org/weston/toc/libweston/log.html?utm_source=chatgpt.com "Logging/Debugging — weston 15.0.90 documentation"
