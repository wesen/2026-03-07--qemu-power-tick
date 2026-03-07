Here’s a lab plan for the next two steps, written so you can hand it to an intern.

## Lab series: Wayland render demo, host-side capture, Chromium kiosk wake/sleep study

### Overall goal

Extend the stage-1 suspend/resume prototype into a guest system that:

1. runs a minimal **Wayland** stack with one demo client,
2. can be **observed and driven from the host**,
3. then swaps the demo client for **Chromium on Wayland**,
4. validates wake-on-input behavior,
5. and studies whether **Chromium itself** or the page it hosts is generating wakeups that interfere with the “sleep unless interacted with” model. Weston supports stand-alone DRM/KMS operation, `kiosk-shell` is explicitly meant for single-app fullscreen use, QEMU can capture guest screens and inject input via QMP/HMP, and Chromium’s Wayland path is the Ozone Wayland backend launched from a Wayland environment such as Weston. ([Wayland][1])

### Big-picture intent

This lab is still about **behavioral semantics**, not absolute power numbers. The working model is:

* the guest app stack should do as little as possible while idle,
* suspend with `freeze` / suspend-to-idle when idle long enough,
* resume on approved wake sources,
* redraw and reconnect,
* and be measurable from the host without needing a full desktop. Linux documents that writing `freeze` to `/sys/power/state` enters suspend-to-idle, and in that flow tasks are frozen until a wake-capable interrupt starts resume. ([Kernel Documentation][2])

---

## What the intern is building

This lab has **two implementation stages** and one **study track** that runs in parallel.

### Stage 2

A **Wayland demo** inside the guest:

* Weston compositor
* one tiny Wayland client
* host-side screenshot capture
* host-side injected input
* suspend/resume continuity

### Stage 3

A **Chromium under Weston** demo:

* Chromium using the Wayland backend
* Weston `kiosk-shell`
* wake on keyboard/mouse
* startup URL or typed URL path
* batch network delivery to reduce needless wakeups

### Study track

A structured investigation of:

* whether Chromium or the hosted page keeps waking the guest,
* which wake sources are involved,
* how much of the wake behavior is browser-internal vs page-driven vs network-driven.

Chromium’s own docs are useful here because they show both tracing/profiling hooks and an important behavioral fact: hidden-page timers are batched aggressively, but **visible pages are only minimally throttled**, which means a visible kiosk page can still generate frequent work if the page or browser is active. ([Chrome for Developers][3])

---

# Stage 2 — Minimal Wayland demo with host-side capture

## Objective

Replace the stage-1 terminal renderer with a minimal Wayland renderer while keeping the same suspend/resume control logic.

The guest should now have:

* Weston running as the compositor,
* one simple Wayland client that draws a status surface,
* host-side capture of the guest output,
* host-side injection of keyboard/mouse events,
* the same network-driven status model from stage 1.

Weston is explicitly designed to run on different backends, including a stand-alone DRM/KMS backend, and QEMU can capture a guest display with `screendump` and inject keys or pointer events through QMP. ([Wayland][1])

## Recommended architecture

Inside the guest:

* `weston`
* `wl-demo-client`
* `sleep-controller` logic, either embedded into the client or split into a small helper process if needed

On the host:

* QEMU launched with a QMP socket
* a host harness script that:

  * captures screenshots with QMP `screendump`
  * injects input with `send-key` / `input-send-event`
  * wakes the guest with `system_wakeup` when needed

QMP supports `screendump`, `send-key`, `input-send-event`, and `system_wakeup`, so the host can automate the whole experiment loop without manual GUI interaction. ([GitLab][4])

## Required guest functionality

The Wayland demo client should:

* connect to `WAYLAND_DISPLAY`,
* create one surface,
* draw a simple status screen,
* redraw only on:

  * timer,
  * state change,
  * network event,
  * post-resume,
* not run a continuous animation loop.

The point is to preserve the “sleep-friendly” design from stage 1. The page equivalent here is a static status board, not an animation.

## Rendering scope

The screen should contain:

* connection state
* packet count
* last packet time
* current suspend cycle number
* last suspend time
* last resume time
* “idle awake / suspending / resumed / disconnected” state label

No fonts or fancy toolkits are required if that slows development. A solid-color background plus text is enough.

## Capture scope

The host harness should take screenshots at these checkpoints:

* post-boot
* after client starts
* after first network data received
* just before suspend
* immediately after resume
* after a synthetic keyboard event
* after a synthetic mouse event

QEMU’s `screendump` can write the primary display to a file, and in modern QMP it supports image format selection. ([GitLab][4])

## Input scope

The host harness should support:

* injecting a key press
* injecting a short text sequence
* moving the pointer
* clicking

QMP documents both `send-key` and `input-send-event` for this. `input-send-event` also supports button and absolute/relative movement events. ([GitLab][4])

## Suspend/wake scope

The guest should:

* enter suspend-to-idle on inactivity,
* resume when the wake path works,
* redraw immediately after resume,
* continue statefully.

Linux’s suspend-to-idle flow freezes tasks and resumes them when a wake-capable non-timer hardware interrupt starts resume; on QEMU, `system_wakeup` is the reliable fallback for deterministic testing. ([Kernel Documentation][5])

## Stage 2 deliverables

The intern should produce:

* a `weston` launch script
* a minimal Wayland client
* a host-side QMP harness script
* a runbook
* a screenshot set showing the lifecycle
* a short note on what wake path was reliable in the VM

## Stage 2 acceptance criteria

Stage 2 is complete when:

* the guest boots into Weston,
* the Wayland client renders visibly,
* the host can capture the rendered output,
* the host can inject input,
* the guest suspends and resumes,
* the client redraws and continues after resume.

---

# Stage 3 — Chromium under Weston, input wake, URL path, batched network

## Objective

Swap the Wayland demo client for Chromium and validate that the same sleep/wake model still works.

Chromium’s Ozone docs say the Wayland backend is intended to be launched from a Wayland environment such as Weston and uses `--ozone-platform=wayland`. Weston’s `kiosk-shell` makes all top-level windows fullscreen, which is a good match for the single-app target. ([Chromium Git Repositories][6])

## Recommended rollout

Do this in two submodes.

### Mode A: debug browser mode

Run Chromium under Weston **without** fully locking into kiosk behavior yet.
Purpose:

* verify Wayland launch,
* verify rendering,
* verify typing and pointer injection,
* verify resume returns to the browser correctly.

### Mode B: kiosk mode

Run Weston with `kiosk-shell` and then run Chromium as the only app.
Purpose:

* simulate the final single-app environment,
* remove normal window-management complexity,
* measure idle behavior in the intended mode.

The reason for the two-step rollout is practical: URL typing and UI recovery are easier to validate before stripping everything down.

## Browser launch requirements

The browser launch setup should verify:

* Wayland backend is actually used
* profile data is stored in a predictable place
* startup URL can be passed from the command line
* trace output can be enabled for Chromium study runs

Chromium provides command-line tracing facilities such as `--trace-startup-file` / `--trace-startup-duration`, and Chromium’s tracing tools and profiling docs are meant for exactly this sort of process-level investigation. ([Chromium Git Repositories][7])

## URL-entry requirement

The lab should support both:

* **typed URL** path for interactive wake/input validation,
* **startup URL** path for reproducible steady-state runs.

Rationale:

* typed URL proves keyboard wake and input routing,
* startup URL removes typing variability in later measurements.

## Wake-on-input requirement

The intern should verify separately that:

* keyboard input wakes the guest or resumes the browser path correctly,
* mouse movement or click does the same,
* if direct input wake is unreliable in the VM, the fallback `system_wakeup` path still leads to a correct resumed UI.

Linux distinguishes system wakeup from runtime/remote wakeup, and system wakeup interrupts generally need to be configured explicitly. That matters because “input works while running” and “input wakes the system” are related but not identical questions. ([Kernel Documentation][8])

## Network batching requirement

This stage should introduce **batched network delivery**.

The first implementation should be at the **application/server level**, not the NIC level:

* instead of trickling tiny messages continuously,
* send bursts every N seconds,
* and observe whether the system sleeps more cleanly between bursts.

This is the right first step because Linux networking docs note that batching often comes from IRQ coalescing in the device, and `ethtool` exposes coalescing controls where the NIC/driver supports them; in a QEMU/virtio-net lab, driver-level coalescing support may not be the most portable thing to depend on, so user-space burst delivery is the safest baseline. That last point is an inference from the virtualization setup, not a documented virtio-net guarantee. ([Kernel Documentation][9])

## Optional network coalescing investigation

If the guest NIC exposes coalescing parameters:

* inspect with `ethtool -c`
* experiment with `ethtool -C`

The intern should record whether the virtual NIC supports any meaningful coalescing knobs before spending time on it. `ethtool` explicitly supports querying and changing coalescing settings. ([man7.org][10])

## Stage 3 deliverables

The intern should produce:

* Chromium launch scripts for debug mode and kiosk mode
* host-side automation for screenshots and injected input
* a burst-mode test server
* a comparison report:

  * unbatched trickle traffic
  * 1-second burst traffic
  * 5-second burst traffic
  * 10-second burst traffic

## Stage 3 acceptance criteria

Stage 3 is complete when:

* Chromium runs under Weston on Wayland,
* the guest can be resumed and Chromium still displays correctly,
* typed input can reach the browser in debug mode,
* kiosk mode works under `kiosk-shell`,
* burst-delivery networking runs end-to-end,
* the team has at least a first-pass answer to whether Chromium/page behavior is causing wakeups.

---

# Chromium wakeup study — explicit investigation track

## Question to answer

Does Chromium itself materially generate wakeups in this setup, and if so, which part is responsible?

That question should be treated as empirical. Chromium clearly has code paths for tracing/profiling, and Chrome’s timer-throttling behavior shows that visible pages are not aggressively suppressed the way hidden tabs are. So it is reasonable to expect wakeups from:

* page JS timers,
* rendering,
* network activity,
* audio/media,
* GPU/compositor activity,
* browser background tasks. The exact mix needs measurement. ([Chrome for Developers][3])

## Study design

Use a small test matrix.

### Case 1: static local page

* local `file://` or local HTTP page
* no JS timers
* no animation
* no network updates

Purpose:

* establish the quietest Chromium baseline.

### Case 2: visible page with `setInterval`

* 100 ms
* 1000 ms
* 5000 ms

Purpose:

* show whether page-level timers create regular wakeups.

Chrome documents that visible pages are under “minimal throttling,” while hidden pages move into once-per-second or once-per-minute batching depending on conditions. That means a visible kiosk page is exactly the regime where poorly written app code can keep the browser active. ([Chrome for Developers][3])

### Case 3: visible page with `requestAnimationFrame`

Purpose:

* show the cost of continuous visible rendering.

### Case 4: websocket or TCP-fed updates

* trickle 1 message/sec
* burst every 5 sec
* burst every 10 sec

Purpose:

* separate “browser wakes because the page is animated” from “browser wakes because data is arriving.”

### Case 5: hidden-page control

If possible, run a non-kiosk hidden-tab comparison.
Purpose:

* validate Chrome’s hidden-page batching behavior against observed behavior.

## Measurements to collect

For each case, collect:

### Guest-side

* suspend/resume log
* browser PID and child PIDs
* timestamps of network events
* timestamps of redraws
* `/sys/class/wakeup/.../event_count` and `wakeup_count` deltas before and after each run

Linux documents the `/sys/class/wakeup` ABI, including counters such as `event_count` and `wakeup_count`, which makes it appropriate for a wake-source inventory. ([Kernel Documentation][11])

### Host-side

* screenshot timeline from QMP `screendump`
* injected-input log
* wake command log
* network burst schedule log

### Chromium-side

* at least one trace run using Chromium tracing
* at least one CPU profile run for the main scenario

Chromium documents startup tracing with `--trace-startup-file` and has dedicated tracing/profiling documentation for deeper inspection. ([Chromium Git Repositories][7])

## What counts as evidence that Chromium is causing wakeups

Treat Chromium as implicated if, in the static-page baseline:

* the system stays awake more often than the Wayland demo client did,
* wake counters rise without corresponding host input or network bursts,
* Chromium traces show periodic browser work even when the page is meant to be quiet.

Treat the **page** as implicated if:

* switching to a quieter page drastically changes the wake behavior,
* timer-driven tests are much noisier than the static-page baseline.

Treat the **network path** as implicated if:

* burst scheduling reduces wake frequency materially without changing page code.

---

# Concrete task breakdown for the intern

## Work package A — Stage 2 Wayland render demo

Build:

* Weston launch config
* minimal Wayland client
* integration with suspend controller
* host-side QMP capture harness

Output:

* screenshots
* logs
* short implementation note

## Work package B — Stage 3 Chromium integration

Build:

* Chromium Wayland launch script
* debug mode and kiosk mode variants
* input injection harness for URL entry
* startup URL mode
* burst-mode test server

Output:

* browser screenshots
* wake/resume logs
* input-validation evidence

## Work package C — Chromium wakeup study

Build:

* test pages
* scenario runner
* trace/profiling commands
* wake-counter collection script
* comparison matrix

Output:

* one short report with tables:

  * scenario
  * expected wake sources
  * observed wake counts
  * did suspend occur
  * did resume recover cleanly
  * suspected source of unwanted wakeups

---

# APIs, interfaces, and resources the intern should study

## QEMU

* QMP `screendump`
* QMP `send-key`
* QMP `input-send-event`
* QMP `system_wakeup` ([GitLab][4])

## Linux power/wakeup

* `/sys/power/state`
* suspend-to-idle flow
* `/sys/class/wakeup`
* distinction between remote wakeup and system wakeup
* wakeup interrupts configuration ([Kernel Documentation][2])

## Weston / Wayland

* Weston stand-alone DRM/KMS backend
* Weston `kiosk-shell` ([Wayland][1])

## Chromium

* Ozone Wayland backend
* tracing via startup trace file
* profiling docs
* timer throttling behavior for visible vs hidden pages ([Chromium Git Repositories][6])

## Networking

* NAPI batching/coalescing concepts
* `ethtool` coalescing controls if available
* host-side burst scheduling as baseline strategy ([Kernel Documentation][9])

---

# Suggested milestones

## Milestone 1

Wayland client renders under Weston, host captures screenshot, host injects key/mouse.

## Milestone 2

Wayland client survives suspend/resume and redraws.

## Milestone 3

Chromium runs on Wayland in debug mode, input reaches browser, URL can be entered.

## Milestone 4

Chromium runs as sole app under `kiosk-shell`, startup URL path works, suspend/resume still works.

## Milestone 5

Burst-network experiments run, Chromium wakeup study report is complete.

---

# Risks and guidance

The main risk is trying to solve too much at once.

The intern should **not** begin by:

* tuning virtual NIC coalescing,
* optimizing GPU usage,
* proving real laptop S0ix behavior,
* or building a complex web app.

The correct order is:

1. render something under Weston,
2. automate capture and input from host,
3. prove suspend/resume continuity,
4. swap in Chromium,
5. then study wake behavior.

Also: for VM testing, do not require wake-on-network as a primary success condition. In Linux, runtime/remote wakeup and true system wakeup are distinct concepts, and in QEMU the most deterministic wake path is still usually explicit input injection or `system_wakeup`. ([Kernel Documentation][12])

If you want, I can turn this into a more formal intern handout with sections like “Context,” “Tasks,” “Milestones,” “Rubric,” and “Submission checklist.”

[1]: https://wayland.pages.freedesktop.org/weston/toc/running-weston.html "Running Weston — weston 15.0.90 documentation"
[2]: https://docs.kernel.org/admin-guide/pm/sleep-states.html?utm_source=chatgpt.com "System Sleep States"
[3]: https://developer.chrome.com/blog/timer-throttling-in-chrome-88 "Heavy throttling of chained JS timers beginning in Chrome 88  |  Blog  |  Chrome for Developers"
[4]: https://qemu-project.gitlab.io/qemu/interop/qemu-qmp-ref.html "QEMU QMP Reference Manual — QEMU  documentation"
[5]: https://docs.kernel.org/admin-guide/pm/suspend-flows.html?utm_source=chatgpt.com "System Suspend Code Flows"
[6]: https://chromium.googlesource.com/chromium/src/%2B/lkgr/docs/ozone_overview.md "Chromium Docs - Ozone Overview"
[7]: https://chromium.googlesource.com/chromium/src/%2B/lkgr/docs/memory-infra/memory_infra_startup_tracing.md "Chromium Docs - Startup Tracing with MemoryInfra"
[8]: https://docs.kernel.org/power/suspend-and-interrupts.html "System Suspend and Device Interrupts — The Linux Kernel  documentation"
[9]: https://docs.kernel.org/networking/napi.html "NAPI — The Linux Kernel  documentation"
[10]: https://man7.org/linux/man-pages/man8/ethtool.8.html "ethtool(8) - Linux manual page"
[11]: https://docs.kernel.org/admin-guide/abi-testing-files.html "Testing ABI Files — The Linux Kernel  documentation"
[12]: https://docs.kernel.org/driver-api/pm/devices.html "Device Power Management Basics — The Linux Kernel  documentation"
