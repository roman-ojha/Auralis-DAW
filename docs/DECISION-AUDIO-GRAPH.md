# Decision: shared audio graph and embedded built-ins (2026-09-28)

## Context and decision

The previous audio milestone rendered track 4 directly to Master while mixer
routes and meters were visual placeholders. The user authorized real instruments,
effects, routing, meters and drag-created tracks. MixerState remains the single
channel/device owner. Arrangement and mixer refer to stable TrackId values;
visual rows no longer assume that track IDs are row numbers.

AudioOutput builds a topologically ordered render plan on the message thread.
Sources feed their channel device chain, gain/balance/width/polarity/mute, then
post-fader routes. Returns feed their own effects and routes. Master processes the
sum, including browser audition injected at Master. Audio-clip audition uses its
own track. The existing DAG rule blocks all feedback cycles. Solo preserves the
upstream sources and downstream destinations of the soloed channel.

DeviceState owns parameters and bounded modulation assignments. DeviceProcessor
owns DSP history, allocated only on first insertion/publication. Immutable plans
share processors across parameter edits; only the callback mutates their DSP
history. The existing hazard protocol retains plans and processors during callbacks
and releases ownership on the message thread. There are no shared_ptr copies,
locks, file operations, GUI calls or dynamic containers mutated in rendering.

Built-ins are compiled into the application, with lazy instances and embedded
editors. This is not dynamic DLL loading or external VST hosting. That alternative
requires a separately scoped scanner, binary lifecycle and failure isolation.
No third-party DSP dependency, proprietary plugin code or copied assets were added.

## Devices and limits

- Prism: 24 voices, three oscillators, sine/saw/pulse/triangle, up to eight unison
  voices per oscillator; detune/blend/phase/range/pan/level. Three AHDSR envelopes,
  four sine LFOs, bounded drag assignments and editable signed depth. A per-voice
  low-pass filter provides cutoff/resonance/pan/drive/fat/mix.
- Atlas: immutable decoded sample playback with root pitch, level, attack/release.
- Bloom: stereo feedback reverb with predelay, size, modulation, diffusion-speed
  control, low/high cuts, decay and independent dry/wet.
- Contour: seven biquads; each supports bell, low/high shelf, low/high pass,
  frequency, gain and Q. Graph drag changes frequency/gain. Graph reference is
  48 kHz; processing coefficients use the device's actual rate.
- Echo, Gravity, Drive, Flange, Phase and Chorus supply basic delay, compression,
  saturation and modulation effects. These are initial original DSP designs, not
  Serum algorithms or full commercial-plugin feature parity.
- 96 total channels including Master/returns, 12 devices per channel, 32 devices
  per session, 32 modulation routes per synth, 16,384 scheduled MIDI notes per
  render plan, 128 audio clips. Delay workspaces are allocated off the callback.
- Meters are post-fader stereo sample peaks with 300 ms exponential decay, in
  dBFS. They are not RMS, loudness or true-peak meters. Intermediate buses retain
  headroom; hardware output clamps to [-1,1]. Channel gain/mute transitions and
  clip gain changes ramp across a block. Not every DSP parameter is smoothed.

## Interaction

Browser device drag descriptions use device kind IDs. Dropping an instrument or
audio file in the persistent bottom drop zone creates a compatible track and
Master route; effects require an existing channel. Audio files on Audio lanes
create independent clips; files dropped in a MIDI channel's device area load
Atlas. A source must be removed before replacing it with a different source.
Clicking an audio clip opens the clip editor; track headers, empty lanes and mixer
selection show the shared device chain. Effects append right. F6 enables computer
keyboard audition; letter editing shortcuts are suspended while enabled.

## Explicit remaining work

No external plugin hosting, project serialization/recovery, device/mixer undo,
MIDI recording, quality pitch-preserving warp, user wavetable import/editor,
spectral/granular engines, Serum preset compatibility, arbitrary LFO drawing,
modulation matrix UI, oversampling, latency compensation or master FFT. MIDI
channel controller lanes are not yet fully automated through the DSP graph;
note velocity/pan/fine pitch are applied. Range selections move in time; individual
clip headers can move across compatible lanes. Browser audition currently stops
arrangement playback. Device bypass and filter edits need broader click-quality
and sample-rate/device-loss profiling. The keyboard uses key-press capture with an 80 ms minimum audition and UI-held-key polling, not a
sample-timestamped hardware MIDI input. These are documented limits, not parity.

## References and verification

Workflow references: [Ableton mixing](https://www.ableton.com/en/manual/mixing/)
and [device chains](https://www.ableton.com/en/manual/working-with-instruments-and-effects/).
The [Serum manual](https://xferrecords.com/manual/serum-2/docs) is a behavior
reference; Auralis uses its own names, layout and processing.
See the dated entry in VALIDATION.md for checks actually performed.

