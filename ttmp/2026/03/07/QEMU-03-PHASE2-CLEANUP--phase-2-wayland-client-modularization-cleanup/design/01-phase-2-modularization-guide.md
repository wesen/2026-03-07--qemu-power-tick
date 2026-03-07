---
Title: Phase 2 Modularization Guide
Ticket: QEMU-03-PHASE2-CLEANUP
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
    - refactor
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: guest/build-wl-sleepdemo.sh
      Note: Updated modular build wiring
    - Path: guest/wl_app_core.h
      Note: Shared app state and public module interfaces
    - Path: guest/wl_net.c
      Note: Extracted reconnecting network state machine
    - Path: guest/wl_render.c
      Note: Extracted rendering module
    - Path: guest/wl_sleepdemo.c
      Note: Thin orchestration entry point after modularization
    - Path: guest/wl_wayland.c
      Note: Extracted Wayland setup and listener module
ExternalSources: []
Summary: Detailed plan for extracting the phase-2 Wayland client monolith into smaller modules while keeping current runtime behavior stable.
LastUpdated: 2026-03-07T17:56:10.748432332-05:00
WhatFor: Explain the modularization targets, interface boundaries, extraction order, and validation strategy for the phase-2 client cleanup pass.
WhenToUse: Use this guide when performing or reviewing the code-splitting work for the phase-2 guest and its build path.
---


# Phase 2 Modularization Guide

## Goal

Refactor the working phase-2 guest code into smaller modules without changing current behavior. The cleanup should make later suspend/resume and measurement work cheaper to implement and easier to reason about.

## Current Pressure Point

[`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) is now large enough that adding more lifecycle logic directly into it would compound coupling instead of moving the lab forward. The file currently combines:

- global app state,
- Wayland registry and listener setup,
- `wl_shm` buffer creation,
- pixel rendering,
- keyboard and pointer event handling,
- network reconnect logic,
- timer and signal handling,
- command-line parsing,
- main-loop orchestration.

That structure was acceptable during fast bring-up, but it is now the main tax on further progress.

## Non-Goals

- Do not add new phase-2 features in this ticket.
- Do not change the current visible dashboard intentionally.
- Do not redesign the networking protocol.
- Do not merge the stage-1 and phase-2 binaries into one executable in this cleanup pass.

## Target Module Map

### `guest/wl_app_core.[ch]`

Own the shared phase-2 app struct, enums, logging helpers, timing helpers, and small utility functions used across modules.

Expected responsibilities:

- `enum conn_state`
- `struct buffer`
- `struct app`
- `now_ms()`
- `log_line()`
- `set_redraw_reason()`
- small utility helpers that do not belong uniquely to Wayland, rendering, or networking

### `guest/wl_render.[ch]`

Own framebuffer allocation and software rendering.

Expected responsibilities:

- `memfd_create_compat()`
- `create_buffer()`
- `fill_rect()`
- `draw_char()`
- `draw_text()`
- `render()`

### `guest/wl_net.[ch]`

Own the reconnecting TCP client behavior.

Expected responsibilities:

- `set_nonblocking()`
- `schedule_reconnect()`
- `attempt_connect()`
- `handle_socket_event()`

### `guest/wl_wayland.[ch]`

Own protocol object discovery, listeners, and Wayland-specific bootstrap.

Expected responsibilities:

- registry listener
- `xdg_wm_base` listener
- keyboard and pointer listeners
- seat listener
- `setup_wayland()`

### `guest/wl_sleepdemo.c`

Become a small composition file.

Expected responsibilities:

- `parse_args()`
- `setup_epoll()`
- main-loop orchestration
- assembly of the extracted modules

## Extraction Order

1. Create a shared header that holds the app struct and the cross-module function declarations.
2. Move render code first because it is the most self-contained and least likely to disturb runtime behavior.
3. Move network code next because it already forms a small state machine with narrow dependencies.
4. Move Wayland listeners and setup last because that is the most coupled portion of the file.
5. Keep `main()` and epoll wiring in `guest/wl_sleepdemo.c` so the final orchestration remains easy to review.

## Validation Strategy

After each extraction slice:

- rebuild the client:
  - `guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
- rebuild the phase-2 initramfs if needed:
  - `guest/build-phase2-rootfs.sh build build/initramfs-phase2-client.cpio.gz build/wl_sleepdemo`
- boot the guest:
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-client.cpio.gz --results-dir results-phase2-cleanup`
- verify:
  - first frame appears,
  - client reconnects to the drip server,
  - pointer input still reaches the client,
  - keyboard input still reaches the client.

## Review Focus

The important review question is not “is this the perfect final architecture.” The important question is whether the extraction reduced coupling while keeping behavior stable and keeping the next feature work cheaper.
