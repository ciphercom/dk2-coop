# Building and packaging DK2 Co-op

## Requirements

- Windows with Visual Studio 2026 or Visual Studio 2022 and the Desktop development with C++ workload, including x86 tools and a Windows SDK. The script prefers 2026 and falls back to 2022.
- CMake with support for the selected Visual Studio generator. The script uses CMake on PATH or the bundled Visual Studio installation.
- Git and Python 3 on PATH.
- Network access for initial dependency downloads. Configuration creates a local `.venv` and installs `requirements.txt`; packaging downloads Microsoft's pinned D3DX package and verifies its SHA-256 checksum.

Use the repository's `build.cmd` entry point to configure, test and package the standalone build. Build output is staged inside this repository, ready to install into your game folder.

## Commands

From PowerShell in the repository root:

```powershell
.\build.cmd
.\build.cmd --package
.\build.cmd --debug --package
```

Plain `build.cmd` uses Debug. `--package` defaults to Release; `--debug` selects Debug explicitly and `--release` selects Release explicitly. `--debug` and `--release` cannot be combined. The script also works when invoked by absolute path from another working directory.

Each run configures the Win32 build, compiles the project, runs its C++ regression checks and Python native-contract/packaging checks, then installs to `install/` (Debug) or `install-release/` (Release). A failing check stops the build. Assertions remain enabled in Release. Packaging follows only after those steps pass.

## Release artifacts

`build.cmd --package` writes the complete distributable ZIP and its `.zip.sha256` sidecar under `build/releases/`. Names use `DK2-Coop-<version>-<identity>.zip`; Debug packages add `-debug`. The identity identifies the exact payload, including the instructions and bundled editor assets.

The package contains `PATCH.dll`, `flame/Flame.dll`, `flame/DKII.dll`, editor patches, the pinned x86 Microsoft D3DX libraries/license, `DK2-Coop-README.txt`, upstream attribution and `flame/coop-build.json`. The manifest records build information and payload hashes. Install the same package on both PCs using the repository README instructions.

Inspect the package and manifest, prepare release notes describing the changes, and publish the ZIP and checksum to [this fork's releases](https://github.com/ciphercom/dk2-coop/releases).

## GitHub workflow

The build workflow runs the same entry point on Windows with Visual Studio 2022 for Release and Debug. Its artifact downloads contain the ZIP and checksum. Pushing a version tag beginning with v prepares a draft GitHub release with the Release package and checksum; a maintainer reviews the notes and publishes the draft.
