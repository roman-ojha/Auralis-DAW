# Auralis

AI contributors: start with [AGENTS.md](AGENTS.md) and the
[Instructions handbook](Instructions/README.md) before modifying this project.

A native C++20 / JUCE desktop workspace for a future DAW. Version 0.2 is a UI
prototype: no audio engine, audio recording, arrangement import, plugin loading, or
instrument DSP exists yet. UI state lasts for the current session only.

## Build and run (Windows)

Requires Visual Studio 2022 C++ desktop tools, Windows SDK, CMake 3.22+, and Git.
JUCE 8.0.12 is pinned to commit 29396c22c93392d6738e021b83196283d6e4d850.
A local `external/JUCE` checkout can be used; otherwise CMake downloads that revision.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
& '.\build\Auralis_artefacts\Release\Auralis.exe'
```

Open `build/Auralis.sln` in Visual Studio and select Auralis as the startup project.
The former RomanDAW binary may remain in the ignored build directory; it is not
part of the renamed project. The workspace folder is intentionally unchanged.

## Workspace

- Integrated File, Edit, Create, Playback, View, Navigate, Options and Help menus.
  Planned commands are disabled; Reset Layout is in View.
- Info View at the bottom of the sidebar explains hovered or keyboard-focused
  controls. Future features must extend this shared help system.
- Choose audio in the waveform panel (or File menu) to inspect a local WAV,
  AIFF or FLAC waveform. This is read-only: no playback or track import. Up to
  10 minutes / 32 channels / 384 kHz; decoding runs in a background worker.
- Auralis orbit logo is embedded in the executable and native window icon.

- Two-column library: icon categories (Instruments, Sounds, Effects) on the left, expandable subsections and searchable placeholders on the right, embedded from `assets/library.json`.
- Four lanes (three instrument lanes support independent MIDI clips) with shared rotary gain, a combined mute/solo button and arm UI.
- Horizontal and vertical timeline scrolling. The arrangement title/count/zoom
  toolbar remains removed; Ctrl+wheel / Page Up/Down zoom both editors.
- Piano-roll/arrangement/mixer icon switches sit after App Resources (also in View menu).
  Both views reference the same channel state, source slot and device-chain owner.
- Mixer with pinned master/current meters, channel faders, pan and stereo knobs,
  and polarity icons. Left-click the power icon to mute; Ctrl+right-click toggles
  solo (S while focused is the keyboard alternative). These are UI controls.
- Right-docked send tracks: use + to create, dock icon to hide/show. Select a source
  strip, click a destination arrow, then adjust its send amount. Routes can connect
  ordinary and send tracks in either direction, except self/feedback paths. Sends
  have no arrangement lanes. Up to 32 sends; each starts routed to Master.
- Mixer cables show the selected source's visible destinations. Scroll channel
  lanes or show the dock to reveal hidden destinations; Info View lists routes.
  Routing is UI state only; no audio mixing or send DSP is running.
- Play/pause/stop advance a wall-clock UI preview. Record only toggles armed state.
- Editable BPM and time signature, shared snap grid, metronome toggle,
  and a selectable transport loop preview. There is no metronome sound or audio DSP.
- A 20 Hz to 20 kHz logarithmic spectrum display and silent track meters. No
  fabricated audio activity. Gain settings do not create a signal.
- CPU is actual application CPU usage, normalized across all logical processors;
  RAM is process resident memory in MiB. These are not audio DSP performance metrics.
- The blank device area follows the selected track and illustrates left-to-right
  flow; it contains no plugin editor or functional routing yet.

Drag the browser divider, its internal category/results divider, track-header divider, or device-area divider. Double-click
one to reset it, or use View > Reset Layout. The menu and transport stay at fixed heights.
The window supports resizing from 1100 x 700 logical pixels, native maximize,
DPI scaling, and scrolling where the viewport cannot show all content.
Space toggles preview playback when the workspace handles keyboard focus; Escape
stops. Text fields and standard controls retain their own keyboard behavior.

## MIDI editing

Create a clip by double-right-clicking an instrument lane, using Ctrl+Shift+M at
the cursor, or Create > MIDI clip. Double-left-click a clip opens its piano roll.
Draw, paint, select, move, resize, erase, mute and slice notes. Velocity, note pan
and fine pitch use the lower control lane; channel volume/pan/pitch use separate
clip-local event curves. F / Shift+F cycles the property.

Drag arrangement clip edges to trim or repeat its local loop. Drag clip bodies or
empty lanes to select time across tracks, Delete to cut it out, and drag inside
that selection to move its fragments. Every clip/copy/split owns independent data.
Ctrl+L loops the selected range; dragging the ruler sets the loop directly.

MIDI edits have Ctrl+Z undo / Ctrl+Y redo. Shared grid options include triplets;
Alt bypasses snapping. The send dock and piano-roll control lane are resizable.
F1 (or Help > Shortcut manual) opens a separate, scrollable shortcut reference.
Transport buttons now use icons. Text fields retain standard editing behavior.

This implements the core clip editor, not every FL Studio feature. Generative
composition tools, chord/scale tools, ghost channels, MIDI I/O, audition and
slide/portamento processing remain future work. Notes and automation are editable
in memory; no sound is produced and project files are not saved yet.

## Source layout

- `src/constants/Design.h`: shared theme, typography, dimensions and limits.
- `src/model/SessionState.h`: UI transport state and constrained panel layout.
- `src/model/MidiProject.h`: clip ownership, notes/events, range edits and undo/redo.
- `src/constants/Editing.h` and `Shortcuts.h`: editing limits and shortcut reference.
- `src/model/MixerState.h`: shared channel ownership, parameters and acyclic sends.
- `src/ui/`: separate browser, transport, arrangement, theme and workspace components.
- `src/platform/`: Windows process resource sampling.
- `assets/`: dummy catalog data, embedded at build time.
- `Instructions/FEATURE_MAP.md`: maintained feature inventory, source ownership,
  behavioral contracts and required updates for every implementation task.
- `tests/`: timing, loop, invalid-input and extreme-size layout regression checks.

The UI model is intentionally separate from future real-time audio processing.
JUCE owns GUI resources through RAII; timer work runs on the message thread.
No audio thread, microphone permission, plugin scanning or network service is started.
The waveform inspector adds JUCE audio_formats/audio_basics for file decoding only;
it does not open an audio device. Recreate the original icon using
`assets/Generate-Icon.ps1`, then reconfigure and rebuild to refresh Windows resources.

## Dependency licensing

JUCE is dual-licensed under AGPLv3 and its commercial licence. A commercial JUCE
licence may be required depending on distribution and eligibility. Review
`external/JUCE/LICENSE.md` and https://juce.com/legal/juce-8-licence/ before release.
No proprietary redistribution licence for Auralis is asserted by this prototype.



