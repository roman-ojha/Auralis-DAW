# Shared arrangement/mixer channels and send routing

Date: 2026-09-21

The user requested linked arrangement/mixer tracks, FL-inspired rotary controls,
combined mute/solo, master/current meters, polarity/pan/stereo controls and a
separate send dock with visible routing. This milestone implements UI and state,
not an audio engine. It intentionally binds arrangement tracks one-to-one to
mixer channels, unlike FL Studio's freely assignable playlist/mixer relationship.

Workspace owns MixerState before its views. Channels have stable integer IDs and
live in a deque, keeping references stable when sends are appended. Arrangement
and mixer reference the same object; future source and device state containers
belong to that object. Adding sends never creates arrangement rows. A single
message-thread callback refreshes the two views and selected device area. It is
cleared before shutdown. This mutable model must not be read by an audio callback.

Send edges store source, destination and normalized level independently from
channel gain. All normal/send channels initially route to Master at unity. The
selected source's destination arrows toggle edges and its destination knobs edit
amounts. Master is a sink; reachability checking rejects feedback, including
indirect cycles. An existing edge can always be removed. These conceptual sends
are post-fader, following the reference workflow:
[Image-Line routing manual](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/mixer_iorouting.htm).
Actual summing, panning law, solo propagation, stereo processing and plugin order
need explicit DSP requirements and real-time graph snapshots in a later milestone.

Mute and solo are independent flags. Solo is additive; toggling it preserves
explicit mute states. Ctrl+right-click is the requested gesture; S on the focused control offers
a keyboard equivalent. Amber ring indicates solo; a slash indicates explicit mute.
Meters stay at -infinity dBFS regardless of gain because no signal exists.

Master/current are pinned; normal channels and sends scroll horizontally in separate
lanes. The whole body scrolls vertically when controls cannot fit. Cables are painted
after children and only connect visible endpoints; Info View lists all outgoing
destinations. Hiding sends does not remove edges. New sends are appended without
destroying controls during their callbacks. Viewports detach borrowed canvases on
destruction. Reset Layout restores geometry, not channel/routing state.

Automatic tests cover shared source/device ownership, gain/mute/solo persistence,
stable references, invalid parameter input, routing directionality, indirect cycles,
edge removal and send bounds. UI evidence and remaining limits are in VALIDATION.md.

