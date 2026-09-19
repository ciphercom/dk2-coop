"""Reject a mislabeled release before GitHub receives any public assets."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import zipfile


def check_release(directory: Path, tag: str) -> Path:
    """Require one Release ZIP whose embedded version and checksum match the tag."""
    if not re.fullmatch(r"v(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.[1-9][0-9]*", tag):
        raise ValueError("Release tags must use v<Flame x.y.z>.<co-op revision>, e.g. v1.7.0.1")
    archives = list(directory.glob("*.zip"))
    if len(archives) != 1:
        raise ValueError("Expected exactly one Release ZIP")
    archive = archives[0]
    with zipfile.ZipFile(archive) as package:
        manifest = json.loads(package.read("flame/coop-build.json"))
    if manifest["coopVersion"] != tag[1:]:
        raise ValueError("Release tag does not match the packaged co-op version")
    if manifest["configuration"] != "Win32 Release, static C++ runtime":
        raise ValueError("Only Release packages can be published")
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if archive.with_suffix(".zip.sha256").read_text().strip() != f"{digest}  {archive.name}":
        raise ValueError("Release checksum does not match the ZIP")
    return archive


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("tag")
    args = parser.parse_args()
    print(check_release(args.directory, args.tag))
