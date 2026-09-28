# Audio libraries and clip playback

## Decision (2026-09-27)

The user requested persistent sample-pack folders, audible preview, audio import,
independent duplicate clips, a lower clip editor, fades and clearer arrangement
selection. This explicitly extends the prior UI-only milestone. It does not make
this prototype a complete Ableton Live implementation.

Use existing JUCE modules (audio_devices added for output) and keep file decode
on WaveformPreview's cancellable worker. Browser uses DirectoryContentsList's
worker and FileTreeComponent for lazy nested browsing; per-user PropertiesFile
stores folder links. Settings writes report failure. Imported/previewed source
files are never modified, moved, uploaded or executed.

AudioData owns validated stereo float PCM and cached peaks. AudioClip stores an
immutable shared media reference and independent value settings. MidiProject now
owns both clip types and their shared undo snapshots. Duplicate and split copy
settings; splitting adjusts source phase. Sharing immutable PCM avoids duplicating
large sample buffers. History remains session-only and retains media references.

AudioOutput opens only the default two-channel output, with no input channels.
The UI publishes immutable plans; the callback reports its sample-count clock.
An atomic hazard pointer prevents UI reclamation of plans while a callback uses
them. Callback access never increments shared ownership, decodes files, allocates,
waits for locks or calls GUI code. Plans and media are destroyed on the message
thread. Output mixes a bounded 128 clips and clamps float samples to [-1,1].
Gain interpolates across a callback block and starts ramp for 5 ms. This is a
basic renderer, not an anti-aliased studio-grade time/pitch engine or limiter.

The direct arrangement path honors Audio track 4 and Master gain, mute, balance,
polarity, and arrangement solo. Existing send routing and stereo width controls
are NOT connected to this graph. Their UI and help must keep that distinction.
Native device sample-rate changes update the callback rate; device errors are
reported when output is opened. Device selection, measured dropout monitoring,
broad device-loss testing and a full routing graph remain future work.

## Supported workflow

Add Library; open folders; click a mono/stereo WAV, AIFF or FLAC to audition.
Click the small waveform to replay. Drag the file onto Audio track 4; select its
clip to reveal the lower editor. Drag the lower panel divider to resize. Play clip
(or click its waveform) auditions current settings. Arrangement transport uses
the sample clock. Preview pauses arrangement playback; Stop/Escape stops audition.

Clip editing: gain, pitch by resampling (speed changes too), reverse, mute, looped
source region, source start/end, linear fade-in/out. Amber handles in arrangement
edit fades. Header drag moves; edge drag trims/extends. Ctrl+B makes an independent
copy; Ctrl+E splits; time selection cuts/moves fragments. Amber overlays identify
selected clip portions, with resize/move/select cursors. Audio drops on MIDI lanes
are rejected. Raw sample lengths are measured in seconds; arrangement extent uses
beat ticks, so changing tempo may change the portion of an unwarped source heard.

## Explicit remaining scope

No full Ableton parity: no pitch-preserving warp modes/markers, clip envelopes,
transient detection/editing, groove extraction, audio-to-MIDI, consolidate/export,
recording, more audio tracks, plugins, instruments, live sends, real meters or
project persistence. Library links persist independently of the unsaved document.

References consulted:
- https://www.ableton.com/en/manual/clip-view/
- https://www.ableton.com/en/manual/arrangement-view/
- https://www.ableton.com/en/manual/audio-clips-tempo-and-warping/

The implementation follows clip-local properties referencing source media. It
uses original Auralis UI and its own basic algorithms, not Ableton's assets or
proprietary warp implementation. Actual test evidence belongs in VALIDATION.md.
