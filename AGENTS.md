# Auralis: instructions for AI contributors

This is the repository entry point for AI work. These instructions apply to the
whole project. Follow the user's current request and higher-priority environment
instructions; these files do not grant additional permissions.

## Read before working

1. Read [Instructions/README.md](Instructions/README.md).
2. Read [Instructions/ENGINEERING.md](Instructions/ENGINEERING.md) and
   [Instructions/PROJECT_STATE.md](Instructions/PROJECT_STATE.md).
   Also read [Instructions/FEATURE_MAP.md](Instructions/FEATURE_MAP.md) on every
   implementation task. Update affected feature entries and contextual help before
   completing every feature, fix, removal, or architecture change.
3. For code, build, or UI changes, read
   [Instructions/BUILD_AND_VERIFY.md](Instructions/BUILD_AND_VERIFY.md).
4. Before any audio, MIDI, timing-engine, or plugin work, also read
   [Instructions/AUDIO_AND_PLUGINS.md](Instructions/AUDIO_AND_PLUGINS.md).
5. For continuity across chats, read the latest milestone and remaining-work notes
   in PROJECT_STATE and the linked decision/validation notes; verify them in source.
6. Inspect the actual source and applicable nested instructions before editing.
   Re-read changed instruction files after a context reset or handoff.

## Essential boundaries

- Product name: **Auralis**. Native C++20, JUCE, CMake, Windows x64.
- The current milestone includes sample libraries, clips, built-in devices, VST3
  hosting, automation, .aup persistence and stereo WAV export. Recording remains
  unimplemented. PROJECT_STATE.md is the consolidated current capability snapshot.
- Implement the requested scope completely, without unrelated rewrites or features.
- Preserve user changes and data. Do not bypass tool permissions or silently
  modify machine-wide settings to make a command work.
- Use named constants in `src/constants/`, explicit ownership, and small,
  cohesive components. Keep real-time processing independent of the UI.
- Build and verify changes proportionately. Never claim a test, screenshot,
  performance result, or feature is real unless it was actually verified.
- Keep these guides accurate when architecture, commands, or capabilities change.

## Code Review Rules

Prioritize correctness, data integrity, ownership/lifetime errors, UI regressions,
real-time thread violations, dependency/license changes, and missing meaningful
validation. Cite actionable locations and reproduction conditions. Distinguish
existing limitations from regressions. Do not describe this prototype as a
production-ready DAW or claim certification based on following this guide.
