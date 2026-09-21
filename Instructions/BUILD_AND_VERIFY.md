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
- Keep simulated transport and placeholders distinguishable from working audio.

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
