"""Verify the diagnostic CLI's public commands and bounded failure behavior."""

import json
from pathlib import Path
import subprocess
import sys
import unittest


CLI = Path(__file__).resolve().parents[1] / "diagnostic_bridge.py"


def run_cli(*arguments):
    """Exercise the real CLI as a caller, including its output and exit status."""
    return subprocess.run(
        [sys.executable, str(CLI), *arguments],
        capture_output=True, text=True, timeout=8,
    )


class DiagnosticCliTests(unittest.TestCase):
    """Invalid targets/operations must never become arbitrary pipe commands."""

    def test_help_lists_only_named_diagnostics(self):
        result = run_cli("--help")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("snapshot", result.stdout)
        self.assertIn("trace", result.stdout)

    def test_missing_bridge_returns_one_json_error_and_failure_exit(self):
        # This cannot identify a live Windows process: real PIDs are multiples of 4.
        result = run_cli("--pid", "4294967295", "snapshot", "session")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(len(result.stdout.splitlines()), 1)
        response = json.loads(result.stdout)
        self.assertIs(response["ok"], False)
        self.assertEqual(response["error"], "bridge_unavailable")
        self.assertEqual(result.stderr, "")

    def test_pid_must_be_positive_windows_process_id(self):
        for pid in ("0", "-1", "4294967296", "other-machine"):
            with self.subTest(pid=pid):
                result = run_cli("--pid", pid, "snapshot", "session")
                self.assertEqual(result.returncode, 2)
                self.assertIn("PID must be a positive Windows process ID", result.stderr)

    def test_raw_memory_and_action_commands_are_rejected(self):
        for command in (("memory", "0x1234"), ("action", "62"),
                        ("snapshot", "session\ntrace on"), ("trace", "forever"),
                        ("logging", "memory", "on")):
            with self.subTest(command=command):
                result = run_cli("--pid", "4", *command)
                self.assertEqual(result.returncode, 2)
                self.assertIn("invalid choice", result.stderr)


if __name__ == "__main__":
    unittest.main()
