"""Package the installed DK2 Co-op runtime, never a developer's game directory."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import urllib.request
import zipfile


DXSDK_NAME = "microsoft.dxsdk.d3dx.9.29.952.8.nupkg"
DXSDK_URL = "https://api.nuget.org/v3-flatcontainer/microsoft.dxsdk.d3dx/9.29.952.8/" + DXSDK_NAME
DXSDK_SHA256 = "ead0906ae8a26c18a7525da7490127a2110f7c58f18293738283e30e97c6ea4b"


def directx_payload(flame: Path) -> dict[str, bytes]:
    """Ship Microsoft's pinned x86 app-local runtime, never DLLs from the builder's OS."""
    cache = flame / "build" / DXSDK_NAME
    if cache.exists():
        data = cache.read_bytes()
    else:
        with urllib.request.urlopen(DXSDK_URL, timeout=30) as response:
            data = response.read()
    if hashlib.sha256(data).hexdigest() != DXSDK_SHA256:
        raise ValueError(f"Microsoft D3DX package checksum mismatch: {cache}")
    cache.parent.mkdir(parents=True, exist_ok=True)
    if not cache.exists():
        cache.write_bytes(data)
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        return {
            "D3DX9_43.dll": archive.read("build/native/release/bin/x86/D3DX9_43.dll"),
            "D3DCompiler_43.dll": archive.read("build/native/release/bin/x86/D3DCompiler_43.dll"),
            "flame/Microsoft-D3DX-LICENSE.txt": archive.read("LICENSE.txt"),
        }


def package(flame: Path, configuration: str = "Release", build_directory: Path | None = None) -> Path:
    """One complete ZIP supports clean installs and upgrades without personal files."""
    assert configuration in ("Debug", "Release")
    install = flame / ("install-release" if configuration == "Release" else "install")
    resources = flame / "resources"
    files = {
        name: install / name
        for name in ("PATCH.dll", "flame/Flame.dll", "flame/DKII.dll")
    }
    files.update({
        "Flame-Upstream-README.txt": install / "readme.txt",
        "DK2-Coop-README.txt": resources / "coop/README.txt",
    })
    # Follow the source-owned install inventory, not arbitrary files left in install/.
    editor = resources / "PatchEditorByQuuz"
    editor_files = sorted(p for p in editor.rglob("*") if p.is_file())
    if not editor_files:
        raise ValueError("The bundled Flame editor assets are missing")
    for source in editor_files:
        name = "Data/editor/" + source.relative_to(editor).as_posix()
        files[name] = install / name
    payload = {name: path.read_bytes() for name, path in sorted(files.items())}
    if any(not data for data in payload.values()):
        raise ValueError("Release payload contains an empty file")
    payload.update(directx_payload(flame))

    # Use configured versions so source edits cannot relabel an existing build.
    # Per-file hashes identify the exact shipped payload even for uncommitted builds.
    build_directory = build_directory or flame / f"build/vs2026-{configuration.lower()}"
    metadata = json.loads((build_directory / f"build_metadata-{configuration}.json").read_text())
    artifact = metadata["artifactName"]
    if not artifact.startswith("DK2-Coop-") or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-" for c in artifact):
        raise ValueError("Invalid configured DK2 Co-op artifact name")
    manifest = {
        "product": "DK2 Co-op",
        "repository": "https://github.com/ciphercom/dk2-coop",
        "build": artifact,
        "flameVersion": metadata["flameVersion"],
        "coopVersion": metadata["coopVersion"],
        "configuration": f"Win32 {configuration}, static C++ runtime",
        "directxPackage": {"url": DXSDK_URL, "sha256": DXSDK_SHA256},
        "files": {name: hashlib.sha256(data).hexdigest() for name, data in payload.items()},
    }
    manifest_bytes = (json.dumps(manifest, indent=2) + "\n").encode()
    identity = hashlib.sha256(manifest_bytes).hexdigest()[:12]
    payload["flame/coop-build.json"] = manifest_bytes
    output = flame / "build/releases"
    output.mkdir(parents=True, exist_ok=True)
    target = output / f"{artifact}-{identity}.zip"
    # Construct in a temporary sibling; a failed package must not look publishable.
    temporary = target.with_suffix(".zip.tmp")
    try:
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for name, data in sorted(payload.items()):
                info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, data)
        with zipfile.ZipFile(temporary) as archive:
            if archive.testzip() is not None:
                raise ValueError("Release ZIP failed its integrity check")
        temporary.replace(target)
    finally:
        temporary.unlink(missing_ok=True)
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    target.with_suffix(".zip.sha256").write_text(f"{digest}  {target.name}\n", encoding="ascii")
    return target


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Release")
    parser.add_argument("--build-directory", type=Path, help="Configured CMake build directory")
    args = parser.parse_args()
    print(package(Path(__file__).resolve().parents[1], args.configuration, args.build_directory))
