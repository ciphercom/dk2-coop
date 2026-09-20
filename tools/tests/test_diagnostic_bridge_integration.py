"""Exercise the Python CLI through the real native named-pipe test host."""

import json
import os
from pathlib import Path
import queue
import subprocess
import sys
import threading
import tempfile
import time
import unittest


FLAME = Path(__file__).resolve().parents[2]
CLI = FLAME / "tools" / "diagnostic_bridge.py"
# build.cmd supplies the matching Debug/Release fixture; keep direct local runs convenient.
HOST = Path(os.environ.get("DIAGNOSTIC_BRIDGE_TEST_HOST",
    str(FLAME / "build/vs2026-debug/src/Debug/diagnostic_bridge_test_host.exe")))


class NativePipeTests(unittest.TestCase):
    """Each test owns a real process to verify PID isolation and transport behavior."""

    def setUp(self):
        self.start_host()

    def start_host(self, *arguments):
        """Start a fixture with the same transport but optionally no game-thread pump."""
        self.assertTrue(HOST.is_file(), "Run build.cmd or set DIAGNOSTIC_BRIDGE_TEST_HOST before the integration tests")
        host = self.host = subprocess.Popen(
            [str(HOST), *arguments], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True, creationflags=subprocess.CREATE_NO_WINDOW,
        )
        self.addCleanup(self.stop_host, host)
        ready = queue.Queue()
        threading.Thread(target=lambda: ready.put(host.stdout.readline()), daemon=True).start()
        self.assertEqual(int(ready.get(timeout=5).strip()), host.pid)
        return host

    def open_pipe(self):
        """Use the public wire interface to reproduce slow and malformed clients."""
        deadline = time.monotonic() + 5
        while True:
            try:
                return open(rf"\\.\pipe\flame-diagnostic-{self.host.pid}", "r+b", buffering=0)
            except OSError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(0.02)

    def stop_host(self, host=None):
        """Cleanup stays bound to its original process, even when a test starts another host."""
        host = self.host if host is None else host
        host.stdin.close()
        try:
            host.wait(timeout=5)
        except subprocess.TimeoutExpired:
            host.kill()
            host.wait(timeout=5)
            self.fail("Native host did not stop within five seconds")
        finally:
            host.stdout.close()
            host.stderr.close()

    def call(self, *command, pid=None):
        result = subprocess.run(
            [sys.executable, str(CLI), "--pid", str(self.host.pid if pid is None else pid), *command],
            capture_output=True, text=True, timeout=8,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(len(result.stdout.splitlines()), 1)
        response = json.loads(result.stdout)
        self.assertIs(response["ok"], True)
        return response

    def test_all_named_snapshots_cross_the_native_pipe(self):
        for name in ("session", "player", "hand", "possession"):
            with self.subTest(snapshot=name):
                response = self.call("snapshot", name)
                self.assertEqual(response["snapshot"], name)
                self.assertIsInstance(response["data"], dict)

    def test_trace_is_opt_in_and_can_be_drained_without_streaming(self):
        initial = self.call("trace", "drain")
        self.assertEqual(initial["events"], [])
        self.assertEqual(initial["dropped"], 0)
        self.assertIs(self.call("trace", "on")["trace"], True)
        self.assertIs(self.call("trace", "off")["trace"], False)
        self.assertIn("events", self.call("trace", "drain"))

    def test_recorder_captures_two_pids_and_disables_tracing(self):
        """Exercise the recorder against isolated real pipes and verify its cleanup contract."""
        first = self.host
        second = self.start_host()
        pids = {first.pid, second.pid}
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "capture.jsonl"
            result = subprocess.run(
                [sys.executable, str(FLAME / "tools" / "diagnostic_record.py"),
                 "--pid", str(first.pid), "--pid", str(second.pid),
                 "--output", str(output), "--seconds", "0.5"],
                capture_output=True, text=True, timeout=12,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(result.stderr, "")
            rows = [json.loads(line) for line in output.read_text(encoding="utf-8").splitlines()]
        self.assertEqual({row["pid"] for row in rows}, pids)
        for pid in pids:
            with self.subTest(pid=pid):
                samples = [row for row in rows if row["pid"] == pid and "session" in row]
                self.assertTrue(samples)
                for name in ("session", "player", "hand", "possession"):
                    self.assertTrue(all(row[name]["ok"] for row in samples))
                    self.assertTrue(all(row[name]["snapshot"] == name for row in samples))
                self.assertTrue(any(row.get("actions", {}).get("events") for row in rows if row["pid"] == pid))
                stopped = [row for row in rows if row["pid"] == pid and "trace_off" in row]
                self.assertEqual(len(stopped), 1)
                self.assertIs(stopped[0]["trace_off"]["trace"], False)
                # The synthetic host records every tick: no new events proves trace stayed off.
                self.assertEqual(self.call("trace", "drain", pid=pid)["events"], [])

    def test_temporary_debug_logging_commands_are_registered(self):
        self.call("logging", "debug", "on")
        self.assertIs(self.call("snapshot", "session")["data"]["debug_logging"], True)
        self.call("logging", "debug", "off")
        self.assertIs(self.call("snapshot", "session")["data"]["debug_logging"], False)

    def test_response_survives_a_client_delaying_its_read(self):
        with self.open_pipe() as pipe:
            pipe.write(b"snapshot session\n")
            time.sleep(0.15)
            response = json.loads(pipe.read(4096))
        self.assertIs(response["ok"], True)
        self.assertEqual(response["snapshot"], "session")

    def test_native_registry_rejects_commands_bypassing_cli_validation(self):
        with self.open_pipe() as pipe:
            pipe.write(b"snapshot memory 0x1234\n")
            response = json.loads(pipe.read(4096))
        self.assertEqual(response, {"ok": False, "error": "unknown_command"})
        self.call("snapshot", "session")

    def test_oversized_request_is_rejected_without_poisoning_next_request(self):
        with self.open_pipe() as pipe:
            pipe.write(b"x" * 128 + b"\n")
            response = json.loads(pipe.read(4096))
        self.assertEqual(response, {"ok": False, "error": "request_too_large"})
        self.call("snapshot", "session")

    def test_incomplete_disconnected_client_does_not_block_next_request(self):
        with self.open_pipe() as pipe:
            pipe.write(b"snapshot ses")
        self.call("snapshot", "session")

    def test_host_can_stop_with_an_incomplete_connected_request(self):
        with self.open_pipe() as pipe:
            pipe.write(b"snapshot ses")
            self.host.stdin.close()
            self.assertEqual(self.host.wait(timeout=5), 0)

    def test_unresponsive_game_thread_returns_bounded_error(self):
        self.stop_host()
        self.start_host("--no-pump")
        result = subprocess.run(
            [sys.executable, str(CLI), "--pid", str(self.host.pid), "snapshot", "session"],
            capture_output=True, text=True, timeout=5,
        )
        self.assertEqual(result.returncode, 1)
        self.assertEqual(json.loads(result.stdout), {"ok": False, "error": "game_thread_timeout"})


if __name__ == "__main__":
    unittest.main()
