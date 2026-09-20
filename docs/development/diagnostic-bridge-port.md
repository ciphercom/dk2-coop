# Diagnostic support for Flame developers

This branch ports the diagnostic tools used to develop DK2 Co-op. Its base is `e8553ca` on the co-op fork; review `git diff e8553ca...feature/diagnostic-bridge` once the port is committed. It adds diagnostic support to the existing game implementation. The bridge is disabled by default and action tracing starts disabled even when the endpoint is enabled.

## Adopt the standalone core first

| Piece | Files / integration |
| --- | --- |
| Command registry and bounded trace ring | `src/diagnostic/bridge.{h,cpp}`; no DK2 or co-op dependencies. |
| Local transport | `src/diagnostic/pipe.{h,cpp}`; Windows named pipe, copied strings, one mailbox, cancelable worker I/O. Link `advapi32`. |
| DK2 snapshots and dispatch observations | `src/diagnostic/game_bridge.{h,cpp}`; native DK2 ABI and Flame configuration. No co-op include or link dependency. |
| Native regression and transport host | `src/diagnostic/bridge_tests.cpp`, `bridge_test_host.cpp`; the budget test also includes `graphics_lifecycle.h`. |
| External tools | `tools/diagnostic_bridge.py`, `diagnostic_record.py`; Python standard library only. |
| External contracts | `tools/tests/test_diagnostic*.py`; set `DIAGNOSTIC_BRIDGE_TEST_HOST` to the selected build's synthetic host. |

Call `diagnostic::init()` on the game thread during normal runtime initialization, and `cleanup()` before that thread exits. The source hooks are in `src/patches/flame_main.cpp`, outside DllMain. `CFrontEndComponent` pumps with `nullptr` so the CLI can report an unavailable world while menus are active. `CGameComponent` pumps its live session. `MyGameSession` observes the local action queue before dispatch, copies a bounded world batch before handlers can mutate it, and records that original copy after world dispatch returns.

The co-op fork passes `network_hands::owner` to `init` as an optional Hand-owner provider. In upstream Flame, use `init()` with no argument and omit that co-op include: the Hand snapshot reports `keeper_hand_owners: null`. All other snapshot fields use native game state.

Only the game thread reads DK2 objects. The worker exchanges copied command/response strings and never follows game pointers. Keep the same-account/SYSTEM endpoint ACL, remote-client rejection, fixed command registry, 128-byte request limit, 64-KiB response limit, 128-event ring and timeout behavior. There is no arbitrary memory access, function invocation or action injection.

## Independent diagnostic additions

- `diagnostic/graphics_lifecycle.{h,cpp}` and observation hooks in `MyDirectDraw.cpp` and `MyCESurfHandle.cpp` log bounded graphics lifecycle state. The budget reserves a slot for the first missing-scratch observation. They use static graphics state and do not dereference a live world.
- The `tools/bug_hunter.cpp` addition preserves native `MESSAGE.LOG` before the crash-dialog child recreates it. This can be adopted independently of the bridge.
- `patches/test_availability.{h,cpp}` and its tests provide the explicitly enabled fresh-network-match availability override. Its `CGameComponent` option/load hook is separate from the bridge pump. This tool mutates test-world availability; both peers must opt in.
- Existing native `LogOOS` and protocol logging need no implementation changes. The usage guide explains their switches.

## Co-op-specific trace integrations

The changes in `network_possession.cpp`, `scripted_camera_hooks.cpp` and `network_gem_ending.cpp` observe the co-op state machines already present in this fork. Upstream Flame can omit these three integrations when adopting the core. When adapting them, preserve unconditional state transitions: some diagnostic conditions log the results of required gameplay calls, rather than replacing those calls.

No background-render workaround, retired Level1 trigger experiment, raw captures, machine-specific profiles or game-launch automation is part of this port. The reusable recorder replaces the old workspace-bound watch scripts. Developer tests and tools stay in the source tree; normal package payload rules are unchanged.

## Usage and validation

See [the diagnostic guide](../../tools/diagnostic_bridge.md) for CLI commands, recording, native logs and test controls. Build through this fork's `build.cmd`; when adopting upstream, register the same source/test targets in Flame's build entry point. The real-pipe tests use synthetic state and do not start a game.
