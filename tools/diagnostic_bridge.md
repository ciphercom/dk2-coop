# Diagnostic bridge

This opt-in local bridge lets Flame developers inspect DK2 without switching game focus.
It reads named snapshots, captures bounded action traces, and temporarily toggles
debug logging. It does not expose memory addresses, arbitrary functions, or raw
GameAction injection.

## Use

Build through `build.cmd`, install the resulting DLLs into a development game copy, then launch DK2 from its installation directory
with `-skip-launcher -diagnostic-bridge`. Omit `-diagnostic-bridge` for ordinary play;
the bridge defaults to disabled. The existing `-level=level1 -q` options can launch
a local campaign mission for diagnostics; they do not establish network campaign
play.

Run the CLI with the DK2 process ID:

```powershell
.venv/Scripts/python.exe tools/diagnostic_bridge.py --pid 1234 snapshot session
.venv/Scripts/python.exe tools/diagnostic_bridge.py --pid 1234 trace on
.venv/Scripts/python.exe tools/diagnostic_bridge.py --pid 1234 trace drain
.venv/Scripts/python.exe tools/diagnostic_bridge.py --pid 1234 trace off
```

Each invocation writes one JSON line. Exit status is 0 for a successful request,
1 for a bridge/transport error, or 2 for invalid CLI arguments. No pip packages
are required. The CLI uses Windows named pipes and an overall five-second I/O
deadline.

The endpoint permits the process's Windows account and SYSTEM. Run the CLI with
an ordinary token for the same account as DK2. A restricted sandbox token can
return `bridge_access_denied` even for that account; use the approved ordinary
local execution path for pipe checks instead of widening the endpoint ACL.

| Command | Purpose |
| --- | --- |
| `snapshot session` | Session availability, simulation/transport identity, and diagnostic state. |
| `snapshot player` | The local interface's Keeper state. |
| `snapshot hand` | Local pending Hand entries, authoritative Keeper held contents and optional parallel `keeper_hand_owners` (network slot + 1; zero for native/scripted pickups). Without a registered ownership provider, this field is `null`. |
| `snapshot possession` | Keeper possession state and local camera presentation. |
| `trace on` / `trace off` | Start/stop action capture; initially off even when the bridge is enabled. |
| `trace drain` | Return and consume retained events and the overflow count. |
| `logging debug on` / `logging debug off` | Change debug logging for this process without saving the change to config. |

## Interpret the evidence

Snapshots are read on the game thread. An unavailable session is not an empty
Keeper: do not use it as evidence about gameplay. Loading or an unresponsive game
thread can cause a bounded timeout.

The trace retains the latest 128 events and reports discarded-event counts. Drain
it between short experiments; it is not a complete replay or an unbounded stream.
An event contains a sequence, tick, boundary, action type, Keeper tag (`player_id`),
and the three original action arguments. It does not invent a Controller ID.

- `local_queue_before_handle` observes the local queue before dispatch. A queue
  entry may be observed again if it remains queued; this is not an exactly-once
  submission receipt.
- `world_tick_return` identifies a batch that returned from world dispatch. It
  does not establish success of each action handler. Compare the action arguments
  and subsequent snapshots to determine effects.

The local queue uses the session tick; the world boundary uses the collected
action-batch tick. Session snapshots expose both session and world ticks. Hand
entries retain DK2's `has_under_hand` and `dropped` fields, with a `drop_target`
only when the cache says one exists; the bridge does not infer successful drops.

The local pipe is `\\.\pipe\flame-diagnostic-<PID>`. It accepts one exact ASCII
command followed by LF per connection (maximum 128 bytes including LF), and returns
one JSON line (maximum 64 KiB). The native registry also rejects unknown commands
and extra arguments; CLI validation is not its only boundary. Remote pipe clients
are rejected.

## Record a manual test

From this repository, select one or two local game PIDs and a new output filename:

```powershell
.venv/Scripts/python.exe tools/diagnostic_record.py --pid 1234 --output capture.jsonl --seconds 60
.venv/Scripts/python.exe tools/diagnostic_record.py --pid 1234 --pid 5678 --output peers.jsonl --seconds 120
```

The recorder reads session, player, Hand and possession snapshots and drains the action ring every 250 ms plus request time. Requests are serialized per PID and peers are observed concurrently. Records include timestamps, PIDs, trace-overflow counts and explicit transport errors. Snapshots are successive observations, not an atomic snapshot across peers. The recorder owns action capture for its selected processes: do not run another drainer at the same time. It stops tracing and drains retained events on completion or Ctrl-C. Existing output files are never overwritten. A failed bridge request is recorded and gives exit status 1. Captures default to 15 minutes and accept at most one hour; in-flight requests and cleanup can extend the wall-clock duration by their bounded I/O deadlines.

The pipe is local only. For two PCs, run a recorder on each PC and compare their captured simulation ticks; host clocks need not agree. The recorder never launches, focuses or terminates a game. Preserve useful findings, then remove raw captures after the investigation. No generic log/JSONL ignore rules are added.

## Related diagnostics on this branch

Enabling `-diagnostic-bridge` also enables bounded native `MESSAGE.LOG` traces:

- `[graphics-lifecycle]`: graphics initialization/destruction and the first missing scratch surface, with at most 128 lines per process.
- Possession entry, scripted-camera transitions and gem-ending state changes: bounded traces in the co-op integrations. These do not feed the bridge's action ring; inspect the native log as well as the JSONL capture.

The crash handler preserves the failing process's native log beside its unique crash report as `<report>.MESSAGE.LOG` before starting the crash-dialog process, which otherwise recreates that log. Copy failures are noted in the crash report. This preservation does not require the bridge.

For an out-of-sync investigation, enable the existing native `-dk2:LogOOS=true` setting on both peers. Its implementation and dump format are unchanged; preserve the first world dumps and compare corresponding world/tick state. Keep protocol logging off unless packet details are needed; `-flame:logging:protocol=true` enables the existing protocol logger. These options are separate from the bridge and remain off by default.

To reach spell/building interactions quickly, `-flame:prototype:all-available=true` enables the test availability helper. Enable it on **both peers before a fresh network match**. It exposes loaded spells, rooms, traps and doors for Keeper factions; it does not change research progress, resources or authored files. It deliberately changes test gameplay, skips saved games and single-player sessions, and is independent of the read-only snapshot interface. Leave it off when investigating mission availability or progression.

## Verify

`build.cmd` runs native bridge/graphics-budget checks, availability contracts, Python CLI/recorder tests and real-pipe integration tests before installing inside this repository. Both Debug and Release configurations run the same contracts. Nothing is deployed to a game automatically.

To repeat just the diagnostic tests against an already built host:

```powershell
$env:DIAGNOSTIC_BRIDGE_TEST_HOST = (Resolve-Path build/vs2026-debug/src/Debug/diagnostic_bridge_test_host.exe).Path
.venv/Scripts/python.exe -m unittest discover -s tools/tests -p "test_diagnostic*.py" -v
```

Choose the host from the configuration/generator you built; CI uses `vs2022-debug` or `vs2022-release`. The synthetic host exercises real local pipes, PID isolation, bounded errors and capture shutdown without starting DK2. See [upstream adoption](../docs/development/diagnostic-bridge-port.md) for code boundaries and integration points.
