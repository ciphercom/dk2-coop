# Internal port validation

Engineering record for the DK2 Co-op port. This document tracks evidence, pending acceptance and scope decisions for maintainers.

## Inherited gameplay evidence

The source project's playtests confirmed matching Level1 lobby labels, a displayed two-Controller capacity and working host campaign progression. Source/native-code audits and regression checks cover broader mission routing and shared-world adaptations. Campaign-wide completion has not been verified; this evidence does not establish every original mission, branch, secret unlock, third-connection rejection or repeated live session.

## Pending live acceptance

- When one Controller possesses a creature as an authored cutscene starts, both should see the cutscene with consistent timing. Valid possession should return afterward with working input; death/release should return to Keeper view. Replay the reported transition to check that no resynchronization occurs. Native-code causes and regression contracts support the included correction; live visual/audio/two-PC acceptance remains pending.
- While one Controller possesses, the other in Keeper view should retain normal creature health flowers at normal size. The possessing Controller should retain native presentation. Related object indicators were outside the narrow correction. Live visual acceptance remains pending.
- After debriefing, both peers should see the Co-op Campaign browser. A new host Create should open the campaign map with current unlocks and create a fresh lobby/readiness state. Live acceptance of this transition remains pending.

## Scope decisions

Client persistent progression, automatic next-mission continuation, co-op save/resume and cutscene skipping are outside the current implementation scope. Pause, speed, disconnect handling and recovery retain the native multiplayer baseline. The route distinguishes co-op mission sessions from ordinary sessions; it does not implement full content compatibility negotiation. More than two Controllers, new transport, matchmaking and host migration are excluded.

The gameplay files were ported without launching the game. Static frontend review checked eight menu/lobby files against the source and confirmed their route integrations. Documentation relative links resolve and the scoped whitespace check passes. Build and package validation is recorded separately below by the port integrator.

## Standalone fork verification — 2026-09-19

Source workspace revision: `ab5594d` (co-op implementation through always-available menu and Release packaging). Fork base: `1da7489` (upstream Flame plus website). Verification used the working tree without creating a commit or altering the user's index.

`build.cmd --package` and `build.cmd --debug --package` both exited 0 with Visual Studio 2026, Win32. Each ran 13 C++ regression executables, six native Hand checks, one native possession-panel check and six packaging tests. A final Release run passed after the documentation update; Debug was repackaged from its verified installed DLLs with the same final instructions.

Independent review found no port blockers. Compared gameplay state changes against the source, including calls previously inside diagnostic conditionals. Shared camera completion, ending rewind/completion and possession world hooks remain present. The diagnostic bridge, traces, testing unlock override and local profiles are excluded. The upstream optional OOS/protocol logging remains unchanged and disabled by default.

Both final archives passed ZIP integrity, complete manifest membership, per-file SHA-256, sidecar checksum, installed-runtime equality and final-instructions equality checks. Their runtime contains the fork's title and no added bridge, availability override or gameplay trace strings. Packages contain 12 files each; no game executables, settings, saves, logs, captures or PDBs.

| Configuration | Archive in `build/releases/` | SHA-256 |
| --- | --- | --- |
| Release | `DK2-Coop-1.7.0-07f6482f27af.zip` | `eff65c8fb7e71ccedd5dbe2123fcb3bd4122152e898844287c788a1b4c135a3c` |
| Debug | `DK2-Coop-1.7.0-debug-6dcace55978c.zip` | `6514ac66fbf9e4a5345e464eea681ad84699ff599bfdca54a915aad2483861f7` |

The GitHub workflow and VS2022 fallback were reviewed but not executed here. No game was launched, local game installation modified, release published or remote repository changed during this port.

### Existing upstream build warnings

The following warnings occurred in the clean Release and Debug builds. They are inherited from existing Flame code; they were not suppressed or treated as evidence of a warning-free build. The DirectPlay uninitialized-variable warning remains an engineering issue in the inherited service provider.

- `LINK : warning LNK4217: symbol '?nameList@dk2@@3PAUNameCfg@1@A (struct dk2::NameCfg * dk2::nameList)' defined in 'CGuiManager.obj' is imported by 'CFrontEndComponent.obj' in function '"public: struct dk2::NameCfg * __thiscall dk2::CFrontEndComponent::sub_535A30(void)" (?sub_535A30@CFrontEndComponent@dk2@@QAEPAUNameCfg@2@XZ)'`
- `src\dk2\button\button_functions.cpp(1110,13): warning C4477: 'swprintf' : format string '%s' requires an argument of type 'wchar_t *', but variadic argument 1 has type 'char *'`
- `src\dk2\button\button_functions.cpp(1110,13): warning C4477: 'swprintf' : format string '%s' requires an argument of type 'wchar_t *', but variadic argument 4 has type 'CHAR *'`
- `src\dk2\gui\CGuiManager.cpp(15,18): warning C4273: 'dk2::nameList': inconsistent dll linkage`
- `src\patches\use_wheel_to_zoom.cpp(33,14): warning C4644: usage of the macro-based offsetof pattern in constant expressions is non-standard; use offsetof defined in the C++ standard library instead`
- `src\tools\StackLimits.cpp(51,16): warning C4477: 'printf' : format string '%08X' requires an argument of type 'unsigned int', but variadic argument 1 has type 'PVOID'`
- `src\weanetr_dll\DPlay.cpp(1157,1): warning C4700: uninitialized local variable 'f3C_flags' used`

Two existing narrow-string `swprintf` warnings in the touched game component were corrected using `%hs`. Other inherited warnings above remain outside the co-op port changes.

### Cleanup

Build logs were summarized here and removed after verification, together with the two superseded package candidates and their checksum files. Final packages and build outputs are retained. Original log hashes:

- `port-release.log`: `89c71aa784354b07d0ed67bf14f8b36798d3cdbd372773c6b13ce31476f4d24b`
- `port-debug.log`: `c3f197374b4be11c01fdf8a7c10925fc293fe91ff049804d183b89e1381c6763`
- `port-release-final.log`: `12b6bcb26bbf1b8ce17d3c7b144a3f05c0636a65235c05eeb8d6e3b2021f773b`
