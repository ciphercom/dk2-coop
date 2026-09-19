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

The package contains `PATCH.dll`, `flame/Flame.dll`, `flame/DKII.dll`, editor patches, the pinned x86 Microsoft D3DX libraries/license, `DK2-Coop-README.txt`, upstream attribution and `flame/coop-build.json`. The manifest records the co-op version, upstream Flame version, build information and payload hashes. Install the same package on both PCs using the repository README instructions.

Tagged builds publish the Release ZIP and checksum to [this fork's releases](https://github.com/ciphercom/dk2-coop/releases/latest).

## Versioning

The release identifier is `<Flame x.y.z>.<co-op revision>`, starting at `1.7.0.1`. The fourth number increases for every co-op release and **never resets**, including when the Flame version changes: `1.7.0.1`, `1.7.0.2`, `1.8.0.3`.

[resources/CMakeLists.txt](../resources/CMakeLists.txt) owns the version. Keep `VER_PRODUCT_NUMBER`, `VER_PRODUCT_VERSION` and `VER_BUILD_NUMBER` aligned with the upstream Flame base. Increment `COOP_REVISION_NUMBER` before each subsequent release. The counter is committed with the code; ordinary builds and workflow reruns do not increment it.

The game version display, loader/runtime DLL version information and package names use the combined co-op version. `flame/coop-build.json` also records the original Flame version separately. The ZIP's hash suffix distinguishes exact build payloads within a version.

## GitHub workflow

The [build workflow](../.github/workflows/CI.yml) runs `build.cmd` on Windows with Visual Studio 2022 for Release and Debug. Pull requests targeting `main` and manual runs provide downloadable workflow artifacts. Ordinary branch pushes do not trigger game builds. Pushing a version tag publishes a GitHub release automatically after **both** configurations pass, attaches the Release ZIP and SHA-256 checksum, generates release notes and marks it **Latest**. Debug packages remain available as workflow artifacts.

To release:

1. Set the next `COOP_REVISION_NUMBER` and commit the release changes. The initial release uses `1`.
2. Push the commit and its matching tag. For the initial version:

   ```powershell
   git push origin main
   git tag -a v1.7.0.1 -m "DK2 Co-op 1.7.0.1"
   git push origin v1.7.0.1
   ```

3. Follow **Actions -> Build DK2 Co-op**. Once it succeeds, the package appears under [Latest release](https://github.com/ciphercom/dk2-coop/releases/latest).

The tag must exactly match the packaged version. The publication check rejects missing or ambiguous packages, Debug packages and incorrect checksums before uploading release assets. Reuse a failed workflow's rerun action to retry a build; publish changed code under a new revision and tag. Existing published releases are not overwritten. Manual workflow runs build artifacts without publishing.

Publication uses the repository's built-in `GITHUB_TOKEN` with write access limited to the release job; no personal token is needed. The workflow uses [GitHub CLI's release command](https://cli.github.com/manual/gh_release_create) to publish the tagged build.
