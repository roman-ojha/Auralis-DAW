# Build, validation, and completion

## Windows development baseline

- Windows x64, Visual Studio 2022 C++ desktop tools and Windows SDK.
- C++20 and CMake 3.22 or newer; prefer a stable CMake release when updating tools.
- CMakeLists.txt is the authority for source lists, versions, defines, and links.
- The current JUCE revision is pinned there. A local `external/JUCE` checkout is
  used if present; otherwise FetchContent retrieves the configured revision.
- Run commands from the repository root. Preserve the user's workspace folder
  name even though it still contains the earlier product name.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
& '.\build\Auralis_artefacts\Release\Auralis.exe'
```

For assertions/debugging, build and test with `--config Debug` / `-C Debug`.
`AuralisEditorTests` links JUCE GUI and drives editor handlers without opening a
window. It checks mouse/keyboard editing against document state; it supplements,
rather than replaces, native rendering and input verification.
For the compact-window check, launch the executable with `--compact`.
Generated IDE solution: `build/Auralis.sln`.

Do not install or upgrade tools solely because newer versions exist. If a command
fails because of sandbox restrictions, use the supported approval mechanism and
explain the blocked action. Do not disable protections or work around permissions.
Before linking over a running binary, close only this application's test instance
through a normal exit, preserving any user data. Do not terminate other DAWs.

## Choose checks by change

| Change | Required evidence |
| --- | --- |
| Documentation only | Read changes; check paths/links/commands and consistency; no needless app rebuild |
| UI layout or control | Build; exercise changed controls; inspect real rendering at normal and compact sizes; test affected divider/scroll/focus behavior |
| State or data handling | Build; deterministic behavior tests including applicable invalid/boundary inputs; relevant UI integration check |
| Defect fix | Reproduce or identify the failing condition; verify the correction; add a meaningful regression test when practical |
| Dependency or build system | Configure/build/test with the changed setup; inspect pin, license, target/source registration and distribution impact |
| Audio/MIDI/plugins | Relevant tests above plus the checks in AUDIO_AND_PLUGINS.md |

Use targeted tests first and existing regression checks for affected behavior.
Broaden testing for unresolved risk, not ritual. Do not remove an assertion, skip
a failure, or weaken a tolerance merely to produce a passing result.

## UI review checklist

- Inspect the running native application, not only source or a mock image.
- Check the default 1440 x 900 and compact 1100 x 700 logical content sizes;
  OS window decorations add to screenshot dimensions.
- Exercise relevant divider minima/maxima, reset behavior, scrolling, selection,
  empty/search-no-result states, and category changes.
- Check labels, contrast, clipping, focus visibility, and meaningful accessibility
  names/values. Test keyboard behavior for changed controls.
- Check scaling/DPI behavior when touched; do not claim cross-DPI validation
  without actually exercising it.
- Use available desktop automation with its documented safety rules. If automation
  cannot exercise a behavior, report that limit and give concise manual steps.
- Keep unsupported record/metronome actions distinguishable from working playback.

## Completion and handoff

Before finishing:

1. Review the final source/diff for accidental scope changes, generated files,
   secrets, hardcoded machine paths, stale names, and ownership/lifetime defects.
2. Confirm the final build succeeds and relevant checks pass. Distinguish warnings
   in changed code from pre-existing tool/dependency warnings; do not suppress
   warnings globally to conceal a problem.
3. Inspect affected UI or behavior. A running process is not proof a control works.
4. Update README and instruction/state files only where the behavior or workflow
   changed. Record validation scope, build mode, platform, and limitations in docs.
   Old verification notes are historical evidence, not proof of the current build.
   Update affected FEATURE_MAP entries and Info View help for every implementation
   task. Verify that new controls explain their actual behavior and limitations.
5. Report the result, what was tested, and any unverified or unfinished aspects.
   If manual help is necessary, give exact steps and explain the blocking reason.

Do not commit, publish, distribute, or create a release unless that is part of
the user's request. A future release requires its own clean-build, packaging,
dependency/license, recovery, compatibility, and installation checks appropriate
to the features being shipped; a local UI smoke test is not a release qualification.

Audio callback verification is built into `AuralisAudioOutputTests`; it exercises
rendering without opening a device, including variable blocks, sample-count clock,
loop overshoot, pause, output bounds and concurrent immutable plan replacement.
Its timing printout is an offline workload measurement, not hardware latency or a
real-time certification. `AuralisAudioTests` covers source-phase/copy/history and
sample values. Native audio checks must also exercise folder persistence, file
selection/replay, drag import, lower editor, fades, stop and clean shutdown.

## Project and hosting milestone

`AuralisProjectTests` covers .aup round trips, embedded PCM checksums, failure
preservation, missing-plugin state, automation curves/ranges and actual FFT input.
`AuralisHostTests` invokes the scanner helper's verification entry point against
repository-owned `AuralisHostFixture.vst3`; it verifies scan, stereo processing,
stable parameters, variable blocks and plugin-state restoration. Build all targets
before CTest so the fixture exists. It is not installed or added to user favorites.
Keep `AuralisPluginScanner.exe` beside the normal app when copying a local build.
Use native QA for editor windows, compact layout and real file-dialog workflows;
a fixture pass does not establish commercial-plugin compatibility or PDC.


## Current regression suite map

| CTest name | Main coverage |
| --- | --- |
| AuralisStateTests | Transport and bounded layout state |
| AuralisMixerTests | Shared channels, route validation and parameter boundaries |
| AuralisMidiTests | Independent clips, repetition, range edits, snap and history |
| AuralisEditorTests | Mouse/keyboard gestures, automation curves/history, wheel zoom/height and restored clip hit order |
| AuralisAudioTests | Sample values, regions/fades, independent copies and history |
| AuralisAudioOutputTests | Graph routing, synth/effects, automation, source end, sample clock and plan lifetime |
| AuralisProjectTests | Serialization, embedded-media CRC, malformed input, failure preservation, native metadata and FFT |
| AuralisHostTests | Owned VST3 fixture scan, processing, parameter IDs, state and teardown |

After persistence/hosting changes, exercise Save/Open with an existing project,
missing-plugin preservation and actual WAV export when affected. Inspect reopened
clip visibility/hit order and a clean initial title. Keep native editor lifecycle
checks separate from host-fixture DSP assertions. Preserve unsaved user changes
before replacing a running executable; saving is now implemented, so do not use
old instructions that assume every session must be discarded.

After rack changes, inspect horizontal chain scrolling and internal parameter
scrolling at both QA sizes. The rack is fixed-height; the audio editor is resizable.
After automation changes, check lane capture, grouping, bypass and playback while
hidden, plus save/reopen. Record exact scenarios and unverified limits.

Latest recorded full verification: 2026-09-28; see [VALIDATION](../docs/VALIDATION.md).
The 2026-09-29 documentation reconciliation does not constitute a new build/test run.
