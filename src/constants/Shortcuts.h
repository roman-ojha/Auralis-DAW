#pragma once
namespace auralis::editing
{
inline constexpr const char* shortcutManual = R"manual(AURALIS — SHORTCUT REFERENCE

TRANSPORT & VIEWS
Space — Start / stop and return to start (silent preview)
Ctrl+Space — Pause / resume at current position
Escape — Stop and return to start
R — Arm recording UI (no recording yet)
Ctrl+M — Toggle metronome UI (no sound yet)
L — Toggle transport loop
Home — Return transport position to start
F5 — Arrangement    F7 — Piano roll    F9 — Mixer
F1 — Open this reference window

SHARED EDITING / NAVIGATION
Ctrl+Z — Undo MIDI edit    Ctrl+Y / Ctrl+Shift+Z — Redo
Ctrl+A — Select all    Ctrl+D — Deselect
Ctrl+C / X / V — Copy / cut / paste
Ctrl+B — Duplicate selection to the right
Delete — Delete selected notes, clips or time range
Backspace — Toggle Line / None snap
Alt while dragging — Bypass the selected snap grid
Wheel — Vertical scroll    Shift+wheel — Horizontal scroll
Ctrl+wheel or Page Up / Down — Horizontal zoom
Middle-button drag — Pan both axes
Ctrl+right-drag — Rectangle/time selection (Auralis choice)

PIANO ROLL
P — Draw    B — Paint    E — Select    D — Delete
C — Slice    T — Mute notes
Left-click grid — Add note (last touched note's length)
Drag note — Move selected notes
Drag right edge — Resize selected notes
Shift+left-click empty grid and drag — Add and resize
Right-click / right-drag — Erase notes under the cursor
Ctrl+left-drag — Select notes
Ctrl+Shift+drag — Add notes to selection
Arrows — Move selected notes by grid / semitone
Ctrl+Up / Down — Transpose selected notes by octave
Ctrl+Q — Quantize selected notes, or all if none selected
F / Shift+F — Cycle the control lane property
Alt+wheel over note — Adjust selected note property
Ctrl+Alt+wheel — Vertical note-row zoom
Double-click divider — Reset control lane height
Paste inserts at the last grid click; duplicating beyond the source loop
extends its length to fit the new notes.

CONTROL LANE
Velocity: 1–127. Note pan: -100…100. Fine pitch: -1200…1200 cents.
Drag stems to edit notes. Selected notes constrain affected stems.
Channel volume: 0–100%; channel pan: -100…100%; pitch: normalized -1…1.
Click/drag channel lanes to insert step events; right-drag removes events.
Events are owned by this clip. Pitch bend range awaits an instrument.
These values do not produce sound until MIDI/audio processing is implemented.

ARRANGEMENT
Double-right-click empty instrument lane — Create one-bar MIDI clip
Double-left-click clip — Open its independent piano roll
Drag clip title — Move clip (instrument lanes only)
Drag either edge — Trim / extend; extension repeats the local loop
Drag clip body or empty lane — Select a time range across tracks
Ctrl+right-drag — Force time selection, including over a clip title
Drag inside highlighted range — Move its clip fragments
Delete time range — Cut out content; independent left/right fragments remain
Ctrl+E — Split clips on clicked track at the last cursor position
Ctrl+L — Set and enable loop from time selection
Drag arrangement ruler — Set and enable transport loop boundaries
No ripple deletion: later clips keep their positions.
Copies and split fragments own their notes/events; there are no shared patterns.

MIXER & PANELS
Click track power — Mute/unmute
Ctrl+right-click track power — Toggle solo
S with power button focused — Toggle solo
Drag knobs vertically; double-click resets their default
Click destination arrow — Toggle send from selected source
Drag send amount knob — Change that route's level
Drag left edge of send dock — Resize; double-click resets
View > Reset Layout — Restore panel geometry (retains composition)
Tab / Shift+Tab — Move keyboard focus; Info View explains the control

COMPATIBILITY & CURRENT LIMITS
Based on documented FL Studio/Ableton workflows, with unified Auralis choices.
Ctrl+right-drag selects here; FL uses that gesture for zoom.
FL R/Space/F5/F7/F9 and Ableton Ctrl+L/Ctrl+E are combined deliberately.
Text entry retains normal typing/editing keys. MIDI edits have session undo;
mixer and transport changes are not part of MIDI undo. No project save/load yet.
Advanced FL generators (Riff Machine, arpeggiator, strum, chord/scale tools),
MIDI recording/import/export, audition, ghost channels, slide/portamento DSP,
and full FL feature/shortcut parity are not implemented in this editor.
)manual";
}
