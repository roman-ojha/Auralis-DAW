# Auralis feature and maintenance register

Read this register at the start of every implementation task. Update affected rows
and contracts before completing every feature, fix, removal, or architecture change.
Verify source first: this is a maintained map, not proof that behavior was tested.
Last implementation update: 2026-09-21.

## Capability inventory

| ID | Capability and current behavior | Owner / source | Preserve and verify when changing |
| --- | --- | --- | --- |
| APP-01 | Native Windows x64 C++20/JUCE application; single instance; resizable window; compact QA argument | `src/main.cpp`, `CMakeLists.txt` | Native close and shutdown; 1100x700 minimum; no machine-wide settings changes |
| APP-02 | Original orbit icon in native window and executable resources | `assets/auralis-icon.png`, `assets/Generate-Icon.ps1`, `src/main.cpp`, CMake ICON_BIG/SMALL | Rebuild resources after asset changes; inspect title bar; Windows may cache executable/taskbar icons |
| MENU-01 | Integrated themed File, Edit, Create, Playback, View, Navigate, Options, Help menus | `src/ui/ApplicationMenu.h`, `Theme.*` | Functional commands share transport state; planned commands stay disabled; detach menu model before destruction |
| MENU-02 | View > Reset Layout restores outer panels, library divider, track headers, timeline scroll and mixer dock | `Workspace::resetLayout`, `Browser::resetLayout`, `Arrangement::resetLayout` | Does not reset transport, gains, selection, or pretend to save a project |
| TRANS-01 | Silent play/pause/stop, elapsed time, bar/beat position and user-selected loop | `Transport.*`, `model/SessionState.h`, `Workspace` timer | Pause preserves position; stop returns to start and disarms; loop preserves overshoot; message timer is never an audio clock |
| TRANS-02 | Tempo 20–300 BPM; supported time signatures; shared note/clip snap grid; record/metronome UI toggles | `Transport.*`, `constants/Design.h` | Signature denominator affects duration; snap affects MIDI edits; record/metronome produce no audio; menu toggle state matches toolbar |
| MON-01 | Silent logarithmic 20 Hz–20 kHz spectrum and silent track meters | `Transport::drawSpectrum`, `TrackRow::paint` | No fabricated signal; distinguish future measured audio from empty presentation |
| MON-02 | Real process CPU and resident memory in MiB | `platform/ProcessMetrics.*` | CPU normalized across logical processors, not DSP load; sampling is throttled |
| LIB-01 | Icon categories Instruments, Sounds, Effects; expandable subsections and placeholder selection | `Browser.*`, `constants/Library.h`, `assets/library.json` | Catalog entries are metadata, not plugins/audio; root detached before replacement/destruction; ownership transfers via addSubItem |
| LIB-02 | Search within current category; category change clears query; empty-results state | `Browser::filter`, `selectCategory` | Search name/description/section; no stale results or hidden old query; selection clears waveform for placeholders |
| HELP-01 | Persistent bottom-left Info View resolves pointer and keyboard focus context | `ContextHelp.h`, `Workspace::timerCallback` | Do not overwrite help while pointer is reading Info View; scroll long help; explicit descriptions for controls and HelpProvider for painted regions |
| WAVE-01 | Bottom-right library waveform from a chosen local WAV, AIFF or FLAC; filename and duration/channels | `WaveformPreview.*`, `constants/Preview.h` | Read-only inspection, not track import or playback; real peak envelope; empty placeholders have no fake waveform |
| WAVE-02 | Background decode, bounded buffer/peak memory, cancellation generation, error states | `WaveformPreview::load`, `Exchange`, `ThreadPool` | Maximum 10 min / 32 channels / 384 kHz; stale results never replace newer selection; no decoder work in paint/input; join worker before destruction; no retained user audio |
| ARR-01 | Four track lanes with MIDI clips on instrument tracks; selection updates blank device path | `Arrangement.*`, `Workspace.*` | Track identity/tint follow selection; independent clip editing; no audio/instrument loading |
| ARR-02 | Shared rotary gain -60 to +6 dB, combined mute/solo control and arm UI; double-click resets gain | `TrackRow`, `constants/Design.h` | Changes remain UI state; no audible gain/mute claim; arm and mute visually distinguishable |
| ARR-03 | Timeline ruler, horizontal scroll and vertical track scroll; title/zoom toolbar removed | `Arrangement`, `TrackRow` | Preserve valid scroll ranges, current signature divisions, clipping at lane bounds and compact usability |
| DEV-01 | Blank left-to-right device path tied to selected track | `DeviceArea` in `Workspace.*` | No plugin editors or signal processing; preserve empty-state explanation |
| LAY-01 | Browser, browser columns, track header and device-area splitters; double-click reset | `Theme::Splitter`, `PanelLayout`, `Browser`, `Arrangement` | Clamp min/max, never overlap or create negative bounds; fixed menu/transport heights; bottom strip removed |
| UX-01 | Shared dark palette, readable text, vector category icons, tooltips and keyboard names | `Theme.*`, `constants/Design.h` | Keep original visual language; no Windows default menu chrome; do not shrink text to hide overflow |
| QA-01 | CMake Release build and CTest transport/layout plus shared-channel/routing tests | `tests/StateTests.cpp`, `tests/MixerStateTests.cpp`, `Instructions/BUILD_AND_VERIFY.md` | Meaningful regression checks; historical UI evidence lives in `docs/VALIDATION.md` |

## Contextual help contract — every new control

### Mixer milestone inventory (2026-09-21)

| ID | Capability | Owner / source | Maintenance contract |
| --- | --- | --- | --- |
| MIX-01 | Arrangement/mixer icon switches after App Resources; also View menu | `Transport`, `Workspace`, `ApplicationMenu` | Switching never recreates channel state or changes selection; arrangement title/count/zoom controls remain removed until requested |
| MIX-02 | One channel instance per arrangement lane, shared controls and future source/device ownership | `model/MixerState.h`, `TrackRow`, `MixerStrip` | Stable TrackId; deque preserves addresses when sends append. No per-view copies of gain/mute/solo/media/device state. All mutations on message thread; one Workspace change callback refreshes views |
| MIX-03 | Channel strips with gain fader, pan, polarity icon, stereo separation/merge, combined mute/solo | `Mixer.*`, `MixerControls.*`, `Theme.*` | Left-click mute; Ctrl+right-click solo; S while focused for keyboard solo. Solo is additive and preserves explicit mute. Gain -60..+6 dB; pan -100 L..100 R; stereo -100 separated..0 original..100 merged. All UI state, no DSP |
| MIX-04 | Same rotary style for arrangement gain and mixer knobs | `Knob`, `Theme::drawRotarySlider` | Vertical drag, editable gain/send readouts, double-click defaults, focus/help support; do not duplicate parameter ownership |
| MIX-05 | Master strip and current-selected meter with dBFS scale | `MixerBody`, `drawSilentMeter` | Meter silence is -infinity, not the fader value; never fabricate audio activity. Master is destination-only |
| MIX-06 | Resizable right-docked sends, create/show/hide, horizontal/vertical scrolling | `Mixer`, `MixerBody`, `constants/Mixer.h` | Sends have no arrangement lane. Max 32 sends; creation defaults to Master output. Hiding preserves routes; references remain valid; detach viewports on destruction |
| MIX-07 | Selected-source routing arrows, cables, independent 0..100% send amount knobs | `MixerState::toggleRoute/canRoute/sendAmount`, `MixerStrip`, `MixerBody::paintOverChildren` | Allow arrangement-to-arrangement, arrangement-to-send, send-to-send and send-to-arrangement; reject self/unknown IDs, Master source and indirect cycles. Existing routes always removable. New routes unity gain; conceptual post-fader sends. Cables only for visible endpoints; Info View lists all destinations |
| MIX-08 | Deterministic shared-ownership, routing and boundary regression tests | `tests/MixerStateTests.cpp` | Test indirect cycles, route removal, stable channel addresses, source/device aliasing, invalid values, selection persistence and send cap |

The previous UI-owned track values have moved into MixerState. Future audio engine
work must consume a suitable real-time snapshot, never read this mutable UI model
from the audio callback. Routing is a validated graph of UI state, not an audio
graph executor. No audio summing, solo propagation, pan law or stereo DSP exists yet.

### Help requirements

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
- The process-session prototype has no project serialization or undo stack.
  Introduce explicit state/command boundaries before persistence/automation.

## Required lifecycle update

For each implementation task: identify affected IDs, inspect their contracts,
update/add/remove rows to match final behavior, update contextual help, update
PROJECT_STATE when scope changes, and record actual verification in VALIDATION.
Record unfinished work as unfinished with concrete limits. Keep IDs stable when
renaming source files. No instruction file guarantees compliance by every AI tool;
the root AGENTS.md is the entry point for compatible agents.

Still future: audio output/recording, media import into tracks, advanced MIDI tools,
instruments/effects DSP, plugin scanning/hosting, audio routing DSP, automation playback,
project save/load, undo/redo, export, recovery, distribution and installer.




## Clip editing milestone (2026-09-21)

| ID | Capability | Owner | Contract |
| --- | --- | --- | --- |
| MIDI-01 | Independent clip-local notes/events, repeated source loop, stable IDs | `MidiProject`, `constants/Editing.h` | Quarter-note ticks at 960 PPQ; copies and splits deep-copy content; no shared patterns; no audio thread access |
| MIDI-02 | Note drawing/painting/selecting/moving/resizing/erasing/muting/slicing | `PianoRoll` | Ctrl+right-drag selects per user request; right-drag erases swept notes; bounds 0..127 pitch and positive durations; edits stay inside source loop |
| MIDI-03 | Velocity, note pan/fine pitch, separate channel volume/pan/pitch events | `PianoRoll`, `MidiNote`, `MidiEvent` | Per-note values distinct from step automation; normalized channel pitch has no device bend-range claim; no DSP |
| MIDI-04 | Snap options, copy/cut/paste/duplicate, quantize, MIDI undo/redo | `MidiProject`, editor key handlers | Shared snap across both editors/top toolbar; letter shortcuts case-insensitive; history one pre-gesture snapshot, maximum 100; text editors keep keys |
| MIDI-05 | Clip create/open, edge trim/repeat, range cut/move, split | `ClipTimeline`, `Arrangement` | Instrument lanes only; cuts leave later time in place; fragments own data and retain loop phase; never overwrite another clip's notes through aliasing |
| MIDI-06 | Selected transport loop and ruler brace | `Arrangement`, `TransportState` | Loop in quarter-note beat units; wraps overshoot to chosen start; UI timing only |
| MIDI-07 | Shared horizontal zoom/scroll, vertical scroll/zoom, middle-drag pan | Editor components | Clamp ranges; preserve usable track headers, notes and controls at compact size; no unbounded grid drawing |
| HELP-02 | F1 separate shortcut window; Info View shortcut hints | `ShortcutWindow`, `Shortcuts.h`, controls | Maintain manual with handlers; explicitly document Auralis conflicts and unimplemented FL features; hide on close, Workspace owns lifetime |
| QA-02 | Deterministic clip/history/snap/loop tests | `tests/MidiTests.cpp` | Include independent note AND automation ownership, loop-phase trim, range move, undo/redo |

See `docs/DECISION-CLIP-EDITING.md`. Current composition is session-only; project
save/load and crash recovery remain unimplemented. Advanced FL generators, MIDI
recording/import/export, ghost channels, audition, slide/portamento and full FL
shortcut/feature parity are not claimed. MIDI undo does not undo mixer/transport.
