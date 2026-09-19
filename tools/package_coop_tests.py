"""Check complete, private-file-free release payloads and safe upgrade extraction."""
import hashlib
import json
from pathlib import Path
import shutil
import unittest
from unittest.mock import patch
import uuid
import zipfile

from package_coop import package, directx_payload, DXSDK_NAME


class PackageTests(unittest.TestCase):
    """Use isolated install fixtures; never read or modify a real DK2 installation."""

    def setUp(self):
        self.build = Path(__file__).resolve().parents[1] / "build"
        self.build.mkdir(exist_ok=True)
        # Keep fixtures under the ignored build tree and verify cleanup stays there.
        self.root = (self.build / f"package-test-{uuid.uuid4().hex}").resolve()
        self.root.mkdir()
        self.assertEqual(self.root.parent, self.build.resolve())
        # Unit fixtures stay offline; the actual package build verifies Microsoft's pinned download.
        mock_runtime = patch("package_coop.directx_payload", return_value={
            "D3DX9_43.dll": b"x86 d3dx runtime",
            "D3DCompiler_43.dll": b"x86 compiler runtime",
            "flame/Microsoft-D3DX-LICENSE.txt": b"Microsoft runtime terms",
        })
        mock_runtime.start()
        self.addCleanup(mock_runtime.stop)
        inputs = {
            "install/PATCH.dll": b"loader",
            "install/flame/Flame.dll": b"runtime",
            "install/flame/DKII.dll": b"stub",
            "install/readme.txt": b"upstream attribution",
            "resources/coop/README.txt": b"install instructions",
            "resources/PatchEditorByQuuz/Graphics/example.bmp": b"editor",
            "install/Data/editor/Graphics/example.bmp": b"editor",
            "build/vs2026-debug/artifact_name-Debug.txt": b"DK2-Coop-1.7.0-debug\n",
            # These must never leak from a reused installation output directory.
            "install/flame/config.toml": b"private settings",
            "install/flame/coop.toml": b"personal coop settings",
            "install/flame/Flame.pdb": b"symbols",
            "install/flame/latest.log": b"private log",
            "install/Data/Save/Flame-GameProgress.txt": b"personal progress",
            "install/Data/editor/unrelated.jsonl": b"capture",
        }
        for name, data in inputs.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)

    def tearDown(self):
        # Validate the resolved recursive cleanup target before removing the fixture.
        if self.root.resolve().parent != self.build.resolve():
            raise AssertionError("Fixture escaped the build directory")
        shutil.rmtree(self.root)

    def test_complete_package_preserves_existing_settings_and_progress(self):
        target = package(self.root, "Debug")
        with zipfile.ZipFile(target) as archive:
            self.assertEqual(set(archive.namelist()), {
                "PATCH.dll", "flame/Flame.dll", "flame/DKII.dll",
                "Flame-Upstream-README.txt", "DK2-Coop-README.txt",
                "flame/coop-build.json",
                "Data/editor/Graphics/example.bmp",
                "D3DX9_43.dll", "D3DCompiler_43.dll", "flame/Microsoft-D3DX-LICENSE.txt",
            })
            manifest = json.loads(archive.read("flame/coop-build.json"))
            self.assertEqual(manifest["product"], "DK2 Co-op")
            self.assertEqual(manifest["repository"], "https://github.com/ciphercom/dk2-coop")
            for name, digest in manifest["files"].items():
                self.assertEqual(hashlib.sha256(archive.read(name)).hexdigest(), digest)
            for installed in (False, True):
                game = self.root / ("existing" if installed else "clean")
                personal = ["flame/config.toml", "flame/coop.toml", "Data/Save/Flame-GameProgress.txt"]
                if installed:
                    for name in personal:
                        path = game / name
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_bytes(b"keep me")
                archive.extractall(game)
                self.assertEqual((game / "flame/Flame.dll").read_bytes(), b"runtime")
                if installed:
                    for name in personal:
                        self.assertEqual((game / name).read_bytes(), b"keep me")
        digest = hashlib.sha256(target.read_bytes()).hexdigest()
        self.assertEqual(target.with_suffix(".zip.sha256").read_text(), f"{digest}  {target.name}\n")

    def test_repeatable_package_and_distinct_changed_build(self):
        first = package(self.root, "Debug")
        original = first.read_bytes()
        self.assertEqual(package(self.root, "Debug").read_bytes(), original)
        (self.root / "install/flame/Flame.dll").write_bytes(b"changed runtime")
        self.assertNotEqual(package(self.root, "Debug").name, first.name)
        self.assertEqual(first.read_bytes(), original)

    def test_incomplete_install_does_not_publish_an_archive(self):
        (self.root / "install/flame/DKII.dll").unlink()
        with self.assertRaises(FileNotFoundError):
            package(self.root, "Debug")
        self.assertFalse((self.root / "build/releases").exists())

    def test_release_uses_separate_install_and_manifest(self):
        """A Release package must not accidentally ship the existing Debug DLLs."""
        shutil.copytree(self.root / "install", self.root / "install-release")
        (self.root / "install-release/flame/Flame.dll").write_bytes(b"optimized runtime")
        artifact = self.root / "build/vs2026-release/artifact_name-Release.txt"
        artifact.parent.mkdir()
        artifact.write_text("DK2-Coop-1.7.0\n")
        with zipfile.ZipFile(package(self.root)) as archive:
            self.assertEqual(archive.read("flame/Flame.dll"), b"optimized runtime")
            manifest = json.loads(archive.read("flame/coop-build.json"))
            self.assertEqual(manifest["configuration"], "Win32 Release, static C++ runtime")
            self.assertEqual(manifest["build"], "DK2-Coop-1.7.0")

    def test_selected_generator_directory_supplies_artifact_identity(self):
        """CI's VS2022 build must use its own configured identity."""
        build_directory = self.root / "build/vs2022-debug"
        build_directory.mkdir()
        (build_directory / "artifact_name-Debug.txt").write_text("DK2-Coop-1.7.1-debug\n")
        target = package(self.root, "Debug", build_directory)
        self.assertTrue(target.name.startswith("DK2-Coop-1.7.1-debug-"))

    def test_modified_dependency_is_rejected(self):
        (self.root / "build" / DXSDK_NAME).write_bytes(b"untrusted replacement")
        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            directx_payload(self.root)


if __name__ == "__main__":
    unittest.main()
