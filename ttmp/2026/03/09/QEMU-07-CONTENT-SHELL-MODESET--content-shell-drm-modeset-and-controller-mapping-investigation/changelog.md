# Changelog

## 2026-03-09

- Initial workspace created
- Imported `/tmp/ozone-answers.md` as the main research note for this narrower follow-up.
- Reframed the problem around Chromium Ozone DRM window/controller mapping rather than generic phase-4 bring-up.
- Added the focused design guide, task list, and diary scaffold for the new ticket.
- Parameterized the phase-4 init/launcher path so `content_shell` window size and fullscreen mode can be controlled from the guest kernel cmdline.
- Ran the first `800x600` no-fbdev control (`results-phase4-drm26`), then confirmed it was not a valid size-control test because `content_shell` ignores Chrome's generic `--window-size`.
- Switched the launcher to Chromium's real `--content-shell-host-window-size=WxH` flag and reran the control as `results-phase4-drm27`.
- Confirmed that the corrected switch changes Blink/content geometry to `800x595`, but scanout-capable `DrmThread` buffers still stay `814x669`, the connector remains disabled, and the host-visible frame remains the same `640x480` fallback image.
- Narrowed the next hypothesis from "content size mismatch" to "shell chrome / toolbar bounds still prevent exact controller mapping".
