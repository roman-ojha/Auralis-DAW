# Project state and source map

Baseline recorded: 2026-09-21. Verify the source before relying on this snapshot.
Update this file after meaningful completed milestones, not every tool invocation.

## Product and current scope

**Auralis** is a Windows C++20/JUCE DAW project, currently a UI/UX prototype.
The user wants incremental development and an original, responsive interface.
Do not interpret the long-term DAW ambition as permission to build future features
in a task that only requests UI or maintenance work.

Implemented UI:

- Integrated themed application menus; working preview transport commands and
  View > Reset Layout. Future commands are disabled. Old branding/status strips removed.
- Persistent contextual Info View for hovered/focused controls and painted areas.
- Read-only local WAV/AIFF/FLAC waveform preview, decoded on a background worker;
  no playback or arrangement import. Limited to 10 minutes, 32 channels and 384 kHz.
- Original Auralis native-window and executable icon resources.

- Fixed-height transport with play/pause/stop preview, recording-arm toggle,
  tempo, signature, metronome toggle, loop preview, quantization preference, timer.
- Silent 20 Hz-20 kHz spectrum presentation; real application CPU and resident RAM
  measurements. App CPU is normalized across logical processors, not DSP load.
- Two-column browser with icon categories Instruments, Sounds, Effects; expandable
  subsections; searchable dummy metadata; selection indication; internal divider.
- Four arrangement tracks with shared rotary gain and a combined mute/solo
  button, arm UI, selection and scrolling. Arrangement title/count/zoom toolbar removed.
- Mixer view with master/current silent meters, channel faders, pan, stereo
  separation/merge and polarity icons. Icon view switches follow App Resources.
- Independent right-docked send tracks, create/show/hide, destination arrows,
  routing cables and independent send-level knobs. Validated UI routing rejects
  feedback; no audio graph or DSP is running. Normal lanes and mixer strips share
  one channel object, including future source/device containers.
- Clip-based piano roll: note draw/paint/select/move/resize/erase, mute/slice,
  velocity/pan/fine pitch, clip-local channel volume/pan/pitch events, shared snap,
  scrolling/zoom, copy/cut/paste/duplicate and bounded MIDI undo/redo.
- Arrangement MIDI clip creation, trim/repetition, independent copies/fragments,
  multi-track time selection, range deletion/move and selected transport looping.
- Resizable send dock; icon transport; F1 shortcut manual in its own window.
- Blank device path reflecting the selected track, without plugin editors.
- Resizable browser, browser columns, track headers, and device area; reset controls.

Not implemented: audio engine, real recording/playback, adding/importing tracks
or media, plugin scanning/hosting, instruments/effects DSP, project persistence,
general project undo/redo, export, installer/update service, or a production release process.
Current UI state lasts only for the process session.

## Source map

| Path | Responsibility |
| --- | --- |
| `CMakeLists.txt` | Application/test targets, pinned JUCE dependency, embedded catalog |
| `src/main.cpp` | JUCE application/window lifetime and compact QA argument |
| `src/constants/Design.h` | Shared design metrics, palette, supported UI ranges |
| `src/constants/Library.h` | Browser category names, kinds, accents |
| `src/model/SessionState.h` | UI-preview transport state and bounded panel layout |
| `src/ui/Theme.*` | Look-and-feel, drawing helpers, draggable splitter |
| `src/ui/Browser.*` | Two-column catalog navigation and subsection tree |
| `src/ui/Arrangement.*` | Empty lanes, selection, track controls, grid and scroll |
| `src/model/MixerState.h` | Stable channel ownership, shared values, selection and acyclic send graph |
| `src/ui/Mixer.*` | Mixer strips, pinned master/current meter, send dock and routing cables |
| `src/ui/MixerControls.*` | Shared rotary knobs, vector icon buttons, mute/solo gesture |
| `src/constants/Mixer.h` | Mixer layout and routing limits |
| `src/model/MidiProject.h` | Clip-local notes/events, deep-copy fragments, bounded MIDI history |
| `src/ui/PianoRoll.*`, `ClipTimeline.*` | Note/control editor and arrangement clip/time-range gestures |
| `src/ui/ShortcutWindow.h`, `src/constants/Shortcuts.h` | Separate shortcut reference window and its maintained content |
| `tests/MidiTests.cpp` | Clip independence, repetition/trim, range edits, history, snap and loop |
| `tests/MixerStateTests.cpp` | Shared ownership, routing and parameter boundary checks |
| `src/ui/Transport.*` | Header controls and resource/spectrum presentation |
| `src/ui/Workspace.*` | Composition, main splitters, blank device area, UI timer |
| `src/ui/ApplicationMenu.h` | Integrated menus, working commands and disabled future actions |
| `src/ui/ContextHelp.h` | Help resolution, custom region interface and Info View |
| `src/ui/WaveformPreview.*` | Async local audio inspection and waveform display |
| `src/constants/Preview.h` | Preview bounds, file filters and panel dimensions |
| `assets/Generate-Icon.ps1` | Reproducible original Windows icon source |
| `src/platform/ProcessMetrics.*` | Windows application CPU and memory sampling |
| `assets/library.json` | Embedded placeholder catalog; not real device/sample files |
| `tests/StateTests.cpp` | Transport timing, input boundaries, layout regression checks |
| `docs/VALIDATION.md` | Historical automated and interactive validation notes |

## Important implementation limits

- The UI timer is not an audio clock. Do not reuse it for sample-accurate processing.
- Channel and routing UI state now live in MixerState. Other controls retain
  prototype state. Add persistence/undo/automation and real-time boundaries before
  engine integration; the mutable UI model is not audio-thread safe.
- Tree item ownership transfers to JUCE; detach the tree root before replacing or
  destroying its owner. Respect model/component shutdown ordering.
- Existing files contain local layout literals. Consolidate meaningful/repeated
  values when touching them; do not claim every literal is already centralized.
- Accessibility names and some focus behavior exist; full accessibility,
  cross-DPI, audio performance, and third-party compatibility are not certified.
- Build output and `external/` are ignored. The repository folder may still be
  called `Roman DAW`; do not rename the directory or delete old build artifacts
  simply to make its name match the product.

The next feature is determined by the user. This snapshot is not an instruction
to begin any of the unimplemented subsystems.

Read and maintain [FEATURE_MAP.md](FEATURE_MAP.md) for the complete capability
inventory and per-feature invariants on every future implementation task.

