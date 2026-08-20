#pragma once
#include "model/AudioClip.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include "DeviceProcessor.h"
#include "HostedProcessor.h"
#include "model/MixerState.h"
#include "model/MidiProject.h"
#include <map>

namespace auralis
{
static_assert(std::atomic<double>::is_always_lock_free);
static_assert(std::atomic<const void*>::is_always_lock_free);
// UI publishes immutable render plans. Hazard pointer protects their lifetime;
// only the message thread reclaims plans and their media buffers.
class AudioOutput final : private juce::AudioIODeviceCallback
{
public:
    struct Plan
    {
        struct Device { DeviceState state; std::shared_ptr<DeviceProcessor> processor; mutable DeviceState renderState; };
        struct Node { int id=0; double gain=1,pan=0,width=1; bool muted=false; std::vector<AutomationLane> automation; std::vector<Device> chain; std::vector<std::pair<int,double>> routes; mutable std::array<float,64> left{},right{}; };
        struct Note { double start=0,end=0;int node=0,key=0,pitch=60;double velocity=1,pan=0,fine=0; };
        struct Event { double time=0;int note=0;bool on=false; };
        std::vector<Node> nodes;
        std::vector<Note> notes;
        std::vector<Event> events;
        std::array<bool,128> held{};
        int liveTrack=-1;
        unsigned noteRevision=0;
        std::vector<AudioClip> clips;
        double tempo=120,start=0,loopStart=0,loopEnd=0,renderEnd=-1;
        bool playing=false,loop=false,audition=false;
        unsigned seek=0;
        double leftGain=1,rightGain=1;
    };
    ~AudioOutput() override;
    juce::String open();
    void publish(Plan);
    void collect();
    void suspendForState(bool value){suspended.store(value);}
    bool stateIsIdle() const{return !callbackActive.load();}
    double currentRate() const{return reportedRate.load();}
    void resetProcessors(){processors.clear();}
    // Separate engine instance only: never call while connected to a device.
    void renderOffline(float* left,float* right,int count,double rate)
    {sampleRate=rate;float* output[]={left,right};audioDeviceIOCallbackWithContext(nullptr,0,output,2,count,{});}
    void connect(Plan&,const MixerState&,const MidiProject&,bool includeMidi=true);
    std::pair<double,double> meter(int id) const;
    SignalAnalysis masterAnalysis;
    SignalAnalysis::Snapshot deviceAnalysis(int id)
    {
        if(auto found=processors.find(id);found!=processors.end())
            if(auto processor=found->second.lock())return processor->analysis.consume();
        return {};
    }
    double position() const { return reportedPosition.load(); }
    bool running() const { return ready.load(); }
    bool auditionFinished() const { return finished.load(); }
private:
    std::atomic<bool> suspended{false},callbackActive{false};
    std::atomic<double> reportedRate{48000};
    friend struct AudioOutputTestAccess;
    std::unique_ptr<std::array<HostMidiEvent,editing::maximumNotes*2+256>> hostEvents=std::make_unique<std::array<HostMidiEvent,editing::maximumNotes*2+256>>();
    juce::AudioDeviceManager device;
    std::unique_ptr<Plan> current;
    std::vector<std::unique_ptr<Plan>> retired;
    std::atomic<const Plan*> published{nullptr},hazard{nullptr};
    std::atomic<double> reportedPosition{0};
    std::atomic<bool> ready{false},finished{true};
    double sampleRate=44100,cursor=0;
    unsigned seenSeek=~0u;
    std::array<double,audio::maximumVoices> priorGains{};
    std::array<int,audio::maximumVoices> priorIds{};
    double outputRamp=0;
    bool tailStopped=false;
    std::map<int,std::weak_ptr<DeviceProcessor>> processors;
    std::array<std::atomic<double>,devices::maximumChannels> peaksL{},peaksR{};
    std::array<bool,128> held{};
    int heldTrack=-1;
    unsigned seenRevision=~0u;
    std::array<double,devices::maximumChannels> channelGains{};
    std::array<bool,devices::maximumChannels> channelSeen{};
    void audioDeviceIOCallbackWithContext(const float* const*,int,float* const*,int,int,const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override { ready=false; }
};
}
