# Clip-based MIDI editing

The document uses integer ticks (960 per quarter note). Clip position/duration,
loop length, and source offset are separate. Each clip owns note/event vectors by
value; a copy or split has an independent identity and independent content. Edge
extension repeats that clip's source loop. A trim adjusts its offset; range cuts
preserve the surviving phase on each fragment. No global pattern aliases exist.

`MidiProject` is message-thread state, not a real-time event queue. `checkpoint`
records one pre-gesture snapshot; undo/redo is bounded to 100 snapshots. IDs are
monotonic across undo so stale IDs do not silently bind to a newly created object.
UI components re-resolve IDs after mutations instead of retaining clip pointers.
Undo covers MIDI edits only; mixer/transport parameters retain their existing state.

Piano-roll note velocity/pan/fine pitch belong to individual notes. Channel volume,
pan and pitch are separate clip-local step events. Volume is normalized 0..1;
pan/pitch are -1..1. Pitch bend range is deliberately undefined until instrument
integration. No event is dispatched to a plugin or audio device.

Arrangement range deletion leaves time in place. Moving a selected range extracts
independent fragments and moves them; it does not ripple other material. MIDI clips
are restricted to existing instrument lanes, not audio lanes or mixer-only sends.
Source loops are edited in the piano roll; the arrangement shows repeated instances.
The transport loop is a separate selection measured in quarter-note beats.

The FL reference workflows were researched in its official piano-roll and shortcut
manuals; Ableton's arrangement documentation informed independent clip editing.
The visual treatment uses Auralis styling. This is not full FL feature parity:
generators, scale/chord tools, recording/import/export, ghost channels, audition,
slide/portamento processing and MIDI output are still future work. Ctrl+right-drag
selects per the user's request, overriding FL's zoom gesture. See the in-app F1
manual (`constants/Shortcuts.h`) for the actual supported mapping.

Sources:
- https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/pianoroll.htm
- https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_shortcuts.htm
- https://www.ableton.com/en/manual/arrangement-view/

Limits: session-only document, no project file or recovery yet. No audio engine,
MIDI scheduling or per-device expression compatibility is implied. Composition
bounds and history limits are in `constants/Editing.h`.
