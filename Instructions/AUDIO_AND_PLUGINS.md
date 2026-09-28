# Audio, MIDI, and plugin engineering

These rules govern the implemented audio graph, MIDI playback, built-in devices
and VST3 host, and any authorized extensions. They are engineering requirements,
not proof of complete compliance. Read ENGINEERING.md and PROJECT_STATE.md first.
Do not implement the remaining-work list as an unsolicited backlog.

## Real-time boundary

- Treat every audio callback as a deadline. No heap allocation/deallocation,
  locks or waits, file/network I/O, logging, process launches, GUI calls, or
  unbounded work on the real-time thread.
- Preallocate buffers and workspaces outside processing. Handle zero/variable
  block sizes and changing device configurations without overrunning buffers.
- Exchange data through bounded queues or carefully designed snapshots. Verify
  lock-free assumptions for the target platform; `std::atomic` alone is not proof.
  Define overflow/backpressure behavior and memory ordering explicitly.
- Publish graph changes safely. Reclaim old graphs, plugins, buffers, and shared
  state off the callback thread. Even a last `shared_ptr` release can allocate or
  destroy resources unpredictably; audit it rather than assuming it is safe.
- Never use message-thread timers or wall-clock preview state as an audio clock.
  Separate audio sample time, musical beat time, and UI elapsed time.
- Handle device loss, sample-rate changes, suspension, and shutdown predictably.
  Use silence or a defined fallback for failures; avoid emitting uninitialized audio.

## Signal and musical correctness

- Document sample format, channel layouts, units, headroom, clipping behavior,
  routing rules, supported sample rates, and buffer-size limits.
- Validate finite inputs. Handle denormals, gain ramps, bypass transitions, and
  filter changes without avoidable clicks. Do not silently normalize user audio.
- Define peak, RMS, dBFS, and true-peak meters precisely when introduced. Silence
  must display as silence. Use bounded meter exchange and slower UI sampling.
- Define tempo-map and time-signature semantics, rounding, and sample-accurate
  event offsets. Test compound meters, tempo changes, loop boundaries, and seek.
- Ensure stop/seek/device-loss behavior releases MIDI notes and sustain state.
  Handle note ordering, channels, velocity, and malformed events defensively.
- Distinguish monitoring latency, plugin latency, and displayed process CPU.
  Verify delay compensation and offline export behavior as separate features.
- Freeze/bounce/export must state what is included and preserve user sources.

## Third-party hosting

- SDK support is not a promise that every plugin works. Maintain a tested
  compatibility matrix by format, architecture, plugin version, and OS.
- Treat plugin binaries/state as untrusted. Design scanning outside the UI
  process with timeouts, crash reporting/quarantine, and cancellation. Do not
  automatically execute arbitrary plugins merely to populate browser metadata.
- Review SDK licensing and distribution conditions before adding each format.
- Respect plugin lifecycle, thread-affinity rules, bus configuration, state
  restoration, parameter identifiers, and processing start/stop ordering.
- Host automation through stable parameter IDs with normalized/display conversions;
  do not scrape plugin windows as the parameter model.
- Design for missing plugins, load failures, hangs, invalid latency reports, and
  editor teardown. Preserve unavailable plugin state for later recovery.
- Make bridging or runtime process isolation an explicit architectural decision.
  Out-of-process scanning alone does not isolate runtime plugin crashes.
- For built-in devices, keep DSP, parameter/state definitions, and editor views
  separate so an embedded device panel does not become the processing engine.

## Verification before calling audio work complete

- Use deterministic signals and reference expectations: silence, impulses, sine
  waves, ramps, clipping boundaries, and malformed/non-finite inputs where relevant.
- Test channel counts, supported sample rates, changing block lengths, gain and
  bypass changes, tempo/loop/seek boundaries, and repeated start/stop/shutdown.
- Measure callback deadlines and overruns under a documented workload; averages
  alone are insufficient. Use Release profiling and appropriate memory/thread tools.
- Test plugin scan failures and missing-plugin project recovery when hosting exists.
- Compare real-time and offline output where deterministic behavior is expected.
  State numerical tolerances and explain intentional differences.
- Use safe monitoring levels for listening tests. Automated signal checks and
  human listening complement each other; never claim listening was performed
  merely because a waveform or meter was visible.

The first sample-playback implementation is documented in
[DECISION-AUDIO-CLIPS](../docs/DECISION-AUDIO-CLIPS.md). Its validation is limited
to the recorded tests, not blanket compliance with every future requirement here.


## Current implementation boundaries

- AudioOutput renders a stereo DAG in chunks of at most 64 samples. Sources,
  devices, post-fader sends/returns and Master share the graph. Browser preview
  enters Master; clip audition enters the clip channel. Output is bounded to
  [-1, 1]. MIDI scheduling uses sample offsets; UI timers only poll audio time.
- Published plans retain immutable base values and private render state. Hazard
  protection and off-callback collection preserve plan/processor lifetime.
  Audit ownership changes carefully; native plugin internals are not guaranteed
  allocation-free or safe merely because the host callback is bounded.
- Built-in automation is evaluated per sample; native parameter automation is
  applied per chunk. Preserve stable IDs/ranges, bypass-to-manual behavior and
  orphaned target data. Piano controller lanes are distinct from grouped lanes.
- SignalAnalysis passes samples through a bounded SPSC queue; full queues drop
  analysis frames. UI code computes the FFT. Stereo magnitude uses independent
  channels, preventing cancellation of anti-phase material. Track meters are
  post-fader sample peaks; Gravity histories measure input/output/reduction.
- Project state capture briefly suspends rendering, asynchronously waits for an
  idle callback, then captures native state on the message thread. Export uses
  independent plugin instances and keeps final native ownership on the UI thread.
- Only Windows x64 VST3 stereo hosting is enabled. Explicit scanning runs in a
  helper with timeout/cancel/quarantine; runtime processing remains in-process.
  PDC, sidechains, multichannel buses, other formats and complete playhead metadata
  are unsupported. Preserve unavailable plugin blobs and stable parameter IDs.
- Audio pitch controls perform resampling (speed changes with pitch), not quality
  time warping. WAV export stops sources at the range end and allows an effect
  tail; loop exports have no pre-roll. Record/metronome controls remain UI only.

Read [the graph decision](../docs/DECISION-AUDIO-GRAPH.md) and
[the project/hosting decision](../docs/DECISION-PROJECTS-HOSTING-AUTOMATION.md).
Use the latest [validation record](../docs/VALIDATION.md) to distinguish verified
cases from untested compatibility, listening quality and deadline behavior.
