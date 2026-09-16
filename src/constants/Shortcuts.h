#pragma once
namespace auralis::editing
{
inline constexpr const char* shortcutManual = R"manual(AURALIS - SHORTCUT REFERENCE

PROJECTS & AUTOMATION
Ctrl+N - New project    Ctrl+O - Open .aup project
Ctrl+S - Save    Ctrl+Shift+S - Save As    Ctrl+Shift+R - Export Master WAV
F2 or double-click channel name - Rename shared arrangement/mixer channel
Shift+A - Show/hide grouped automation lanes and capture touched parameters
Automation: click to add points, drag to move; Alt-drag a segment to curve it.
Right-click point / Delete - Remove point; Ctrl+Z/Y - Undo/redo lane edits
Double-click lane name - Rename; On - Enable/bypass (manual value restored)
Alt+wheel in arrangement - Resize channel height
Device rack height is fixed; scroll horizontally to reveal devices.
Options > Plug-In Settings - Scan Windows x64 VST3 folders and star favorites.

TRANSPORT & VIEWS
Space - Start / stop and return to start (arrangement audio)
Ctrl+Space - Pause / resume at current position
Escape - Stop and return to start
R - Arm recording UI (no recording yet)
Ctrl+M - Toggle metronome UI (no sound yet)
L - Toggle transport loop
Home - Return transport position to start
F5 - Arrangement    F7 - Piano roll    F9 - Mixer
F1 - Open this reference window
F6 - Toggle computer keyboard instrument audition (A W S E D F T G Y H U J K O L P)
With Keys on, unmodified letter shortcuts are suspended; Space still controls transport.
DEVICES: drag ENV/LFO chips onto Prism knobs; click a chip for depth/removal.
EQ: drag numbered bands for frequency/gain; band page provides Q and type.

SHARED EDITING / NAVIGATION
Ctrl+Z - Undo clip edit    Ctrl+Y / Ctrl+Shift+Z - Redo
Ctrl+A - Select all    Ctrl+D - Deselect
Ctrl+C / X / V - Copy / cut / paste
Ctrl+B - Duplicate selection to the right
Delete - Delete selected notes, clips or time range
Backspace - Toggle Line / None snap
Alt while dragging - Bypass the selected snap grid
Wheel - Vertical scroll    Shift+wheel - Horizontal scroll
Ctrl+wheel or Page Up / Down - Horizontal zoom
Middle-button drag - Pan both axes
Ctrl+right-drag - Rectangle/time selection (Auralis choice)

PIANO ROLL
P - Draw    B - Paint    E - Select    D - Delete
C - Slice    T - Mute notes
Z - Drag a box to zoom in time
Shift+1 / 2 / 3 - Preset horizontal zoom
Shift+4 - Fit all notes    Shift+5 - Fit selection
Shift+I - Invert note selection
Ctrl+Shift+M / + Clip - New independent clip at the transport position
Click an empty editor grid - Create a clip and draw the first note
Double-click note / Enter with selection - Numeric note properties
Left-click grid - Add note (last touched note's length)
Drag note - Move selected notes
Drag either edge - Resize selected notes
Shift-drag existing note - Clone selected notes, then move the copies
Shift+left-click empty grid and drag - Add and resize
Right-click / right-drag - Erase notes under the cursor
Ctrl+left-drag - Select notes
Ctrl+Shift+drag - Add notes to selection
Arrows - Move selected notes by grid / semitone
Ctrl+Up / Down - Transpose selected notes by octave
Ctrl+Q - Quantize selected notes, or all if none selected
F / Shift+F - Cycle the control lane property
Alt+wheel over note - Adjust selected note property
Ctrl+Alt+wheel - Vertical note-row zoom
Double-click divider - Reset control lane height
Click piano key - Audition loaded instrument; Ctrl-click - Select that pitch
C then drag a line - Slice intersected notes on release
T then drag - Toggle mute on each crossed note once
Alt+V - Toggle ghost outlines from other clips overlapping this source loop
Ctrl+L - Quick legato    Ctrl+G - Glue touching notes    Ctrl+U - Chop to grid
Alt+Y - Reverse note timing    Alt+S - Strum chords (15 ticks per note)
Alt+A - Arpeggiate chord pitches by the current grid
Alt+R - Humanize (timing +/-12 ticks, velocity +/-8)
Tools menu also provides pitch flip, C major/minor pitch correction and triads.
These are bounded editing presets, not the full FL Studio generator dialogs.
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
Double-click empty instrument lane with either mouse button - Create/open one-bar MIDI clip
Ctrl+Shift+M / + MIDI clip - Create a clip at the timeline cursor
Double-left-click clip - Open its independent piano roll
Drag clip title - Move clip (instrument lanes only)
Drag either edge - Trim / extend; extension repeats the local loop
Drag clip body or empty lane - Select a time range across tracks
Ctrl+right-drag - Force time selection, including over a clip title
Drag inside highlighted range - Move its clip fragments
Delete time range - Cut out content; independent left/right fragments remain
Ctrl+E - Split clips on clicked track at the last cursor position
Ctrl+L - Set and enable loop from time selection
Drag arrangement ruler - Set and enable transport loop boundaries
Drag loop edges - Resize loop; drag its top brace - Move the loop
Shift-drag ruler - Replace the loop range
With ruler focused: Left/Right - Move by grid; Up/Down - Move by loop length
Ctrl+Left/Right - Resize by grid; Ctrl+Up/Down - Double/halve loop length
No ripple deletion: later clips keep their positions.
Copies and split fragments own their notes/events; there are no shared patterns.

MIXER & PANELS
Click track power - Mute/unmute
Ctrl+right-click track power - Toggle solo
S with power button focused - Toggle solo
Drag knobs vertically; double-click resets their default
Click destination arrow - Toggle send from selected source
Drag send amount knob - Change that route's level
Drag left edge of send dock - Resize; double-click resets
View > Reset Layout - Restore panel geometry (retains composition)
Tab / Shift+Tab - Move keyboard focus; Info View explains the control

AUDIO LIBRARIES & CLIPS
Add Library - Link a local sample folder; links persist after restart
Unlink folder - Remove a sidebar link; files and imported clips stay intact
Click a sample - Preview; click the small waveform to replay
Drag sample onto Audio track - Import and open the editor below
Click audio clip - Open clip editor; Play clip auditions its settings
Drag clip header / edges - Move / trim or extend
Drag amber handles below clip header - Fade in / out
Ctrl+B - Independent duplicate; Ctrl+E - Split at timeline cursor
Gain, pitch, reverse, loop, source start/end and fades belong to each copy
Pitch uses resampling (changes speed too); no pitch-preserving warp yet
Escape / transport Stop - Stop arrangement playback and sample audition

COMPATIBILITY & CURRENT LIMITS
Based on documented FL Studio/Ableton workflows, with unified Auralis choices.
Ctrl+right-drag selects here; FL uses that gesture for zoom.
FL R/Space/F5/F7/F9 and Ableton Ctrl+L/Ctrl+E are combined deliberately.
Text entry retains normal typing/editing keys. MIDI and audio clip edits have session undo;
mixer and transport changes are not part of MIDI undo. Projects save/open as .aup. Automation lanes have separate undo; general project undo is not implemented.
Full FL generator dialogs (including Riff Machine), arbitrary scale libraries,
MIDI recording/import/export, editable ghost channels and slide/portamento remain unfinished.
Built-in instruments play notes; full FL feature/shortcut parity is not implemented.
)manual";
}



