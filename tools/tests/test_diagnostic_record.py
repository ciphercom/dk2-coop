"""Check that manual-play captures preserve evidence and always release tracing."""
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import diagnostic_record as recorder


class RecorderTests(unittest.TestCase):
    def test_capture_preserves_both_peer_ids_overflow_and_shutdown(self):
        """A bounded capture preserves dropped-event counts and keeps peer records separate."""
        def respond(pid, command):
            return {"ok": True, "data": {"pid": pid}, "dropped": 7 if command == "trace drain" else 0}
        output = io.StringIO()
        with patch.object(recorder, "query", side_effect=respond) as query, \
             patch.object(recorder.time, "monotonic", side_effect=[0, 0, 1, 1]):
            self.assertEqual(recorder.record([4, 8], output, 0.5), 0)
        rows = [json.loads(line) for line in output.getvalue().splitlines()]
        samples = [row for row in rows if "session" in row]
        self.assertEqual({row["pid"] for row in samples}, {4, 8})
        self.assertTrue(all(row["actions"]["dropped"] == 7 for row in samples))
        for pid in (4, 8):
            query.assert_any_call(pid, "trace off")
        self.assertEqual({row["pid"] for row in rows if "trace_off" in row}, {4, 8})

    def test_disconnected_process_is_recorded_and_exits_unsuccessfully(self):
        output = io.StringIO()
        with patch.object(recorder, "request", side_effect=recorder.BridgeError("bridge_unavailable")):
            self.assertEqual(recorder.record([4], output, 1), 1)
        rows = [json.loads(line) for line in output.getvalue().splitlines()]
        self.assertEqual(rows[1]["session"]["error"], "bridge_unavailable")
        self.assertIn("trace_off", rows[-1])

    def test_output_failure_still_stops_every_peer(self):
        output = io.StringIO()
        with patch.object(recorder, "query", return_value={"ok": True}) as query, \
             patch.object(output, "write", side_effect=OSError("disk full")):
            with self.assertRaises(OSError):
                recorder.record([4, 8], output, 1)
        for pid in (4, 8):
            query.assert_any_call(pid, "trace off")

    def test_interruption_still_stops_and_drains_tracing(self):
        output = io.StringIO()
        with patch.object(recorder, "query", return_value={"ok": True}) as query, \
             patch.object(recorder, "sample", side_effect=KeyboardInterrupt):
            self.assertEqual(recorder.record([4], output, 1), 0)
        self.assertEqual([call.args[1] for call in query.call_args_list],
                         ["trace on", "trace off", "trace drain"])


if __name__ == "__main__":
    unittest.main()
