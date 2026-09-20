"""Record bounded snapshots and action traces from one or two explicitly selected local DK2 processes."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import math
from pathlib import Path
import sys
import time

from diagnostic_bridge import BridgeError, process_id, request


def query(pid, command):
    """Keep transport failures visible in the capture instead of treating them as empty state."""
    try:
        return request(pid, command)
    except BridgeError as error:
        return {"ok": False, "error": str(error)}


def sample(pid):
    """Serialize each pipe's requests; snapshots are observations at successive game ticks."""
    return {"pid": pid, "time": time.time(), **{
        name: query(pid, command) for name, command in (
            ("session", "snapshot session"), ("player", "snapshot player"),
            ("hand", "snapshot hand"), ("possession", "snapshot possession"),
            ("actions", "trace drain"))}}


def record(pids, stream, seconds):
    """Own tracing for this capture and turn it off on normal completion or interruption."""
    failed = False

    def emit(row):
        nonlocal failed
        failed |= any(isinstance(value, dict) and value.get("ok") is False for value in row.values())
        stream.write(json.dumps(row) + "\n")
        stream.flush()

    with ThreadPoolExecutor(max_workers=len(pids)) as pool:
        try:
            for pid in pids:
                emit({"pid": pid, "time": time.time(), "trace_on": query(pid, "trace on")})
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                rows = list(pool.map(sample, pids))
                for row in rows:
                    emit(row)
                if all(row["session"].get("error") == "bridge_unavailable" for row in rows):
                    break
                # One-second drains overflowed the 128-event ring during possession tests.
                time.sleep(min(0.25, max(0, deadline - time.monotonic())))
        except KeyboardInterrupt:
            pass  # Ctrl-C is a normal stop; the final trace drain and shutdown still run.
        finally:
            # Stop every peer before writing: a full disk must not leave another peer tracing.
            final = [{"pid": pid, "time": time.time(), "trace_off": query(pid, "trace off"),
                      "actions": query(pid, "trace drain")} for pid in pids]
            for row in final:
                emit(row)
    return 1 if failed else 0


def duration(value):
    """Require an explicit finite recording window, with at most one hour per capture."""
    try:
        seconds = float(value)
    except ValueError:
        raise argparse.ArgumentTypeError("seconds must be a number") from None
    if not math.isfinite(seconds) or not 0 < seconds <= 3600:
        raise argparse.ArgumentTypeError("seconds must be greater than zero and at most 3600")
    return seconds


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", action="append", type=process_id, required=True,
                        help="local process ID; repeat once to observe a second peer")
    parser.add_argument("--output", type=Path, required=True, help="new JSONL file (existing files are preserved)")
    parser.add_argument("--seconds", type=duration, default=900, help="recording duration (default: 900)")
    args = parser.parse_args()
    if len(args.pid) > 2 or len(set(args.pid)) != len(args.pid):
        parser.error("select one or two distinct PIDs")
    try:
        with args.output.open("x", encoding="utf-8") as stream:
            result = record(args.pid, stream, args.seconds)
    except OSError as error:
        print(f"Recording failed: {error}", file=sys.stderr)
        return 1
    print(f"Recording complete: {args.output}")
    return result


if __name__ == "__main__":
    sys.exit(main())
