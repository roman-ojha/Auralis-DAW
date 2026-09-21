# Integrated menus, contextual help and media inspection

Date: 2026-09-21

The user requested removal of the branding/status strips, integrated menus,
sidebar information and waveform views, Windows identity and durable AI guidance.

The menu uses JUCE MenuBarComponent with the existing Auralis theme, within the
client area. Native window chrome remains responsible for moving/resizing/closing.
Existing commands share the transport model; future commands are disabled. This
avoids implementing a second transport state or suggesting nonexistent save/edit
features. Reset Layout restores existing geometry/zoom behavior from View.

Info View resolves explicit component descriptions/tooltips and custom-painted
regions through HelpProvider. Workspace samples pointer/focus changes using its
existing timer. This avoids per-control listeners and keeps help available during
keyboard use. Library selection also updates contextual help. A required contract
in FEATURE_MAP.md makes new controls and future behavior changes extend this help.
This is a development rule, not an automatic guarantee for unknown future code.

The catalog contains no audio files. A file chooser therefore provides a real,
read-only waveform without introducing a media library, track import or playback.
JUCE audio_formats (and audio_basics) is added to the already pinned dependency.
A single worker decodes into a bounded buffer and peak array; an atomic generation
rejects obsolete jobs and results. UI polls results, with no component references
on the worker. Source files are never modified. AudioThumbnail was considered;
its audio_utils dependency adds device and processor modules unnecessary here.

Limits: 10 minutes, 32 channels, 384 kHz, combined-channel absolute-peak envelope;
not sample editing or true-peak metering. Shutdown cooperatively cancels between
read blocks and waits for the current decoder call; a stalled filesystem/decoder
can delay shutdown. Process isolation/timeouts for hostile media are future work.

The original orbit mark is rendered by a reproducible System.Drawing asset script.
CMake embeds Windows icon resources and the runtime window receives the same PNG.
No icon-cache clearing, registry changes or third-party logo assets are required.

Validation evidence and explicit untested cases live in VALIDATION.md.
