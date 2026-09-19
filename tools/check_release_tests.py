"""Exercise the publication gate without making any GitHub requests."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from check_release import check_release


class ReleaseCheckTests(unittest.TestCase):
    """Tags must describe the exact Release payload chosen for publication."""

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.archive = self.root / "DK2-Coop-1.7.0.1-example.zip"
        self.write_package()

    def write_package(self, configuration="Release"):
        with zipfile.ZipFile(self.archive, "w") as archive:
            archive.writestr("flame/coop-build.json", json.dumps({
                "coopVersion": "1.7.0.1",
                "configuration": f"Win32 {configuration}, static C++ runtime",
            }))
        self.checksum = self.archive.with_suffix(".zip.sha256")
        digest = hashlib.sha256(self.archive.read_bytes()).hexdigest()
        self.checksum.write_text(f"{digest}  {self.archive.name}\n")

    def test_matching_release_is_accepted(self):
        self.assertEqual(check_release(self.root, "v1.7.0.1"), self.archive)

    def test_wrong_or_nonrelease_tags_are_rejected(self):
        for tag in ("v1.7.0.2", "v1.8.0.1", "v1.7.0", "v1.7.0.1-rc1", "v1.7.0.0", "v1.7.0.01"):
            with self.subTest(tag=tag), self.assertRaises(ValueError):
                check_release(self.root, tag)

    def test_debug_cannot_be_published(self):
        self.write_package("Debug")
        with self.assertRaisesRegex(ValueError, "Only Release"):
            check_release(self.root, "v1.7.0.1")

    def test_missing_or_ambiguous_archive_is_rejected(self):
        extra = self.root / "old.zip"
        extra.write_bytes(b"old")
        with self.assertRaisesRegex(ValueError, "exactly one"):
            check_release(self.root, "v1.7.0.1")
        extra.unlink()
        self.archive.unlink()
        with self.assertRaisesRegex(ValueError, "exactly one"):
            check_release(self.root, "v1.7.0.1")

    def test_wrong_or_missing_checksum_is_rejected(self):
        self.checksum.write_text("wrong")
        with self.assertRaisesRegex(ValueError, "checksum"):
            check_release(self.root, "v1.7.0.1")
        self.checksum.unlink()
        with self.assertRaises(FileNotFoundError):
            check_release(self.root, "v1.7.0.1")


if __name__ == "__main__":
    unittest.main()
