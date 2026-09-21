# UI milestone validation

Validated on Windows 11 / Visual Studio 2022 x64 Release.

## Automated

`ctest --test-dir build -C Release --output-on-failure` passes the state regression executable:

- stopped and paused clocks do not advance;
- bar timing at 120 BPM in 4/4 and 6/8;
- four-bar looping preserves elapsed overshoot;
- invalid timer input is ignored;
- tempo clamps to supported bounds;
- stop resets the playhead and record-armed state;
- extreme divider positions at 1100/1440/1920 by 700/900/1080 preserve usable panels.

## Interactive desktop checks

- Started the native application and inspected its rendered UI and accessibility tree.
- Search by keyboard plus the Effects category returned the matching Bloom placeholder.
- Play advanced the timer/playhead; Pause retained position; Escape returned to zero.
- Selecting the Audio track updated the device-area heading.
- Gain set to -12 dB updated both the slider and numeric readout.
- Time signature selection changed to 6/8.
- Browser, track-header, and device-area dividers resized their panels.
- Reset layout restored all three dividers.
- Inspected both default 1440 x 900 and compact 1100 x 700 content sizes.
- Compact layout retained top controls and supplied vertical scrolling for track lanes.

For repeatable compact-window inspection, launch `Auralis.exe --compact`.
This switch also resizes an already-running instance. Window sizes here refer to
JUCE content sizes; native OS decorations add to the captured outer dimensions.

This milestone does not validate DSP, latency, audio recording, plugin compatibility,
project persistence, or production accessibility. Those capabilities are not implemented.

## Two-column browser update

- Verified Instruments, Sounds and Effects navigation and each category's groups.
- Collapsing Textures hid its items without affecting the other Sounds groups.
- Searching Effects for b showed only Bloom in Reverb.
- Verified both the outer browser resize and internal category/results divider at
  the minimum browser width; category labels and results remained visible.
- Shared layout regression checks passed with the new 350–500 pixel browser limits.
- Catalog entries remain dummy metadata; no files or audio are loaded.

## 2026-09-21 — Integrated menus, Info View, waveform inspection and Windows identity

- Configured and built Release x64 with VS2022/CMake successfully. Final CTest:
  1/1 passed (transport and bounded layout regression suite).
- Inspected native running UI at 1440x900 and 1100x700 content sizes. Branding and
  global bottom status strips are removed; integrated menus and fixed transport
  remain; Info View and waveform fit without overlapping arrangement/device panels.
- Verified View menu, disabled planned entries, browser minimum-width drag and
  View > Reset Layout restoring the sidebar. Verified Playback loop menu updates
  the toolbar toggle. Native title bar visibly shows the orbit logo; generated RC
  contains executable ICO resources. Separate taskbar/cache behavior not inspected.
- Verified contextual menu/button help, Tab focus moving Play to Pause, library
  item selection and Down-arrow selection updating item-specific Info View text.
  Corrected a discovered focus-context issue where a tree's center row could
  overwrite the selected item's help. Final build rechecked Prism -> Ember.
- Generated a deterministic 2-second mono PCM WAV (44.1 kHz, 440 Hz with amplitude
  envelope) in ignored build output. Chose it through the native file dialog:
  real envelope and 2.00 s / 1 ch rendered. Selecting a placeholder cleared it.
- Chose a deliberately malformed .wav: displayed 'Cannot read this audio file'
  with an empty waveform; application remained responsive. No playback was run.
- Updated root AGENTS, handbook, engineering/build rules, PROJECT_STATE and the
  new FEATURE_MAP register. Added architecture note for help, preview and icons.
- Not exhaustively exercised: AIFF/FLAC, 32-channel/boundary-length files, rapid
  replacement during long decoding, stalled filesystem behavior, every menu item,
  every control's accessibility semantics, cross-DPI rendering or taskbar pin caches.
  Shutdown cooperatively waits for the current decoder read (see decision note).
- Repository Git status was unavailable due to ownership/trust checks, including
  a command-scoped safe.directory attempt. No global Git configuration was changed.
  Source and generated-resource inspection were used; no commit was created.

## 2026-09-21 — Shared mixer and send routing UI

- Release x64 build succeeded using VS2022/CMake. Final CTest passed 2/2:
  transport/layout tests and new shared-channel/routing tests. The latter cover
  stable references, shared source/device ownership, parameter bounds/non-finite
  inputs, route removal, indirect feedback rejection and the 32-send limit.
- Inspected the running native application at 1440x900 and 1100x700 content
  sizes. Icon view switches follow App Resources; the old arrangement
  title/count/zoom toolbar is absent. Rotary gain controls fit arrangement rows.
- Set track 1 gain to -14.1 dB in the mixer and verified that value in arrangement.
  Verified mute state across views. Focused S toggled solo and the mixer displayed
  the same solo ring; Enter activated mute. Final pan/stereo initial readouts
  correctly displayed C and Original. Info View reflected mute/solo state.
- Created two mixer-only sends. Connected track 1 to Send 1, changed its send
  level to 37.5%, then connected Send 1 to Send 2 and normal track 2. Selected
  source cables and destination amount knobs updated; the cycle-producing route
  back to track 1 was disabled. Hidden/reopened send dock preserved routing.
- Compact layout retained view switches and faders; vertical scrolling reached
  routing controls and cables. Normal channels and sends have separate horizontal
  viewports. Master/current meters stayed silent; no fabricated signal was shown.
- Ctrl+right-click is implemented but could not be injected as a combined mouse
  modifier gesture by the available desktop automation API. Manual check: hold
  Ctrl and right-click a track's power icon in either view; only its solo flag
  should toggle, with the same state visible in the other view. Model solo behavior
  and the focused S alternative were verified.
- No audio DSP, actual send summing, pan/stereo processing, solo propagation,
  plugin hosting or project persistence was implemented or tested. No cross-DPI
  or full 32-send interactive stress test was performed. UI state resets on exit.
- Updated README, PROJECT_STATE, FEATURE_MAP and the shared-mixer decision note.
  Git ownership/trust restrictions remained; no global trust settings or commits
  were changed.
