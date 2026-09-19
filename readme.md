# DK2 Co-op

DK2 Co-op adds **Shared Keeper co-op** to Dungeon Keeper 2: two people control one Keeper, sharing the dungeon, creatures, resources and original campaign. This is the co-op fork of [Flame](https://github.com/DiaLight/Flame), retaining its game fixes and native multiplayer foundation.

The original campaign uses DK2's existing mission map and unlock rules. The host selects an unlocked mission and retains campaign progression; the joining player does not need the same unlocks. Each Controller has separate hand contents within the shared native hand capacity, independent camera control, and access to the Keeper's actions. Only one Controller can possess a creature at a time. Scripted cutscenes and mission endings are shared.

Build your dungeon together, explore with independent cameras and take turns possessing creatures. See [playing the co-op campaign](docs/coop.md) for the session flow.

## Install and play

1. Both people need their own Dungeon Keeper 2 v1.70 installation containing `DKII-DX.exe`, with matching game data.
2. Download the same `DK2-Coop-*.zip` from [this fork's releases](https://github.com/ciphercom/dk2-coop/releases).
3. Close the game and back up any existing `PATCH.dll`, `flame/` and `Data/editor/`. Extract the ZIP into the game folder beside `DKII-DX.exe`, accepting replacement.
4. Start `DKII-DX.exe` normally. Choose **Multiplayer -> Co-op Campaign** on both computers and use the native TCP/IP connection flow with distinct player names.
5. The host creates a session and selects an unlocked mission through the campaign map. The other person joins; both confirm readiness and start together.

The package is ready to install directly into your game folder. Start through `DKII-DX.exe` and keep personal settings in `flame/config.toml`. It includes the runtime, editor patches by Quuz and the required x86 D3DX libraries with their Microsoft license. Use the same DK2 Co-op build, gameplay-affecting settings and resource packs on both computers.

Let shared cutscenes finish, then continue playing together. After a mission, return through the result screen to create and join a fresh co-op session for the next unlocked mission. To choose a different mission, create a new lobby.

## Report problems

Use [DK2 Co-op issues](https://github.com/ciphercom/dk2-coop/issues). Include the package filename or build identity from `flame/coop-build.json`, the mission, what each person was doing, what happened and what you expected. Report unrelated problems separately. This fork's co-op issues belong here rather than in the upstream Flame tracker.

## Build and release

Use `build.cmd` from this repository; it configures, builds, runs the project checks and installs the result into a local staging folder. It never copies files into game installations. A Windows C++ toolchain, CMake, Git and Python are required; see [the build and release guide](docs/build.md).

```powershell
.\build.cmd                  # Debug build, checks, install/
.\build.cmd --package        # Release build, checks, install-release/, ZIP
.\build.cmd --debug --package # Explicit Debug package
```

Packages and SHA-256 sidecars are written to `build/releases/`.

## Upstream and credits

[Flame by DiaLight and contributors](https://github.com/DiaLight/Flame) provides the partial DK2 recompilation, patch loader and game fixes on which this fork is built. Its upstream authorship and source history are retained. Internal names such as `PATCH.dll`, `flame/Flame.dll` and `flame/DKII.dll` remain compatible with that loader; they are also the names used in this fork's packages. The original packaged attribution is retained as `Flame-Upstream-README.txt`. Quuz created the bundled level-editor patches in `Data/editor/`.

Flame loads replacement functions through its DLL rather than redistributing the original game executable. Its earlier [executable-merging](https://github.com/DiaLight/Flame/tree/93e04efaba41bb3a574b33a0a8d91d2f63d4b31d) and [full-relinking](https://github.com/DiaLight/Flame/tree/46e5b0c1df93060bd01a83bb6d14d064e9c8c3dc) approaches remain documented in upstream history.

The promotional website lives in `website/`; its publishing instructions are in [docs/website.md](docs/website.md).
