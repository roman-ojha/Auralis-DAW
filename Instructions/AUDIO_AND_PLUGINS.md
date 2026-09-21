# Audio, MIDI, and plugin engineering

These rules apply when the user authorizes real audio/MIDI/plugin work. They are
future acceptance requirements, not a claim these subsystems exist now. Read
ENGINEERING.md first. Do not implement this document as an unsolicited backlog.

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
