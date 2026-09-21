# Engineering standards

## Scope and architecture

- Build incrementally toward a professional DAW. A UI milestone is not authority
  to implement an audio engine, plugin scanner, installer, cloud service, or store.
- Keep presentation (`src/ui/`), state (`src/model/`), platform integration
  (`src/platform/`), constants (`src/constants/`), and data (`assets/`) distinct.
- A model must not depend on a window, graphics context, or Windows handle.
  Introduce abstractions at real boundaries, not speculative layers for every class.
- Give each component one coherent responsibility. Prefer composition and explicit
  dependencies over inheritance hierarchies, hidden globals, or service locators.
- Keep `main.cpp` concerned with application startup, window lifetime, and shutdown.
- Add new source files to CMake explicitly. Do not edit generated solution/project
  files, generated assets, or ignored build output as the implementation.
- For future real features, maintain one authoritative state model. Do not let UI
  labels become the only source of tempo, routing, gain, or project data.
- Preserve original Auralis styling. Other DAWs are workflow references, not assets
  or proprietary implementations to copy.

## C++20 and ownership

- Use RAII for memory, handles, listeners, threads, timers, and temporary resources.
  Prefer value semantics and `std::unique_ptr`; use shared ownership only when real
  shared lifetime is necessary and documented.
- Raw pointers/references normally observe, not own. Document explicit ownership
  transfers required by JUCE APIs such as `setContentOwned` or `addSubItem`.
- Establish destruction order: detach models/listeners, cancel pending work, and
  stop timers before their captured objects disappear. Guard asynchronous UI
  callbacks with suitable lifetime tokens or `juce::Component::SafePointer`.
- Avoid reference cycles, dangling callbacks, detached worker threads, and captured
  stack references that outlive their scope. Keep shutdown bounded and predictable.
- Use initialized members, `const`, `override`, scoped enums, and explicit units.
  Use `noexcept` only when the implementation genuinely cannot propagate exceptions.
- Validate indices and conversions at boundaries; avoid unsigned underflow and
  unchecked narrowing. Consider overflow for sample positions and durations.
- Use assertions for programmer invariants and runtime handling for user/file/device
  failures. Do not rely on Debug-only assertions for input validation.
- Do not swallow errors with empty catch blocks or broad fallback success paths.
  Never let exceptions cross C ABI, platform callback, or audio callback boundaries.
- Match the existing four-space, brace-on-new-line style. Use descriptive names,
  small functions, and comments explaining constraints or intent. Avoid densely
  packed multi-action lines in new complex code and unrelated reformatting.
- No broad `using namespace` directives in public headers. Keep headers self-contained
  and dependencies as small as practical. Avoid premature template abstractions.

## Constants, configuration, and assets

- Put shared and configurable values in named, domain-specific headers under
  `src/constants/`: colors, typography, spacing, panel bounds, defaults, supported
  ranges, timing limits, and category metadata. Extend Design.h/Library.h where apt.
- Use `inline constexpr` for immutable compile-time values. Give non-obvious values
  units or explanatory names; avoid scattered magic numbers and hardcoded paths.
- Constants are not mutable global state. User settings, session state, meters, and
  current selections belong to owned models, not the constants folder.
- Simple loop counters, zero initialization, and local arithmetic need no artificial
  global constant. Repeated layout or behavior policy does need a named constant.
- Store catalog/sample metadata as data, not duplicated lists in widgets. Validate
  it when loading. Clearly mark placeholders; never imply that dummy assets play.
- Use portable repository-relative paths in code/build/docs. Keep machine-specific
  absolute paths, credentials, binaries, and caches out of tracked source.
- Preserve dependency and asset notices. Do not add unlicensed sample packs, fonts,
  icons, or copied plugin interfaces.

## JUCE UI/UX

- Mutate components only on the JUCE message thread. Keep blocking file operations,
  scanning, decoding, and expensive work out of paint, resize, and input callbacks.
- Paint from existing state; do not create business state or rebuild catalogs on
  every frame. Repaint only changing regions at justified refresh rates.
- Preserve a fixed-height transport header and resizable content panels. Clamp
  dividers with valid minimum/maximum bounds and provide recovery/reset behavior.
- Test the smallest supported window and extreme panel sizes. Prevent negative
  bounds, overlapping controls, unreachable content, and text reduced to illegibility.
- Use logical coordinates and vector graphics where suitable. Support DPI changes,
  readable contrast, useful empty states, and scrolling when content cannot fit.
- Provide meaningful accessibility names and values, keyboard focus indicators,
  tab navigation, and discoverable tooltips. Do not convey critical state only by color.
- Every new/changed interactive control and meaningful painted region must provide
  specific Info View help through `configureButton`, `setHelp`, or `HelpProvider`.
  Follow the coverage contract in FEATURE_MAP.md and verify hover/keyboard context.
- Text editors, popups, and sliders retain their keyboard semantics. Global shortcuts
  must not consume typing. Destructive shortcuts need deliberate, recoverable behavior.
- Use consistent control semantics: record/arm must be unmistakable; stop, pause,
  loop, mute, and solo must not silently perform a different operation.
- Make unsupported actions disabled or clearly identified as previews. Do not show
  fake playback, signal meters, recordings, progress, or successful saves as real.
- Keep app CPU/RAM distinct from future DSP load. Identify measurement units/scope.
- Preserve the two-column library: icon categories on the left, relevant expandable
  subsections/items on the right, with search and resize behavior kept coherent.

## Reliability, files, and dependencies

- Bound memory/work for imported metadata, media, and project files. Validate types,
  sizes, counts, paths, sample rates, and non-finite numbers before use.
- Future saves must preserve originals on failure, use a tested atomic-replace
  strategy where supported, and report errors. Version project formats and test
  migrations; never silently discard unknown or missing-plugin state.
- Future edit operations should be undoable transactions. Distinguish dirty state,
  saved state, autosave/recovery, and transient UI preferences.
- Log actionable failures without secrets or private audio content. Do not add
  telemetry, networking, or uploads unless explicitly part of the requested scope.
- Prefer existing dependencies. Pin additions/upgrades to reviewed releases or
  immutable revisions and document the need, compatibility, license, and impact.
- Do not patch `external/JUCE` as a shortcut. If a dependency patch is necessary,
  make it reproducible and document why application-level fixes are insufficient.
- Do not assume a local dependency checkout matches the pinned CMake revision;
  verify it for dependency changes and reproducibility investigations.
- Respect JUCE and SDK licensing; document unresolved distribution requirements.
  A successful local build is not proof of redistribution rights.
- Preserve user work. Never reset unrelated changes, disable OS protections, change
  global Git trust settings, or delete caches recursively without a justified,
  bounded target and the permissions required by the environment.

## Performance and maintainability

- Measure before optimizing. Define the workload, build mode, hardware, and units;
  report observed results rather than guessed performance guarantees.
- Avoid unbounded UI redraws, repeated parsing, excessive allocations, and busy loops.
  Cache only with a clear invalidation and ownership strategy.
- Keep bug fixes accompanied by useful regression coverage where appropriate.
  Do not add tests that merely restate trivial implementation details.
- Record technical debt specifically: location, practical consequence, and next
  action. Do not hide unfinished behavior behind success-looking UI.
- Maintain FEATURE_MAP.md on every implementation task: completed capabilities,
  source ownership, state/data flow, limitations and invariants must match the code.
