# Auralis feature and maintenance register

Read this register at the start of every implementation task. Update affected rows
and contracts before completing every feature, fix, removal, or architecture change.
Verify source first: this is a maintained map, not proof that behavior was tested.
Last implementation milestone: 2026-09-28. Documentation reconciled: 2026-09-29.

All tables below describe current behavior; stable feature IDs are retained from
earlier milestones. Historical decisions and test evidence live under `docs/`.

## Current projects / devices / hosting / automation

| ID | Capability | Owner | Contract |
| --- | --- | --- | --- |
| PROJECT-01 | Versioned self-contained .aup New/Open/Save/Save As | ProjectFile, ProjectActions, SessionView | Validate before mutation; temporary validated save before replacement; embed PCM once; preserve missing-plugin blobs and IDs; no autosave or undo history persistence |
| EXPORT-01 | Routed stereo Master WAV with range/rate/depth/tail choices | ProjectActions, AudioOutput | Independent graph/native instances; atomic replacement; 16/24/32-bit PCM; no pre-roll, dither, stems or PDC |
| HOST-01 | Explicit folder scan, persistent stars/quarantine, Plug-Ins browser | PluginSettings, PluginScanner, Browser | Windows x64 VST3; isolated 20-second scan timeout/cancel; preserve preferences |
| HOST-02 | Native VST3 rack, editor, state and parameter automation | PluginHost, HostedProcessor, DeviceState | Stable parameter IDs; stereo buses; callback blocks <=64; UI owns editors/final teardown; missing effects pass through; runtime not isolated |
| AUTO-01 | Shift+A capture and grouped child lanes, including Master/returns | MixerState, Arrangement, AutomationLaneView | No extra mixer channel; target identity retained; lane rename/bypass; points/curves, lane undo/redo, normalized copy/cut/paste |
| AUTO-02 | Playback and saved automation | Automation, AudioOutput, ProjectFile | Per-sample built-ins, <=64-sample native parameter blocks; validate target ranges; manual base restored on bypass; missing targets retained/ignored |
| ANALYSIS-01 | Real Master/device 20 Hz-20 kHz spectra and Gravity history | SignalAnalysis, Transport, DeviceArea | Bounded audio queue; UI FFT; actual sample rate, stereo magnitude max; sample peaks, not true peak |
| DEVICE-02 | Wide Prism tabs, modulation badges, enlarged EQ, fixed-height rack | DeviceArea, Devices, Workspace | Embedded built-ins; larger scrollable knobs; audio clip editor remains resizable; original artwork |
| FX-03 | Expanded Echo parameters and DSP | DeviceState, DeviceProcessor | Tempo sync, offset/model, feedback filters/drive, modulation/smoothing/diffusion, dry/wet levels |
| ARR-06 | Ctrl+wheel timeline zoom, Alt+wheel track height, shared rename | ClipTimeline, Arrangement, Mixer, Workspace | Bounded zoom/height; shared IDs; F2 respects text entry; restore clip/automation z-order when rows rebuild |
| QA-06 | Project, FFT, automation and native fixture checks | ProjectTests, AudioOutputTests, HostVerification, HostFixture | Round trips and failure preservation; measured DSP; native scanner/host lifecycle; no broad compatibility certification |

See DECISION-PROJECTS-HOSTING-AUTOMATION.md and current validation notes for limits.
New controls use Info View; F1 includes project, hosting and automation shortcuts.

## Capability inventory

| ID | Capability and current behavior | Owner / source | Preserve and verify when changing |
| --- | --- | --- | --- |
| APP-01 | Native Windows x64 C++20/JUCE application; single instance; resizable window; compact QA argument | `src/main.cpp`, `CMakeLists.txt` | Native close and shutdown; 1100x700 minimum; no machine-wide settings changes |
| APP-02 | Original orbit icon in native window and executable resources | `assets/auralis-icon.png`, `assets/Generate-Icon.ps1`, `src/main.cpp`, CMake ICON_BIG/SMALL | Rebuild resources after asset changes; inspect title bar; Windows may cache executable/taskbar icons |
| MENU-01 | Integrated themed File, Edit, Create, Playback, View, Navigate, Options, Help menus | `src/ui/ApplicationMenu.h`, `Theme.*` | Functional commands share transport state; planned commands stay disabled; detach menu model before destruction |
| MENU-02 | View > Reset Layout restores outer panels, library divider, track headers, timeline scroll and mixer dock | `Workspace::resetLayout`, `Browser::resetLayout`, `Arrangement::resetLayout` | Does not reset transport, gains, selection, or pretend to save a project |
| TRANS-01 | Audio play/pause/stop, elapsed time, bar/beat position and user-selected loop | `Transport.*`, `model/SessionState.h`, `Workspace` timer | Pause preserves position; stop returns to start and disarms; loop preserves overshoot; message timer is never an audio clock |
| TRANS-02 | Tempo 20–300 BPM; supported time signatures; shared note/clip snap grid; record/metronome UI toggles | `Transport.*`, `constants/Design.h` | Signature denominator affects duration; snap affects MIDI edits; record/metronome produce no audio; menu toggle state matches toolbar |
| MON-01 | Real Master/device spectrum and track meters | `Transport`, `SignalAnalysis`, `TrackRow`, `MixerControls` | See ANALYSIS-01/METER-01; real audio only, silence remains silence |
| MON-02 | Real process CPU and resident memory in MiB | `platform/ProcessMetrics.*` | CPU normalized across logical processors, not DSP load; sampling is throttled |
| LIB-01 | Instruments, Sounds, Effects, Plug-Ins and linked-folder categories | `Browser.*`, `constants/Library.h`, `assets/library.json` | Built-in metadata refers to real devices; no fabricated sample presets; detach tree root before replacement/destruction |
| LIB-02 | Search current category and browsed library files; empty-results state | `Browser::filter`, `selectCategory` | Keep category/query/selection coherent; file search does not imply a recursive disk index |
| HELP-01 | Persistent bottom-left Info View resolves pointer and keyboard focus context | `ContextHelp.h`, `Workspace::timerCallback` | Do not overwrite help while pointer is reading Info View; scroll long help; explicit descriptions for controls and HelpProvider for painted regions |
| WAVE-01 | Library waveform, filename/duration and audible preview | `WaveformPreview.*`, `Workspace` | Read-only source; click waveform to replay through Master; see WAVE-03 |
| WAVE-02 | Background decode, bounded buffer/peak memory, cancellation generation, error states | `WaveformPreview::load`, `Exchange`, `ThreadPool` | Maximum 10 min / 2 channels / 384 kHz / 128 MiB decoded; stale results never replace newer selection; no decoder work in paint/input; join worker before destruction; retained preview PCM is bounded; imported media is embedded when saving projects |
| ARR-01 | Dynamic linked arrangement/mixer tracks, MIDI and audio clips | `Arrangement`, `Workspace`, `MixerState` | Stable track IDs; selection exposes shared device chain; independent clip settings |
| ARR-02 | Shared gain -60 to +6 dB, mute/solo and arm UI | `TrackRow`, `MixerState` | Gain/mute/solo affect graph audio; recording arm remains UI only |
| ARR-03 | Timeline ruler, horizontal scroll and vertical track scroll; title/zoom toolbar removed | `Arrangement`, `TrackRow` | Preserve valid scroll ranges, current signature divisions, clipping at lane bounds and compact usability |
| DEV-01 | Selected-channel embedded device path | `src/ui/DeviceArea.*` | Instrument/sample followed by effects; native plugins have separate editors; see DEVICE-01/02 |
| LAY-01 | Browser/columns, track header, send dock and audio-editor resizing | `Theme::Splitter`, `PanelLayout`, `Browser`, `Arrangement`, `Mixer`, `Workspace` | Clamp bounds and preserve reset; transport and device rack have fixed heights; rack/parameters scroll |
| UX-01 | Shared dark palette, readable text, vector category icons, tooltips and keyboard names | `Theme.*`, `constants/Design.h` | Keep original visual language; no Windows default menu chrome; do not shrink text to hide overflow |
| QA-01 | CMake Release build and CTest transport/layout plus shared-channel/routing tests | `tests/StateTests.cpp`, `tests/MixerStateTests.cpp`, `Instructions/BUILD_AND_VERIFY.md` | Meaningful regression checks; historical UI evidence lives in `docs/VALIDATION.md` |

## Mixer

| ID | Capability | Owner / source | Maintenance contract |
| --- | --- | --- | --- |
| MIX-01 | Arrangement/mixer icon switches after App Resources; also View menu | `Transport`, `Workspace`, `ApplicationMenu` | Switching never recreates channel state or changes selection; arrangement title/count/zoom controls remain removed until requested |
| MIX-02 | One shared channel instance per arrangement lane | `MixerState`, `TrackRow`, `MixerStrip` | Stable TrackId, no per-view state copies; UI mutations on message thread; clear observing rows before restoring channel storage |
| MIX-03 | Channel gain, pan, polarity, stereo width and mute/solo | `Mixer`, `MixerControls`, `AudioOutput` | Real DSP; left-click mute, Ctrl+right-click solo; additive solo preserves explicit mute and downstream routes |
| MIX-04 | Same rotary style for arrangement gain and mixer knobs | `Knob`, `Theme::drawRotarySlider` | Vertical drag, editable gain/send readouts, double-click defaults, focus/help support; do not duplicate parameter ownership |
| MIX-05 | Master/current selected measured dBFS meters | `MixerBody`, `drawMeter` | Silence is -infinity, not fader position; Master is destination-only; sample peaks, not true peak |
| MIX-06 | Resizable right-docked sends, create/show/hide, horizontal/vertical scrolling | `Mixer`, `MixerBody`, `constants/Mixer.h` | Sends have no arrangement lane. Max 32 sends; creation defaults to Master output. Hiding preserves routes; references remain valid; detach viewports on destruction |
| MIX-07 | Selected-source routing arrows, cables, independent 0..100% send amount knobs | `MixerState::toggleRoute/canRoute/sendAmount`, `MixerStrip`, `MixerBody::paintOverChildren` | Allow arrangement-to-arrangement, arrangement-to-send, send-to-send and send-to-arrangement; reject self/unknown IDs, Master source and indirect cycles. Existing routes always removable. New routes unity gain; post-fader sends. Cables only for visible endpoints; Info View lists all destinations |
| MIX-08 | Deterministic shared-ownership, routing and boundary regression tests | `tests/MixerStateTests.cpp` | Test indirect cycles, route removal, stable channel addresses, source/device aliasing, invalid values, selection persistence and send cap |

MixerState owns channel and routing values. AudioOutput consumes published plans;
never read this mutable UI model from the callback. GRAPH-01 covers actual graph
execution. Native plugin and restored-row lifetimes require explicit teardown.

## Contextual help contract — every new control

1. `configureButton` registers button title, tooltip and description together.
   For other components call `setHelp(component, title, description)` or supply
   a useful tooltip. Explain action, units/range, reset shortcut and current limits.
2. Nested slider editors/buttons inherit their nearest documented ancestor.
   Give specialized child controls their own descriptions when semantics differ.
3. For custom-painted regions implement `HelpProvider::helpAt(localPoint)`.
   Track lanes/meters and transport position/spectrum/resources use this path.
4. Tree item tooltips explain subsection expansion or the selected placeholder.
   Menus provide a shared description; individual planned commands are visibly
   disabled and labeled `(planned)`. Native file dialogs retain OS help behavior.
5. Verify hover and Tab focus in Info View at both supported QA sizes. Do not
   substitute generic workspace help for a new feature's specific semantics.
6. Help text is also maintenance documentation: update it when functionality
   becomes real, changes range, moves, or is removed. Do not leave “UI only” on
   working audio features or remove that warning before audio exists.

## Data flow and lifetime

- Workspace owns theme, transport state, menus and panels. UI mutations run on
  the message thread. Transport menus and buttons use the same TransportState.
- Waveform file chooser callback holds a Component::SafePointer. Decoder jobs
  own only a file and shared Exchange; they never access components. Worker
  results cross a short mutex-protected handoff, polled on the UI thread.
  Generation changes cancel superseded work between bounded read blocks.
- Preview combines absolute peaks from every channel and displays a symmetric
  envelope, clipped visually at full scale. It is not an FFT, sample editor,
  normalization operation, or true-peak measurement. Sources are never modified.
- MIDI/audio and per-lane automation have separate bounded histories. Project
  serialization and automation playback exist; general mixer/transport undo does not.

## Required lifecycle update

For each implementation task: identify affected IDs, inspect their contracts,
update/add/remove rows to match final behavior, update contextual help, update
PROJECT_STATE when scope changes, and record actual verification in VALIDATION.
Record unfinished work as unfinished with concrete limits. Keep IDs stable when
renaming source files. No instruction file guarantees compliance by every AI tool;
the root AGENTS.md is the entry point for compatible agents.

Known missing capabilities are centralized in [PROJECT_STATE.md](PROJECT_STATE.md).

## MIDI clip editing

| ID | Capability | Owner | Contract |
| --- | --- | --- | --- |
| MIDI-01 | Independent clip-local notes/events, repeated source loop, stable IDs | `MidiProject`, `constants/Editing.h` | Quarter-note ticks at 960 PPQ; copies and splits deep-copy content; no shared patterns; no audio thread access |
| MIDI-02 | Note drawing/painting/selecting/moving/resizing/erasing/muting/slicing | `PianoRoll` | Ctrl+right-drag selects per user request; right-drag erases swept notes; bounds 0..127 pitch and positive durations; edits stay inside source loop |
| MIDI-03 | Velocity, note pan/fine pitch and clip-local controller events | `PianoRoll`, `MidiNote`, `MidiEvent` | Note attributes feed built-ins; controller editing/persistence is not complete controller-event DSP; native notes do not imply per-note expression support |
| MIDI-04 | Snap options, copy/cut/paste/duplicate, quantize, MIDI undo/redo | `MidiProject`, editor key handlers | Shared snap across both editors/top toolbar; letter shortcuts case-insensitive; history one pre-gesture snapshot, maximum 100; text editors keep keys |
| MIDI-05 | Clip create/open, edge trim/repeat, range cut/move, split | `ClipTimeline`, `Arrangement` | Instrument lanes only; cuts leave later time in place; fragments own data and retain loop phase; never overwrite another clip's notes through aliasing |
| MIDI-06 | Selected transport loop and ruler brace | `Arrangement`, `TransportState`, `AudioOutput` | Quarter-note beat units; sample-clock playback wraps chosen boundaries |
| MIDI-07 | Shared horizontal zoom/scroll, vertical scroll/zoom, middle-drag pan | Editor components | Clamp ranges; preserve usable track headers, notes and controls at compact size; no unbounded grid drawing |
| HELP-02 | F1 separate shortcut window; Info View shortcut hints | `ShortcutWindow`, `Shortcuts.h`, controls | Maintain manual with handlers; explicitly document Auralis conflicts and unimplemented FL features; hide on close, Workspace owns lifetime |
| QA-02 | Deterministic clip/history/snap/loop tests | `tests/MidiTests.cpp` | Include independent note AND automation ownership, loop-phase trim, range move, undo/redo |

See [the clip-editing decision](../docs/DECISION-CLIP-EDITING.md) for historical
choices. Projects now save; general undo, advanced generators, MIDI I/O,
editable ghosts and slide/portamento parity remain incomplete.

## Interactive MIDI tools

| ID | Capability | Owner | Contract |
| --- | --- | --- | --- |
| MIDI-08 | Empty instrument lanes accept either-button double-click; visible + MIDI clip and + Clip actions; drawing in an empty piano roll creates a document | `ClipTimeline`, `Arrangement`, `PianoRoll`, `Workspace` | Audio lanes reject MIDI; new piano-roll clips use selected instrument track (track 1 fallback for master/audio/sends), at transport position; arrangement uses timeline cursor. Never silently edit an unrelated inactive clip |
| MIDI-09 | Piano-shaped keyboard, pitch highlights, full-height editor, tools/properties | `PianoRoll`, `MixerControls`, `Workspace` | Loaded source auditions keys; keep usable at compact size; guard modal callbacks and resolve stable IDs |
| MIDI-10 | Swept paint/erase/mute, dragged slice line, both-edge note resize, Shift-drag clones, zoom selection/fit/presets, ghost outlines | `PianoRoll` | Each gesture has one pre-edit checkpoint; unique note IDs; bounded note count; ghost notes are read-only; edits independent of other clips |
| MIDI-11 | Quick legato/glue/chop/reverse/flip/strum/arpeggiation/humanize, C major/minor correction and triads | `model/MidiEditing.h`, `PianoRoll::showActions` | Explicit bounded presets, not full FL generator parity. Preserve unselected notes and automation; undoable; clamp pitch, length and timing; reject cap overflow before mutation |
| MIDI-12 | Loop edge resizing, brace move and ruler keyboard operations | `Arrangement`, `AudioOutput` | Shift-drag replaces range; preserve positive bounded range; sample-clock playback |
| QA-03 | JUCE editor-handler regression executable | `tests/EditorTests.cpp`, CMake | Exercise creation, mouse edits, modifier selection, delete/undo, cloning, sweep mute/erase, slicing, tools and snap; supplement with native UI checks |

MIDI-04 snap update: None/Alt preserves tick precision (960 PPQ); minimum note
duration remains 15 ticks. Line chooses a zoom-dependent grid in each view;
fixed divisions and Bar remain shared. Help and F1 describe these distinctions.

## Audio libraries and arrangement

| ID | Capability | Owner | Contract |
| --- | --- | --- | --- |
| LIB-03 | Add/unlink persistent folder categories; nested folder browsing and filename search | Browser, JUCE PropertiesFile/DirectoryContentsList | Max 32 links; per-user local settings; no sample copying/deletion; missing folders remain recoverable; scans off UI thread |
| WAVE-03 | Background mono/stereo decode and audible preview; click waveform to replay | WaveformPreview, Workspace, AudioOutput | Generation cancellation, finite PCM, 10 min / 384 kHz / 128 MiB per file; preview reduced gain; report decode/output errors; no Choose audio button |
| AUDIO-01 | Immutable source PCM with independent clip settings; shared MIDI/audio undo | AudioClip, MidiProject | Source unchanged; copy/split properties independent; phase preserved; bounded audio count/history/media |
| AUDIO-02 | Drop audio on compatible tracks or empty drop zone; lower clip editor | `ClipTimeline`, `Arrangement`, `Workspace`, `AudioEditor` | Validate extension/decode; new drop-zone track shares mixer state; audio editor remains resizable |
| AUDIO-03 | Source start/end, gain, resampling pitch, reverse, repeat, mute and linear fades | AudioEditor, audioSamplePrepared | Units seconds/dB/semitones; pitch also changes speed; positive source region; source files read-only; edits undoable |
| AUDIO-04 | Audio move/trim/duplicate/split/range cut/move; arrangement fade handles | ClipTimeline, MidiProject | Stable identities, clip-local changes, bounded length/time, Ctrl+B and Ctrl+E; preserve fragment phase |
| AUDIO-05 | Stereo graph output, arrangement and sample/clip audition | `AudioOutput`, `Workspace` | Zero recording inputs; plans/reclamation off callback; sample clock; full routing in GRAPH-01 |
| ARR-04 | Hover cursors and selected-clip portions highlighted in amber with count | ClipTimeline | Distinguish time selection from active clip; resize edges, move headers/ranges, selection crosshair; MIDI and audio coverage |
| QA-04 | Pure audio signal/model tests and offline callback timing/concurrent-plan tests | AudioTests, AudioOutputTests | Validate stereo, fades, pitch/speed, reverse, loops, split phase, copy independence, history, variable blocks and plan lifetime; offline rendering is not a listening/device certification |

Audio limits: resampling changes speed/pitch together; advanced warp, clip
envelopes and recording are absent. Persistence, dynamic tracks, routing and
hosting now exist. Library links persist separately from project content.

## Shared audio graph and embedded devices

See [the graph decision](../docs/DECISION-AUDIO-GRAPH.md) for architecture and limits.

| ID | Capability | Owner | Contract |
| --- | --- | --- | --- |
| GRAPH-01 | Real topological channel/send/Master rendering | AudioOutput, MixerState | One shared model; post-fader DAG, no cycles; immutable plan publication and off-callback reclamation; preserve DSP history across parameter edits |
| METER-01 | Arrangement, mixer and current-channel stereo dBFS meters | AudioOutput, MixerControls, Arrangement, Mixer | Measured post-fader sample peaks; 300 ms decay; red at >=0 dBFS; not RMS/true peak |
| ARR-05 | Drag-created audio/instrument tracks and visible bottom drop area | Arrangement, ClipTimeline, Workspace, MixerState | Stable IDs separate from visual row; linked mixer strip and default Master route; effects require an existing channel; type-safe single-clip moves |
| DEVICE-01 | Embedded, horizontally scrolling shared device chain | DeviceArea, DeviceState, Workspace | One source followed by effects; append right; bypass/remove controls; channel selection returns from audio clip editor; no floating built-in editors |
| SYNTH-01 | Prism oscillators, envelopes, LFOs and low-pass filter | DeviceProcessor, DeviceArea | 24 voices, 3 oscillators, 8 unison voices each; 3 AHDSR, 4 sine LFOs, max 32 drag assignments; original bounded DSP, not full Serum |
| SAMPLE-01 | Atlas sample instrument | DeviceProcessor, Workspace | Immutable PCM, independent device settings; root pitch, attack/release and level; source files unchanged |
| FX-01 | Bloom, Echo, Gravity, Drive, Flange, Phase and Chorus DSP | `DeviceProcessor`, `DeviceState` | Lazy processor instances; return defaults; original algorithms, not complete commercial parity |
| EQ-01 | Contour seven-band graphical EQ and live FFT | `DeviceArea`, `FilterCoefficients` | Drag frequency/gain, select Q/type; graph and processing use actual sample rate |
| MIDI-13 | Scheduled clip notes, piano and computer-keyboard audition | AudioOutput, Workspace, PianoRoll | Note velocity/pan/fine pitch; F6 letter mode; release on key-up/stop/seek; controller lanes still need full DSP automation |
| QA-05 | Routing, audition, synth/release, EQ and effects checks | AudioOutputTests | Offline deterministic checks plus native device drag, clip creation, EQ graph and modulation-assignment verification |

Update specific Info View help whenever changing these interactions. Built-in
metadata describes real devices; the Sounds category has no fabricated sample
presets. General project/device undo and full commercial parity remain future
work. Saving and external VST3 hosting are covered by PROJECT-01 and HOST-01/02.
