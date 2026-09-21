#pragma once
#include "constants/Design.h"
#include "constants/Mixer.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace auralis
{
using TrackId = int;
enum class ChannelKind { master, arrangement, send };
struct SourceState { std::string name, path; };
struct DeviceState { int id = 0; std::string name; };
struct ChannelState
{
    TrackId id = 0;
    ChannelKind kind = ChannelKind::arrangement;
    std::string name, type;
    std::uint32_t tint = design::colour::mint;
    double gain = 0, pan = 0, stereo = 0;
    bool muted = false, solo = false, armed = false, polarity = false;
    // One owner for future media/device state; neither view owns a duplicate.
    std::optional<SourceState> source;
    std::vector<DeviceState> devices;
};
struct SendRoute { TrackId source, destination; double amount = mixing::defaultSend; };
class MixerState
{
public:
    MixerState()
    {
        channels.push_back({0, ChannelKind::master, "Master", "MASTER", design::colour::mint});
        channels.push_back({1, ChannelKind::arrangement, "01 Instrument", "MIDI", design::colour::violet});
        channels.push_back({2, ChannelKind::arrangement, "02 Instrument", "MIDI", design::colour::mint});
        channels.push_back({3, ChannelKind::arrangement, "03 Drums", "MIDI", design::colour::amber});
        channels.push_back({4, ChannelKind::arrangement, "04 Audio", "AUDIO", design::colour::blue});
        for (TrackId id = 1; id <= 4; ++id) routes.push_back({id, 0});
    }
    std::function<void()> onChanged;
    const std::deque<ChannelState>& all() const { return channels; }
    const std::vector<SendRoute>& sends() const { return routes; }
    TrackId selectedId() const { return selected; }
    ChannelState* find(TrackId id)
    {
        for (auto& channel : channels) if (channel.id == id) return &channel;
        return nullptr;
    }
    const ChannelState* find(TrackId id) const
    {
        for (const auto& channel : channels) if (channel.id == id) return &channel;
        return nullptr;
    }
    const ChannelState& selectedChannel() const { return *find(selected); }
    void select(TrackId id) { if (find(id) && selected != id) { selected = id; changed(); } }
    void gain(TrackId id, double value) { setNumber(id, value, &ChannelState::gain, design::minimumGainDb, design::maximumGainDb); }
    void pan(TrackId id, double value) { setNumber(id, value, &ChannelState::pan, mixing::panMinimum, mixing::panMaximum); }
    void stereo(TrackId id, double value) { setNumber(id, value, &ChannelState::stereo, mixing::stereoMinimum, mixing::stereoMaximum); }
    void toggleMute(TrackId id) { if (auto* c = find(id)) { c->muted = !c->muted; changed(); } }
    void toggleSolo(TrackId id) { if (auto* c = find(id)) { c->solo = !c->solo; changed(); } }
    void toggleArm(TrackId id) { if (auto* c = find(id)) { c->armed = !c->armed; changed(); } }
    void togglePolarity(TrackId id) { if (auto* c = find(id)) { c->polarity = !c->polarity; changed(); } }
    int sendCount() const
    {
        return static_cast<int>(std::count_if(channels.begin(), channels.end(), [](const auto& c) { return c.kind == ChannelKind::send; }));
    }
    TrackId addSend()
    {
        if (sendCount() >= mixing::maximumSends) return -1;
        const auto id = channels.back().id + 1;
        const auto name = "Send " + std::to_string(sendCount()+1);
        channels.push_back({id, ChannelKind::send, name, "RETURN", design::colour::coral});
        routes.push_back({id, 0});
        changed();
        return id;
    }
    const SendRoute* route(TrackId source, TrackId destination) const
    {
        for (const auto& r : routes) if (r.source == source && r.destination == destination) return &r;
        return nullptr;
    }
    bool canRoute(TrackId source, TrackId destination) const
    {
        if (!find(source) || !find(destination) || source == 0 || source == destination) return false;
        std::vector<TrackId> visited;
        return !reaches(destination, source, visited);
    }
    bool toggleRoute(TrackId source, TrackId destination)
    {
        const auto existing = std::find_if(routes.begin(), routes.end(), [=](const auto& r) { return r.source == source && r.destination == destination; });
        if (existing != routes.end()) { routes.erase(existing); changed(); return true; }
        if (!canRoute(source, destination)) return false;
        routes.push_back({source, destination}); changed(); return true;
    }
    void sendAmount(TrackId source, TrackId destination, double amount)
    {
        if (!std::isfinite(amount)) return;
        for (auto& r : routes) if (r.source == source && r.destination == destination)
        { r.amount = std::clamp(amount, mixing::sendMinimum, mixing::sendMaximum); changed(); return; }
    }
private:
    std::deque<ChannelState> channels; // Stable channel addresses when sends are appended.
    std::vector<SendRoute> routes;
    TrackId selected = 1;
    void changed() { if (onChanged) onChanged(); }
    void setNumber(TrackId id, double value, double ChannelState::* field, double minimum, double maximum)
    {
        if (!std::isfinite(value)) return;
        if (auto* c = find(id)) { c->*field = std::clamp(value, minimum, maximum); changed(); }
    }
    bool reaches(TrackId from, TrackId target, std::vector<TrackId>& visited) const
    {
        if (from == target) return true;
        if (std::find(visited.begin(), visited.end(), from) != visited.end()) return false;
        visited.push_back(from);
        for (const auto& r : routes) if (r.source == from && reaches(r.destination, target, visited)) return true;
        return false;
    }
};
}
