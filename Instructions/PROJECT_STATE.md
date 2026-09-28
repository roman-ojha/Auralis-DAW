# Current project state and source map

Documentation reconciled with source: 2026-09-29. Last recorded build/native
verification: 2026-09-28 in [VALIDATION](../docs/VALIDATION.md). This documentation
update did not rebuild or retest the application. Verify source before extending it.

## Product and implemented capabilities

**Auralis** is a native Windows x64 C++20/JUCE/CMake DAW prototype. It now has real
sample and instrument playback, a shared mixer graph, embedded devices, VST3
hosting, project persistence and automation. It is not a production-certified DAW
or a complete implementation of FL Studio, Ableton Live or Serum.

- **Workspace:** themed menus and vector controls, contextual Info View, F1 shortcut
  manual, arrangement/piano/mixer views, process CPU and resident memory. Browser,
  track headers, send dock and audio editor have bounded resize/scroll controls.
  The device rack has fixed height, clamped to available space, with scrolling.
- **Library:** Instruments, Sounds, Effects and Plug-Ins categories; persistent
  linked sample folders, nested browsing/search, async WAV/AIFF/FLAC decoding,
  waveform preview and click-to-replay. Drag audio or an instrument into the
  bottom drop area to create a linked arrangement/mixer channel.
- **MIDI:** independent clip-local notes and events; either-button double-click
  creation; note draw/paint/move/resize/select/erase/mute/slice; note properties,
  velocity/pan/fine pitch, controller lanes, shared snap, clone/copy/paste,
  transforms, bounded history, ghost outlines and zoom/scroll. Instrument notes
  play; piano keys and F6 computer keys audition loaded instruments. Controller
  lane editing is not a claim of complete controller-event DSP support.
- **Arrangement/audio:** time-range selection, visible selected clip portions,
  move/delete/split, independent copies/fragments, MIDI repetition, transport
  loop editing, audio waveforms and draggable fades. The lower audio editor
  changes region, gain, resampling pitch/speed, reverse, looping and mute.
  Ctrl+wheel zooms time; Alt+wheel changes track height. F2/double-click renames
  the shared channel. Restored rows preserve clip and automation hit/paint order.
- **Mixer/engine:** dynamic channels, gain, pan, stereo width, polarity, mute/solo,
  post-fader sends/returns and Master run through an acyclic processing graph.
  Browser audition enters Master; clip audition enters its channel. Track/mixer
  meters show measured sample peaks. The audio sample clock drives playback;
  the UI timer polls it. Record-arm and metronome toggles do not record or click.
- **Built-ins:** Prism three-oscillator synth with unison, three envelopes, four
  LFOs, drag modulation and filter; Atlas sample instrument; Bloom reverb,
  Contour seven-band EQ, Echo delay, Gravity compressor, Drive, Flange, Phase and
  Chorus. DSP instances are created as needed; built-in code is compiled into
  the executable. Editors are embedded, with wide Prism tabs/modulation badges,
  a large Contour spectrum and side controls, Gravity history and expanded Echo.
  Master/device FFT displays use actual audio; meters are not true-peak/RMS.
- **External plugins:** explicit folder scanning, stars, persistent catalogue and
  quarantine, Plug-Ins browser, rack cards and separate native/generic editors.
  Windows x64 VST3 only. Scanning uses a separate helper; playback hosts plugins
  in-process. Stable parameter IDs and state blobs support save/restore and
  automation. Keep AuralisPluginScanner.exe beside Auralis.exe.
- **Automation:** Shift+A shows grouped child lanes and enables capture of touched
  channel/device/native parameters. Lanes retain plugin/parameter identity, can
  be renamed/bypassed, and support point editing, segment curvature, copy/cut/paste
  and bounded lane history. Master/return lanes have no extra mixer channel.
  Enabled lanes play even when hidden; missing targets remain saved but ignored.
- **Projects/export:** New/Open/Save/Save As, dirty-state confirmation and versioned
  Auralis `.aup` files. Save includes MIDI, audio settings and embedded PCM,
  channels/routes, devices/native state, automation, transport settings and
  selected editor/view positions. Open restores stopped playback. WAV export
  renders an independent Master graph with range, rate, PCM depth and tail options.

## State, persistence and ownership

Workspace owns the mutable message-thread models: MixerState, MidiProject,
transport and views. Editors must share these models rather than duplicate state.
AudioOutput consumes published plans with private processing state; old plans and
processors are reclaimed off the callback. Never read mutable UI models directly
from processing. Native plugin state capture suspends rendering and waits for the
active callback before message-thread capture. Project I/O and export use workers;
UI ownership keeps native plugin final destruction off those workers.

ProjectFile format version 1 contains a bounded XML manifest and deduplicated,
CRC-checked stereo float PCM. Save validates a temporary file before replacement;
load validates a separate snapshot before replacing the session. Missing native
plugin state is preserved; binaries are referenced, not embedded. Source samples
are never overwritten. Library links/plugin folders/favorites are per-user
settings, separate from project content. No file association is installed; this
Auralis format is not compatible with Audacity files sharing the `.aup` suffix.

Limits are authoritative in `src/constants/`: 16 MiB project metadata, 512 MiB
embedded media, 16 MiB per native state blob, 4096 native parameters, 32 automation
lanes per channel and 1024 points per lane. Device limits are 96 channels, 12
devices per channel and 32 total devices. Check the constants before changing limits.
Undo history, all UI selections/device pages/window geometry and plugin binaries
are not a complete part of project restoration. Do not promise an exact desktop
snapshot or recovery from unsaved changes.

## Source ownership map

| Area | Primary files |
| --- | --- |
| Startup, targets, pinned JUCE, scanner/fixture | `src/main.cpp`, `CMakeLists.txt` |
| Composition, commands, lifetime, help | `src/ui/Workspace.*`, `ApplicationMenu.h`, `ContextHelp.h` |
| Channels, sends, devices, automation | `src/model/MixerState.h`, `DeviceState.h`, `Automation.h` |
| MIDI/audio documents and transforms | `src/model/MidiProject.h`, `MidiEditing.h`, `AudioClip.h` |
| Transport and saved view fields | `src/model/SessionState.h`, `SessionView.h` |
| Save/load schema and UI workflow | `src/model/ProjectFile.*`, `src/ui/ProjectActions.cpp` |
| Sample-clock graph, scheduling, audition/export rendering | `src/audio/AudioOutput.*` |
| Built-in DSP and signal analysis | `src/audio/DeviceProcessor.*`, `SignalAnalysis.h` |
| Host interface and VST3 instances/editors | `src/audio/HostedProcessor.h`, `PluginHost.*` |
| Scan/preferences and host insertion | `src/platform/PluginScanner.cpp`, `src/ui/PluginSettings.*`, `PluginActions.cpp` |
| Arrangement, MIDI/audio and automation editors | `src/ui/Arrangement.*`, `ClipTimeline.*`, `PianoRoll.*`, `AudioEditor.*`, `AutomationLaneView.h` |
| Mixer/device panels | `src/ui/Mixer.*`, `MixerControls.*`, `DeviceArea.*` |
| Browser, decoding and preview | `src/ui/Browser.*`, `WaveformPreview.*` |
| Theme, transport, resource monitoring | `src/ui/Theme.*`, `Transport.*`, `src/platform/ProcessMetrics.*` |
| Policy bounds and shortcut manual | `src/constants/`, `src/ui/ShortcutWindow.h` |
| Built-in catalogue and original icon | `assets/library.json`, `assets/Generate-Icon.ps1` |
| Regression suites | `tests/`, targets listed in `CMakeLists.txt` |

## Known remaining work

Audio/MIDI recording, audible metronome, advanced MIDI I/O/generators/slide behavior,
full piano controller-event processing, pitch-preserving warp/transient tools,
clip envelopes and full commercial synth/effect parity remain incomplete.
Hosting lacks VST2 DLL/CLAP/AU/32-bit bridges, runtime isolation, plugin delay
compensation, sidechains/multichannel buses and complete transport metadata.
Automation lacks full read/touch/latch modes, independently looping automation
clips and rectangular point-range workflows. General project/device/mixer undo,
autosave/crash recovery, format migrations beyond v1 and comprehensive UI-state
restoration remain future work. Export lacks stems, dither, normalization and
pre-roll. Broad plugin compatibility, cross-DPI/accessibility, device-loss and
worst-case real-time profiling are not certified. No installer/release pipeline.

## Continue work in a new chat

Read [README](README.md), [ENGINEERING](ENGINEERING.md), this file and
[FEATURE_MAP](FEATURE_MAP.md), plus the build/audio guides as appropriate.
Inspect affected source and tests and preserve existing edits, projects and
preferences. A previously running window or permission to discard an older
session does not prove that new edits are disposable. The next feature is chosen
by the user; this remaining-work list is not authorization to implement it.

Historical decisions and actual verification remain in:

- [Clip editing](../docs/DECISION-CLIP-EDITING.md)
- [Sample clips](../docs/DECISION-AUDIO-CLIPS.md)
- [Shared graph and built-ins](../docs/DECISION-AUDIO-GRAPH.md)
- [Projects, hosting and automation](../docs/DECISION-PROJECTS-HOSTING-AUTOMATION.md)
- [Dated validation evidence](../docs/VALIDATION.md)

Older decision notes describe their milestone, not today's complete feature set.
The 2026-09-28 final record reports eight passing Release suites, native normal/
compact checks, live analysis, saved-project restoration and non-silent WAV export.
These are recorded results, not new results from this documentation update.
