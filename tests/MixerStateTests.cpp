#include "model/MixerState.h"
#include <iostream>
#include <limits>
#include <stdexcept>

void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int main()
{
    try
    {
        auralis::MixerState model;
        auto* arrangementReference = model.find(1);
        auto* mixerReference = model.find(1);
        require(arrangementReference == mixerReference, "Views must reference one channel instance");
        model.gain(1, -14.1); require(arrangementReference->gain == -14.1, "Gain shared across views");
        model.toggleMute(1); model.toggleSolo(1);
        require(mixerReference->muted && mixerReference->solo, "Solo preserves explicit mute state");
        model.toggleSolo(1); require(mixerReference->muted && !mixerReference->solo, "Unsolo does not lose mute");
        model.gain(1, 999); require(mixerReference->gain == 6, "Gain clamped");
        model.gain(1, std::numeric_limits<double>::quiet_NaN()); require(mixerReference->gain == 6, "NaN rejected");
        model.pan(1, -999); model.stereo(1, 999); model.togglePolarity(1);
        require(mixerReference->pan == -100 && mixerReference->stereo == 100 && mixerReference->polarity, "Channel parameter bounds");
        arrangementReference->source = auralis::SourceState{"Test source", "fixture.wav"};
        require(model.addDevice(1, auralis::DeviceKind::synth), "Instrument insertion succeeds");
        require(mixerReference->source->name == "Test source" && mixerReference->devices.size() == 1, "Source/device ownership is shared");
        const auto sendA = model.addSend(), sendB = model.addSend();
        require(model.find(sendA)->kind == auralis::ChannelKind::send, "Sends do not create arrangement tracks");
        require(model.find(1) == arrangementReference, "Adding sends cannot invalidate channel references");
        require(model.route(sendA, 0) != nullptr, "New sends output to Master");
        require(!model.toggleRoute(1, 1) && !model.toggleRoute(0, 1), "No self-route or Master output cycle");
        require(model.toggleRoute(1, sendA) && model.toggleRoute(sendA, sendB), "Track to send and send to send supported");
        require(!model.toggleRoute(sendB, 1), "Reject indirect cycle");
        require(model.toggleRoute(sendB, 2), "Send to arrangement channel supported");
        require(!model.toggleRoute(2, sendA), "Reject multihop return feedback");
        model.sendAmount(1, sendA, 0.375);
        require(model.route(1, sendA)->amount == 0.375, "Independent send amount retained");
        model.select(2); model.select(1);
        require(model.route(1, sendA)->amount == 0.375, "Source selection does not reset routes");
        model.sendAmount(1, sendA, -10); require(model.route(1, sendA)->amount == 0, "Send minimum clamped");
        model.sendAmount(1, sendA, std::numeric_limits<double>::infinity()); require(model.route(1, sendA)->amount == 0, "Infinite send rejected");
        require(model.toggleRoute(sendA, sendB), "Remove existing route");
        require(model.canRoute(sendB, 1), "Removing route releases cycle constraint");
        require(model.toggleRoute(1, sendA) && !model.route(1, sendA), "Disconnect removes route");
        require(!model.canRoute(-1, 1) && !model.canRoute(1, 9999), "Unknown IDs rejected");
        for (int i = 2; i < auralis::mixing::maximumSends; ++i) require(model.addSend() >= 0, "Create bounded sends");
        require(model.addSend() == -1 && model.find(1) == arrangementReference, "Send cap and stable ownership");
        require(std::count_if(model.all().begin(), model.all().end(), [](const auto& c) { return c.kind == auralis::ChannelKind::arrangement; }) == 4, "Send additions never add arrangement lanes");
        std::cout << "Shared channel, routing, cycle and bounds checks passed.\n";
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

