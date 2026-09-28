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

## 2026-09-27 - Interactive piano roll and clip creation

- Windows x64 Release build succeeded. CTest passed 4/4, including the new JUCE
  editor-handler suite: both-button clip creation, audio-lane rejection, note
  creation/movement, modifier selection, deletion/undo, cloning, swept erase/mute,
  slicing, note presets, unique identities and adaptive/tick-precision snap.
- Inspected native rendering at 1440x900 and 1100x700 content sizes. The shaped
  keyboard, full-height editor, vector tool strip and control lane fit both sizes.
  Left double-click created and opened clips on instrument tracks 1 and 2.
  Drew a note, resized its right edge, switched back to arrangement, and edited
  its velocity stem. Verified Ctrl+B duplication and Ctrl+Z undo in the app.
- Native input testing exposed control-character key events without modifiers;
  EditorKey now normalizes that input path, covered by a raw Ctrl+B regression.
  Removed temporary key tracing. No input recording remains in application code.
- Created a loop on the ruler, resized its end and moved its top brace while
  preserving length. Info View described these actions. F1 opened its separate
  shortcut window; corrected garbled separators and verified final rendering.
- The automation API cannot inject a right-button double-click. The handler suite
  verifies that path; physical-device check remains: double-right-click empty
  instrument lane space, which should create and open a clip at the grid position.
  Multi-modifier gestures are primarily handler-tested, not all physically injected.
  No cross-DPI, exhaustive generator, note-properties-dialog or send-divider
  interaction test was performed in this update. Existing model suites passed.
- Updated normal build/Auralis_artefacts/Release/Auralis.exe and launched it.
  The user authorized discarding the previous unsaved session before replacement.
- This remains a session-only silent UI prototype. Full FL generator dialogs,
  arbitrary scale libraries, MIDI I/O, audition, editable ghosts, slide/portamento,
  audio playback and project save/load are not implemented. Full FL parity is
  explicitly not claimed. Documented workflows were checked against Image-Line's
  piano-roll and keyboard-shortcut manuals linked in the clip-editing decision.
- Source whitespace review found no findings. Git ownership/trust restrictions
  prevented normal status inspection; compared against a pre-edit local baseline.
  No global Git settings or commits were changed.

## 2026-09-27 - Sample libraries, audio clips and arrangement feedback

- Release x64 build succeeded with JUCE audio_devices added; no new third-party
  dependency was introduced. CTest passed 6/6: existing state/MIDI/mixer/editor
  suites plus new audio model/signal and callback suites.
- Audio tests verify stereo sample values, native-rate and octave resampling,
  reverse, source-region loops, fade values, mute/end silence, independent copies,
  undo/redo, split phase, range deletion and range movement.
- Callback tests verify sample-count clock, pause silence/position, loop overshoot,
  zero/variable block lengths, finite bounded output, and 500 concurrent immutable
  plan swaps. Offline workload: 128 voices, 48 kHz, 512 frames, 100 blocks; measured
  maximum 0.9452 ms per block versus 10.667 ms audio duration in one Release run.
  This is not a hardware latency or dropout certification and is not a listening test.
- Native 1440x900 content inspection: Add Library opened the folder picker and a
  user-added pack appeared as a sidebar category with nested folders. Clicking
  WAV and AIFF entries showed their actual decoded waveforms. The obsolete
  Choose audio button was absent. Dragging a WAV onto track 4 created an audio
  clip and opened the lower editor. Ctrl+B duplicated it; Reverse affected the
  copy and clicking the original showed Reverse still off. A dragged fade handle
  changed fade-in to 0.215 s. A time selection showed amber affected clip portions
  and a clip count; clip headers showed the hand cursor.
- The user authorized discarding the test session for executable replacement.
  The desktop automation tool detected concurrent user input several times;
  refreshed state before continuing. One drag returned a monitor error, but the
  subsequent refreshed native screenshot confirmed the import succeeded.
- No listening claim is made: automated tests checked generated output samples;
  native inspection checked workflows and rendering. Not yet covered: broad
  device-loss/sample-rate hot-swap/DPI testing, studio resampling quality, full
  malformed-file corpus, maximum-size pack stress, or every keyboard gesture.
- No source samples were changed. Library links persist separately from the
  session-only composition. Audio editor is intentionally documented as a basic
  subset: warp markers/algorithms, envelopes, recording, more audio tracks,
  plugins, real meters, live sends and project save/load remain unimplemented.
- Updated root AGENTS continuity instructions, PROJECT_STATE, FEATURE_MAP,
  BUILD_AND_VERIFY, README, F1/Info View and DECISION-AUDIO-CLIPS. No commit or
  global Git trust change was made. Source whitespace review had no findings.
- Final native follow-up: restarted the normal build at 1100x700; the user-added
  pack link persisted. Added a generated 6-second stereo sine WAV test folder,
  previewed and dragged it onto the scrolled Audio lane, and inspected the lower
  editor at compact size. Its white audition cursor advanced with device sample
  time. Removed only the temporary test-folder link through Unlink; the imported
  test clip remained, proving unlink does not delete imported audio. Kept the
  user's existing pack link. Corrected stale placeholder help over real files.

## 2026-09-28 shared routing and embedded devices

Windows x64 Release build succeeded with the existing VS2022/JUCE toolchain.
All six CTest executables passed after graph/device/editor changes. The existing
MixerState ownership test now inserts a real typed instrument rather than an
obsolete placeholder initializer. No tests were removed or weakened.

AudioOutputTests additionally verify a dynamic audio track with an interleaved
return ID, a half-level route through that return to Master, return mute, Master
mute for browser audition, real MIDI synth energy, stopped-transport keyboard
playback, complete note release, exact EQ bell centre gain, and finite output
from all eight effects. Existing variable-block, loop/clock, bounded-output and
500 concurrent-plan replacement checks still pass. One recorded offline run of
128 audio clips at 48 kHz / 512 frames had a maximum 1.1731 ms over 100 blocks;
this measures the sample mixer, not worst-case synth/effects or hardware latency.

Native UI checks used the actual Windows executable at normal 1440x900 and
compact 1100x700 content sizes. Verified: Prism browser drag creates a new linked
instrument track and mixer strip; double-click on that new lane creates/opens a
MIDI clip; notes can be drawn/resized; selecting mixer shows the same Prism;
Contour drag appends after Prism; EQ band drag changes both frequency and gain;
ENV 1 drag onto oscillator level creates an assignment visible in its depth menu;
resizing the lower panel reveals controls; the final compact arrangement keeps a
fixed visible drop zone even while track lanes scroll. Short keyboard taps
visibly activated arrangement dBFS and mixer instrument/Master meters. No human
listening or subjective audio-quality evaluation is claimed.

Corrections found during validation: compact EQ graph could hide controls;
short injected key taps were missed by timer polling; first-note device startup
could consume an audition. The graph now leaves scrollable controls visible,
key-press events latch an 80 ms minimum audition, and instrument/keyboard activation
warms the output device. Return reverb/delay/modulation effects default fully wet.

Limits: external plugin hosting, session persistence, complete controller-lane
DSP, general device undo, full Serum parity, exhaustive device-loss/sample-rate
coverage, worst-case multi-synth deadline profiling and listening remain future
work. The final source was reviewed directly; this workspace did not expose a
usable Git worktree to the command-scoped diff invocation. No Git trust settings
were changed, no commits made, and user library settings were preserved.

Final DSP regression checks also confirm that ENV 2 modulation of ENV 1 attack
changes rendered attack energy, and ENV modulation of LFO rate changes downstream
oscillator output. Modulation feedback uses a bounded one-sample delay; parameter
ranges are clamped. The final Release rebuild and all six suites passed after
these corrections. No new warnings were reported in changed source.

## 2026-09-28 — projects, device analysis, automation and VST3

Windows x64 / VS2022 Release. Eight suites pass in the normal `build` tree,
including repository-owned VST3 scan, stable-parameter control, actual stereo
processing, variable-block state restoration and teardown. ProjectTests verifies
embedded-media deduplication/CRC, MIDI/device/route/native-state round trips,
editor view state, corruption/truncation rejection, invalid target ranges and
byte-for-byte preservation of the destination after a failed save. FFT checks use
an actual anti-phase stereo 1 kHz input. Editor tests include automation point
creation, Alt-drag curvature, selection/delete/copy/paste/undo and Ctrl/Alt wheel
handlers. Existing audio graph, synth, effects, clock and plan-lifetime tests pass.

Native intermediate-build checks verified Prism tabs, larger knobs, LFO assignment
badge, touch-created grouped automation lanes and point editing. The user's test
session was saved through File > Save; the unsaved marker cleared before closing.
User-created plugin favorites and library paths were retained. Subsequent native
verification and any corrections are recorded below. No human listening,
exhaustive plugin compatibility or worst-case real-time certification is claimed.

The VST3 fixture test initially failed because its bundle path ended in `../..`
and it assumed exactly one host-visible parameter. Use the canonical bundle path
and locate the gain parameter by name (JUCE exposes extra wrapper parameters).
The corrected test passes without weakening its audio/state assertions.

Native QA also found Ctrl+O swallowed by a focused search TextEditor. Workspace
now registers a guarded listener on the focused child for project commands only;
normal text and lane/clip editing shortcuts remain with their owners.


Final verification: the normal Release build and all eight CTest suites pass.
Native QA reproduced restored TrackRow components covering the clip canvas;
Arrangement now restores clip/automation z-order after rebuilding rows. The
regression test makes its root component visible before testing hit detection
(the first test attempt otherwise returned no component). Saved MIDI clips are
visible again in the running app. Initial native parameter notifications and
queued piano toolbar initialization no longer dirty the freshly opened test file.

Observed real Master spectrum, device spectra, track dBFS meters and Gravity
input/output/gain-reduction histories during looped MIDI playback through Prism,
Contour, Gravity, Echo and the owned VST3 fixture. Used File > Export audio and
its native file chooser: artifacts/verified-export.wav is stereo 48 kHz 24-bit
PCM, 192000 frames (four seconds including the two-second tail), measured peak
0.00171947 at the deliberately quiet test Master setting. The app reported Export
complete. This verifies actual rendered data, not human listening quality.

Inspected native layouts at 1440x900 and 1100x700 logical sizes, plus maximized.
The fixed rack uses horizontal scrolling and internal parameter scrolling in the
compact viewport. Saved test-session edits before replacing its build. Reopened
the user's Desktop/Untitled.aup in the final executable: the title is clean and
its Diva/Pro-Q 3 rack and fifth-track MIDI clip are restored. User library links
and plugin preferences remain intact. Commercial plugin audio/editor compatibility
is not exhaustively verified; the automated host fixture is the repeatable test.


## 2026-09-29 — instruction handbook reconciliation

Documentation-only maintenance: consolidated PROJECT_STATE into a current source
and ownership map; reconciled existing FEATURE_MAP IDs and removed stale claims
that routing, instruments, saving, meters and hosting are unavailable. Updated
all six instruction files for the present engine, persistence, rack layout,
automation, VST3 boundaries and regression suite responsibilities. Read current
source/targets and constants; checked relative Markdown links, UTF-8 and feature-ID
uniqueness. No application code or contextual UI behavior changed, and no build,
runtime tests or new native verification were performed in this update. Historical
2026-09-28 results remain dated evidence rather than a claim of new verification.
