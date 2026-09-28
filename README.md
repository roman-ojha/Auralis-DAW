# Auralis

AI contributors: start with [AGENTS.md](AGENTS.md) and the
[Instructions handbook](Instructions/README.md) before modifying this project.

A native Windows C++20 / JUCE DAW prototype with independent MIDI/audio clips,
built-in instruments/effects, channel/send/Master routing, VST3 hosting, grouped
automation, self-contained `.aup` projects and stereo WAV export. This is an
incremental application, not complete FL Studio, Serum or Ableton feature parity.

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
- Add Library links a sample folder; browse nested folders and click files to hear
  them. Click the small waveform to replay. Unlink removes only the saved link.
  Mono/stereo WAV, AIFF and FLAC; up to 10 minutes, 384 kHz and 128 MiB decoded per
  file. Background decoding; sample files are never modified.
- Drag a file onto an audio track or the bottom drop zone, then click the clip to edit it below. Independent
  gain, pitch by resampling, reverse, loop, source trim and linear fades. Drag the
  amber arrangement handles for fades. Ctrl+B duplicates; Ctrl+E splits; range
  selection, delete/move and clip undo include audio. Session limits: 128 audio
  clips and 512 MiB imported media (history retains source buffers).
- Auralis orbit logo is embedded in the executable and native window icon.

- Two-column library: icon categories (Instruments, Sounds, Effects, Plug-Ins) on the left, expandable subsections and searchable devices on the right, embedded from `assets/library.json`.
- Four initial lanes, with additional linked tracks created by dropping audio/instruments below them with shared rotary gain, a combined mute/solo button and arm UI.
- Horizontal and vertical timeline scrolling. The arrangement title/count/zoom
  toolbar remains removed; Ctrl+wheel / Page Up/Down zoom both editors.
- Piano-roll/arrangement/mixer icon switches sit after App Resources (also in View menu).
  Both views reference the same channel state, source slot and device-chain owner.
- Mixer with pinned master/current meters, channel faders, pan and stereo knobs,
  and polarity icons. Left-click the power icon to mute; Ctrl+right-click toggles
  solo (S while focused is the keyboard alternative). Audio track/Master gain, mute, solo, pan and polarity affect playback; stereo width and post-fader sends are processed in the audio graph.
- Right-docked send tracks: use + to create, dock icon to hide/show. Select a source
  strip, click a destination arrow, then adjust its send amount. Routes can connect
  ordinary and send tracks in either direction, except self/feedback paths. Sends
  have no clip lanes; their automation appears in grouped automation rows. Up to 32 sends; each starts routed to Master.
- Mixer cables show the selected source's visible destinations. Scroll channel
  lanes or show the dock to reveal hidden destinations; Info View lists routes.
  Sends and returns use the same ordered device graph as channels and Master.
- Play/pause/stop use the audio-device sample clock. Preview pauses arrangement playback; Escape or Stop stops audition. Record only toggles armed state.
- Editable BPM and time signature, shared snap grid, metronome toggle,
  and a selectable transport loop. There is no metronome sound.
- Measured 20 Hz to 20 kHz Master/device FFT displays and stereo track sample-peak meters. Gain settings do not create a signal.
- CPU is actual application CPU usage, normalized across all logical processors;
  RAM is process resident memory in MiB. These are not audio DSP performance metrics.
- The device rack follows arrangement/mixer selection. Built-ins stay embedded; VST3 editors open in separate windows. Prism uses oscillator/envelope/LFO tabs and modulation badges. Contour has a large spectrum/EQ graph; Gravity shows input/output and gain-reduction history.

Drag the browser divider, its internal category/results divider, track-header divider, or audio-clip-editor divider. The instrument/effect rack has a fixed height. Double-click
one to reset it, or use View > Reset Layout. The menu and transport stay at fixed heights.
The window supports resizing from 1100 x 700 logical pixels, native maximize,
DPI scaling, and scrolling where the viewport cannot show all content.
Space toggles preview playback when the workspace handles keyboard focus; Escape
stops. Text fields and standard controls retain their own keyboard behavior.

## MIDI editing

Create a clip by double-clicking an empty instrument lane with either mouse button,
using Ctrl+Shift+M at the cursor, + MIDI clip, or Create > MIDI clip. Double-left-click
a clip opens its piano roll. The piano roll also has + Clip and creates a document
when you draw on an empty editor. New piano-roll clips start at the transport
position on the selected instrument track (track 1 when an audio/send/master is selected).
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

The piano editor uses the full workspace height, shaped white/black piano keys,
pitch labels/highlights and a vector tool strip. Shift-drag clones selected notes;
both note edges resize; mute/erase/paint work across swept paths; Slice cuts notes
along a dragged line. Double-click a note (or Enter with selection) opens numeric
properties. Z drags a zoom region; Shift+4/5 fits all/selected notes. Alt+V displays
read-only ghost outlines from overlapping clips.

The Tools menu provides undoable quick legato, glue, grid chop, time reversal,
pitch flip, strum, arpeggiation, humanization, C major/minor pitch correction and
triad presets. These are bounded editing operations, not FL's complete generator
dialogs. The loop brace can be moved by its top bar and resized at either edge;
Shift-drag creates a new range. F1 lists the corresponding keyboard operations.

Full FL Studio feature parity remains unfinished: Riff Machine and full generator
dialogs, arbitrary scale libraries, MIDI I/O, editable ghosts and
slide/portamento processing are not implemented. Notes play loaded instruments and save with the project. Piano-roll channel controller events are retained, but their complete DSP mapping remains unfinished.

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

The UI model is intentionally separate from real-time audio processing.
JUCE owns GUI resources through RAII; timer work runs on the message thread.
The audio callback reads immutable render plans; no microphone input or network service is started. External plugins load only by explicit insertion or project restore.
The waveform inspector adds JUCE audio_formats/audio_basics for file decoding only;
sample playback opens the default stereo output with no input channels. Recreate the original icon using
`assets/Generate-Icon.ps1`, then reconfigure and rebuild to refresh Windows resources.

## Dependency licensing

JUCE is dual-licensed under AGPLv3 and its commercial licence. A commercial JUCE
licence may be required depending on distribution and eligibility. Review
`external/JUCE/LICENSE.md` and https://juce.com/legal/juce-8-licence/ before release.
No proprietary redistribution licence for Auralis is asserted by this prototype.




### Built-in instruments and live routing

Drag **Prism** from Instruments onto a MIDI lane or **Drag clip/instrument here**
to create a new linked track. Drop effects into the lower device chain; they run
left to right. **Contour** offers seven draggable EQ bands; **Bloom** is the
reverb. **F6** turns computer-keyboard audition on/off (A W S E D F T G Y H U J K
O L P). Piano keys and placed notes audition the loaded instrument. Drop a sample
into a MIDI track's device area to load **Atlas**, or into the arrangement drop
zone for a new Audio track. Click a track header/mixer strip to show its chain;
click an audio clip to show its editor.

Tracks, returns, browser audition and Master now share a real audio graph with
post-fader routing and live sample-peak dBFS meters. Built-ins instantiate lazily;
VST3 hosting and project saving are available. Full Serum feature parity is not implemented. See [audio graph details and limits](docs/DECISION-AUDIO-GRAPH.md).

## Projects, automation and external devices

- Ctrl+N/O/S and Ctrl+Shift+S: New/Open/Save/Save As. `.aup` embeds source PCM,
  independent clip edits, MIDI, channels, routing, device settings/native state,
  automation and editor view positions. Playback opens stopped. Original files
  survive failed validation/writes. Missing plugins keep their state and lanes.
- Ctrl+Shift+R exports the routed Master to stereo PCM WAV at 44.1/48/96 kHz,
  16/24/32 bits, whole arrangement or loop range, with a configurable effect tail.
- Shift+A shows automation and captures touched mixer/device/native-plugin
  parameters. Child rows identify the target; rename or bypass each lane.
  Click/drag points, right-click/Delete to remove, Alt-drag segments for curves,
  Ctrl+A/C/X/V for point selection/copy/cut/paste, Ctrl+Z/Y for lane edit history.
  Paste starts at the transport cursor and transfers normalized values.
- F2 or double-click a channel name renames its linked mixer/arrangement state.
  Ctrl+wheel zooms timeline; Alt+wheel changes arrangement track height.
- Options > Plug-In Settings: add folders, explicitly scan and star Windows x64
  VST3 plugins. Drag favorites from Plug-Ins to a compatible rack/track. Scans run
  in `AuralisPluginScanner.exe` beside the main executable, with cancellation,
  timeout and failed-plugin quarantine. Retry failed clears that quarantine.
  Loaded plugins execute in-process; a bad running plugin can still crash the host.

Limits: no VST2 `.dll`, CLAP, AU, 32-bit bridge, multichannel/sidechain hosting,
plugin latency compensation, autosave/recovery, general project undo, audio/MIDI
recording or full advanced automation/warp tools. Runtime plugin compatibility
requires individual testing. `.aup` is Auralis-specific, not an Audacity project;
no machine-wide file association is installed. See the latest
[project state](Instructions/PROJECT_STATE.md) and [validation](docs/VALIDATION.md).
