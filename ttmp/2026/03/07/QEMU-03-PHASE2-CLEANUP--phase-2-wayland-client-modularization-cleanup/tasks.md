# Tasks

## Cleanup Plan

- [x] Write the cleanup design note describing the intended module boundaries, invariants, and non-goals for this refactor.
- [x] Start a dedicated cleanup diary and keep it updated after each extraction step and commit.
- [x] Define the initial extraction seams for the phase-2 guest:
  - Wayland object setup and listeners
  - software rendering and shared-memory buffer management
  - network reconnect state machine
  - common app state and logging helpers
- [x] Extract shared declarations into headers so the resulting modules have explicit interfaces instead of hidden file-local coupling.
- [x] Split `guest/wl_sleepdemo.c` into smaller translation units while preserving existing phase-2 behavior.
- [x] Update `guest/build-wl-sleepdemo.sh` so the modularized build remains simple and reproducible.
- [x] Mirror every new source/header/build helper into this ticket’s `scripts/` directory.
- [x] Rebuild the phase-2 client after the refactor and verify that it still compiles cleanly.
- [x] Boot the existing phase-2 guest with the modularized client and confirm no regressions in:
  - first frame
  - reconnect behavior
  - pointer input
  - keyboard input
- [x] Record the resulting module map, validation commands, and commit hashes in the diary and changelog.
- [ ] Run `docmgr doctor --ticket QEMU-03-PHASE2-CLEANUP --stale-after 30`.

## Non-Goals

- [ ] Do not add suspend/resume features in this ticket.
- [ ] Do not change the visible Wayland UI or checkpoint semantics except where required to preserve the existing build after extraction.
