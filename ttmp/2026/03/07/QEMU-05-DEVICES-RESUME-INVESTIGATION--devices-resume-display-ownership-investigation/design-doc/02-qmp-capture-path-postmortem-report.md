---
Title: QMP Capture Path Postmortem Report
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
    - Path: guest/init-phase3
      Note: Current stage-3 bootstrap path that wires Weston, debug capture, and suspend controls together
    - Path: guest/dump_drm_state.sh
      Note: Guest-side DRM debugfs truth source used for the latest KMS-state controls
    - Path: guest/dump_wayland_screenshot.sh
      Note: Guest-side Weston screenshot helper used once debug policy was enabled
    - Path: host/run_phase3_suspend_capture.sh
      Note: Main host harness that captures QMP pre/post screenshots around suspend
    - Path: host/extract_drmstate_from_serial.py
      Note: Host-side extractor for DRM debugfs state blocks emitted by the guest
    - Path: host/extract_guestshot_from_serial.py
      Note: Host-side extractor for guest screenshots emitted by the guest
ExternalSources:
    - local:01-chat-gpt-research-kms.md
Summary: "Postmortem synthesis of the stage-3 devices-resume investigation: the guest display stack remains healthy after resume, while the host-visible QMP screendump path diverges."
LastUpdated: 2026-03-07T23:51:27.867656848-05:00
WhatFor: ""
WhenToUse: ""
---

# QMP Capture Path Postmortem Report

## Executive Summary

This report captures the current state of the `pm_test=devices` resume investigation after the stage-2 and stage-3 lab work converged on one narrower question: why does host-side QMP `screendump` show a `720x400` firmware-looking frame after resume when the guest still appears to have a healthy Wayland/DRM stack?

The strongest current conclusion is that the guest is probably resuming correctly and the host-visible QMP capture path is probably the thing that is wrong or stale. That conclusion is now supported by three independent lines of evidence:

- guest DRM debugfs state remains active and bound to a Weston-owned `1280x800` XR24 plane after resume,
- guest compositor screenshots from Weston itself remain graphical and `1280x800` after resume,
- host-side QMP `screendump` in the same runs still falls to a `720x400` firmware-looking image.

The imported KMS research in [01-chat-gpt-research-kms.md](../sources/local/01-chat-gpt-research-kms.md) helps because it matches the shape of this evidence very closely: treat guest KMS state as ground truth for whether the guest resumed correctly, and treat QMP `screendump` only as ground truth for what QEMU is currently exposing to the host.

## Problem Statement

The lab originally looked like a broad “graphics resume is broken” problem. That framing was too vague to debug well. The actual question is smaller and sharper:

- Did Weston and the guest DRM/KMS stack fail to restore the visible scene after `pm_test=devices`?
- Or did the guest restore correctly while QEMU’s host-visible capture path switched to or remained on the wrong display surface?

That distinction matters because the fixes are in different places:

- guest-side failure implies more work in Weston, DRM, fbcon handoff, or client redraw logic,
- host-side capture failure implies QEMU `screendump`, `virtio-vga`, or host-visible scanout selection is the real investigation target.

The current goal is therefore not “make screenshots look right by any means.” The goal is to identify which layer is actually lying.

## Proposed Solution

The proposed solution is to treat the system as a layered pipeline and assign each layer its own witness:

```text
guest app scene
  -> Weston compositor scene
    -> guest KMS state (connector -> CRTC -> plane -> FB_ID)
      -> virtio-gpu / virtio-vga transport
        -> QEMU host-visible scanout
          -> QMP screendump
```

For each layer we now have or should use the following witnesses:

- Guest application/compositor-visible scene:
  Weston screenshot under explicit `--debug`
- Guest KMS truth:
  DRM debugfs state from `/sys/kernel/debug/dri/0/state` and related files
- Host-visible QEMU output:
  QMP `screendump`

The postmortem conclusion is:

1. Do not use `/dev/fb0` as a truth source in the Weston DRM session.
2. Do not assume `kiosk-shell.so` blocked screenshots.
3. Use guest DRM debugfs state plus guest Weston screenshots as the primary correctness checks for guest resume.
4. Shift the next investigation step toward QEMU `screendump` / `virtio-vga` plane selection.

This is not just a preference. It is the only interpretation consistent with the current evidence set.

## Design Decisions

### Decision: Stop treating `/dev/fb0` as “what the user sees”

Reason:
- the raw `fb0` dump remained nearly black,
- its contents were byte-identical across suspend/resume,
- and it did not match even the pre-suspend graphical QMP frame.

Result:
- `fb0` is a useful negative control here, not a display truth source.

### Decision: Use DRM debugfs state as the main guest-side KMS witness

Reason:
- the debugfs `state` dump directly shows the active plane, the selected framebuffer, the mode, the connector, and the CRTC,
- it is lower-level than fbdev,
- and it avoids needing Weston screenshot authorization.

Result:
- we can now say the guest still has an active Weston-owned `1280x800` plane after resume.

### Decision: Use Weston screenshots only under explicit debug mode

Reason:
- Weston’s own docs say `--debug` exposes the screenshot interface,
- the earlier `unauthorized` result was not caused by `kiosk-shell.so`,
- and using `--debug` as a deliberate investigation knob is better than treating screenshot availability as mysterious policy.

Result:
- we now have direct guest-visible image evidence before and after resume.

### Decision: Shift suspicion from guest-side resume to host-side QMP capture

Reason:
- guest DRM debugfs remains healthy after resume,
- guest compositor screenshots remain healthy after resume,
- only QMP `screendump` falls to the firmware-looking `720x400` image.

Result:
- the next target is QEMU host-visible scanout selection, not more guest-side repaint guessing.

## Alternatives Considered

### Keep blaming `kiosk-shell.so` for screenshot failure

Rejected because:
- the same kiosk-shell session succeeds once Weston is launched with `--debug`.

### Keep debugging only Chromium

Rejected because:
- the same fallback showed up in the simpler `weston-simple-shm` controls,
- and the stage-3 problem is now narrower than “Chromium resume is broken”.

### Keep using `/dev/fb0` readback

Rejected because:
- it proved the wrong thing well,
- but it is not the compositor-visible plane in this setup.

### Restart stage 2 or stage 3 from scratch

Rejected because:
- the current evidence is already structured and internally consistent,
- and restarting would throw away useful controls rather than improving clarity.

### Assume guest DRM is broken because host screenshots are broken

Rejected because:
- the DRM debugfs state and guest compositor screenshots now contradict that assumption directly.

## Implementation Plan

The next concrete plan should move in the host/QEMU direction.

### Immediate next step

Investigate QEMU `screendump` / `virtio-vga` plane selection now that guest-side truth sources remain healthy after resume.

### Suggested experiment sequence

1. Keep the guest fixed at the known-good `weston-simple-shm` control path.
2. Keep collecting:
   - guest DRM debugfs state
   - guest Weston screenshot
   - host QMP `screendump`
3. Add one host/QEMU-side differentiator per run:
   - different QEMU display device if practical,
   - different QMP screen/head selection if available,
   - host query of display surfaces or consoles if QEMU exposes them,
   - simpler guest KMS test if needed.
4. Compare whether the host-visible capture continues to show the legacy-looking `720x400` plane while guest KMS remains on the Weston-owned `1280x800` plane.

### Minimal “one afternoon” experiment

The imported research suggests the best direct bisect:

```text
after resume:
  set or flip a very simple known framebuffer pattern from the guest KMS side
  capture guest DRM state
  capture guest compositor screenshot
  capture QMP screendump
  compare whether QMP follows the active KMS plane or not
```

If guest KMS and guest screenshots agree while QMP disagrees, the host capture path is the broken layer.

### Pseudocode

```text
boot known-good weston-simple-shm guest
capture:
  drm_state(pre)
  guestshot(pre)
  qmp(pre)

run pm_test=devices suspend/resume

capture:
  drm_state(post)
  guestshot(post)
  qmp(post)

if drm_state(post) sane and guestshot(post) sane and qmp(post) bad:
  debug qemu/virtio-vga capture path
else:
  return to guest-side resume logic
```

## Open Questions

The imported research is useful because it gives cleaner versions of the same questions we already ended up with:

- Why does QEMU `screendump` show a `720x400` firmware-looking frame when guest DRM debugfs still shows a Weston-owned `1280x800` active plane?
- Is QEMU `screendump` reading the wrong console or legacy surface after `pm_test=devices`?
- Is `virtio-vga` maintaining a fallback host-visible surface that diverges from the guest’s active DRM scanout?
- Can QEMU expose enough host-side display state to tell us which surface `screendump` is actually using?
- Are the older `virtio_gpu_dequeue_ctrl_func ... 0x1203` lines a separate bug, or just noisy by-products of earlier broken runs?

These are the right open questions now. They are host/QEMU-facing, not “did the guest repaint?” questions.

## References

- Main investigation guide: [01-devices-resume-analysis-guide.md](./01-devices-resume-analysis-guide.md)
- Investigation diary: [../reference/01-investigation-diary.md](../reference/01-investigation-diary.md)
- Imported KMS research: [../sources/local/01-chat-gpt-research-kms.md](../sources/local/01-chat-gpt-research-kms.md)
- Stage-3 ticket: [../../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md](../../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)
- Stage-2 ticket: [../../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md](../../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md)
- QMP harness: [host/qmp_harness.py](../../../../../../host/qmp_harness.py)
- Suspend capture harness: [host/run_phase3_suspend_capture.sh](../../../../../../host/run_phase3_suspend_capture.sh)
- Guest DRM state helper: [guest/dump_drm_state.sh](../../../../../../guest/dump_drm_state.sh)
- Guest screenshot helper: [guest/dump_wayland_screenshot.sh](../../../../../../guest/dump_wayland_screenshot.sh)
