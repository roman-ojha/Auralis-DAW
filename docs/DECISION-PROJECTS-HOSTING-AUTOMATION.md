# Projects, analysis, VST3 and automation — 2026-09-28

## Decision

The user explicitly requested persistence, export, external hosting, automation
and richer device views. Extend the existing shared MixerState/MidiProject model;
do not introduce a second document model per editor. Other DAWs are interaction
references, not imported assets or a claim of commercial feature parity.

ProjectFile owns version-1 Auralis `.aup`: a bounded XML manifest followed by
CRC-checked, deduplicated, embedded stereo float PCM. A temporary file is flushed
and loaded back through validation before atomic replacement. Parsing constructs
a separate snapshot before altering the active document. Unknown device/channel
content and unsupported versions fail rather than silently discarding data.
Source files remain unchanged. Plugin binaries are referenced, not embedded;
unavailable plugin state, stable parameter IDs and automation are retained.
Metadata is capped at 16 MiB and embedded PCM at 512 MiB. This is not autosave or
crash recovery, and undo history is not serialized. Editor positions are optional
version-1 fields so files saved by the first intermediate build still open.

ProjectActions captures on the message thread; native plugin state capture briefly
suspends rendering and waits asynchronously for the active callback to finish.
File writing/reading and offline WAV rendering run on the owned worker. Exports
instantiate independent native plugins; the UI retains ownership until the next
export or shutdown so final destruction never occurs on the render worker.
Offline export stops source scheduling at the selected end and renders effect
tails. Loop exports start fresh without pre-roll. No dither, stems or normalization.

AudioOutput now evaluates the routing DAG in at most 64-sample chunks. Native
plugins receive actual stereo blocks and sample-offset MIDI. Built-in automation
is evaluated per sample; native parameter automation is applied per chunk. Plans
retain immutable base values and private render copies. Graph reclamation remains
off the callback. Native plugin internals are outside Auralis's real-time guarantee.
Plugin latency compensation, sidechains and multichannel buses are not implemented.

SignalAnalysis is a bounded SPSC queue: audio writes measured samples and drops
analysis frames when full; UI consumes and computes Hann-windowed 2048-point FFTs.
96 logarithmic bins cover 20 Hz–20 kHz at -90..0 dBFS. The maximum of independent
left/right magnitudes avoids anti-phase cancellation. Gravity history uses real
input/output peaks and gain reduction. These are not true-peak/RMS meters.

NativePlugin wraps JUCE VST3 instances, stable parameter IDs, state blobs, editor
lifetime, transport position and parameter notifications. An isolated scanner
helper has a 20-second per-candidate timeout, cancellation and persistent quarantine.
Scanning is explicit. Windows x64 VST3/stereo only; VST2 requires an absent legacy
SDK. CLAP, AU and 32-bit bridging are unsupported. Runtime instances are in-process.
The bundled JUCE revision remains 29396c22c93392d6738e021b83196283d6e4d850;
its bundled Steinberg VST3 SDK has the MIT license (Copyright 2025 Steinberg).
JUCE's own AGPL/commercial terms still apply; this is not distribution clearance.

AutomationLane stores normalized points with per-segment curvature, target ranges,
stable device identity, enabled state and user name. Shift+A enables capture and
shows grouped rows. Master/return lanes are included without creating clip/mixer
channels. The renderer ignores missing targets and preserves their saved data.
Lane edit history is bounded and independent of MIDI history. Full commercial
read/touch/latch modes, automation clips with independent looping, rectangular
point-range manipulation and general project undo remain future work.

## Validation and next work

See the dated entry in VALIDATION.md. HostFixture is repository-owned test code,
not an installed plugin or a claim that commercial plugins all work. Further work:
third-party compatibility matrix, PDC, crash recovery, worst-case DSP profiling,
recording, advanced warp and complete project-wide undo. No machine settings or
file associations are changed by this milestone.
