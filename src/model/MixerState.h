#pragma once
#include "constants/Design.h"
#include "constants/Mixer.h"
#include "DeviceState.h"
#include "Automation.h"
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
    std::vector<AutomationLane> automation;
    double peakLeft=0,peakRight=0; // Message-thread display values from the engine.
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
    std::function<void(TrackId)> onSelected, onRenameRequested;
    void rename(TrackId id,const std::string& name)
    {if(auto* c=find(id);c&&!name.empty()){c->name=name.substr(0,128);changed();}}
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
    void select(TrackId id) { if (find(id)) { selected = id; if(onSelected)onSelected(id); changed(); } }
    TrackId addTrack(bool instrument)
    {
        if(channels.size()>=devices::maximumChannels)return -1;
        const int id=channels.back().id+1;
        channels.push_back({id,ChannelKind::arrangement,std::to_string(id)+(instrument?" Instrument":" Audio"),instrument?"MIDI":"AUDIO",instrument?design::colour::violet:design::colour::blue});
        routes.push_back({id,0});changed();return id;
    }
    bool addDevice(TrackId id,DeviceKind kind,std::shared_ptr<const AudioData> sample={})
    {
        auto* c=find(id);if(!c||c->devices.size()>=devices::maximumDevices)return false;
        size_t total=0;for(const auto& channel:channels)total+=channel.devices.size();
        if(total>=devices::maximumTotalDevices)return false;
        if(isInstrument(kind)&&(c->type!="MIDI"||std::any_of(c->devices.begin(),c->devices.end(),[](const auto& d){return d.instrument();})))return false;
        DeviceState d(kind,nextDevice++);d.sample=std::move(sample);
        if(c->kind==ChannelKind::send)
        {
            if(kind==DeviceKind::reverb){d.values[7]=0;d.values[8]=1;}
            if(kind==DeviceKind::delay)d.values[2]=1;
            if(kind==DeviceKind::chorus||kind==DeviceKind::flanger||kind==DeviceKind::phaser)d.values[3]=1;
        }
        if(isInstrument(kind))c->devices.insert(c->devices.begin(),std::move(d));else c->devices.push_back(std::move(d));
        changed();return true;
    }
    void restore(std::deque<ChannelState> next,std::vector<SendRoute> nextRoutes,int selection)
    {
        channels=std::move(next);routes=std::move(nextRoutes);selected=selection;nextDevice=1;
        for(const auto& c:channels)for(const auto& d:c.devices)nextDevice=std::max(nextDevice,d.id+1);
    }
    bool automationMode=false;
    editing::Tick automationTime=0;
    void capture(TrackId id,int device,int parameter,double value,double minimum,double maximum,const std::string& title,bool logarithmic=false)
    {
        if(!automationMode)return;
        auto* c=find(id);if(!c)return;
        auto found=std::find_if(c->automation.begin(),c->automation.end(),[=](const auto& lane){return lane.device==device&&lane.parameter==parameter;});
        if(found==c->automation.end())
        {
            if(c->automation.size()>=project::maximumAutomationLanes)return;
            int identity=1;for(const auto& lane:c->automation)identity=std::max(identity,lane.id+1);
            c->automation.push_back({identity,device,parameter,title,true,logarithmic,minimum,maximum,{}});found=c->automation.end()-1;
        }
        found->setPoint(automationTime,value);
    }
    void parameter(TrackId id,int device,int parameter,double value)
    {
        if(auto* c=find(id))for(auto& d:c->devices)if(d.id==device)
        {
            if(d.kind==DeviceKind::external)
            {
                if(parameter<0||parameter>=static_cast<int>(d.externalValues.size()))return;
                if(!std::isfinite(value)||std::abs(d.externalValues[parameter]-value)<1e-7)return;
                d.externalValues[parameter]=std::clamp(value,0.0,1.0);capture(id,device,parameter,value,0,1,d.pluginName+" / "+d.parameterNames[parameter]);changed();return;
            }
            const auto specs=deviceParameters(d.kind);if(parameter<0||parameter>=static_cast<int>(specs.size())||!std::isfinite(value))return;
            const auto& p=specs[parameter];d.values[parameter]=std::clamp(value,p.minimum,p.maximum);
            capture(id,device,parameter,d.values[parameter],p.minimum,p.maximum,std::string(deviceName(d.kind))+" / "+p.name,p.logarithmic);changed();return;
        }
    }
    void notifyDevices() { changed(); }
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
        if (sendCount() >= mixing::maximumSends || channels.size()>=devices::maximumChannels) return -1;
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
        { r.amount = std::clamp(amount, mixing::sendMinimum, mixing::sendMaximum); capture(source,-1,100+destination,r.amount,0,1,"Mixer / Send to "+find(destination)->name); changed(); return; }
    }
private:
    std::deque<ChannelState> channels; // Stable channel addresses when sends are appended.
    std::vector<SendRoute> routes;
    TrackId selected = 1;
    int nextDevice=1;
    void changed() { if (onChanged) onChanged(); }
    void setNumber(TrackId id, double value, double ChannelState::* field, double minimum, double maximum)
    {
        if (!std::isfinite(value)) return;
        if (auto* c = find(id)) { c->*field = std::clamp(value, minimum, maximum); capture(id,-1,field==&ChannelState::gain?0:field==&ChannelState::pan?1:2,c->*field,minimum,maximum,field==&ChannelState::gain?"Mixer / Volume":field==&ChannelState::pan?"Mixer / Pan":"Mixer / Width"); changed(); }
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
