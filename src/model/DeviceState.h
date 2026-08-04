#pragma once
#include "constants/Devices.h"
#include "AudioClip.h"
#include <array>
#include <string>
#include <vector>
namespace auralis
{
class HostedProcessor;
enum class DeviceKind { synth, sampler, reverb, equalizer, delay, compressor, distortion, flanger, phaser, chorus, external };
inline const char* deviceName(DeviceKind kind)
{
    constexpr const char* names[]={"Prism","Atlas","Bloom","Contour","Echo","Gravity","Drive","Flange","Phase","Chorus","Plug-In"};
    return names[static_cast<int>(kind)];
}
inline bool isInstrument(DeviceKind kind) { return kind==DeviceKind::synth||kind==DeviceKind::sampler; }
struct Parameter { std::string name; double minimum,maximum,initial; bool logarithmic=false; };
inline std::vector<Parameter> deviceParameters(DeviceKind kind)
{
    if(kind==DeviceKind::external)return {};
    if(kind==DeviceKind::synth)
    {
        std::vector<Parameter> p;
        for(int osc=0;osc<3;++osc)
        {
            const auto prefix="OSC "+std::to_string(osc+1)+" ";
            for(auto v:std::vector<Parameter>{{"Wave",0,3,0},{"Unison",1,8,1},{"Detune",0,1,.12},{"Blend",0,1,.5},{"Phase",0,1,0},{"Range",-36,36,0},{"Pan",-1,1,0},{"Level",0,1,osc==0?.65:0}})
            { v.name=prefix+v.name;p.push_back(v); }
        }
        for(int env=0;env<3;++env)
        {
            const auto prefix="ENV "+std::to_string(env+1)+" ";
            for(auto v:std::vector<Parameter>{{"Attack",.001,8,.01,true},{"Hold",0,4,0},{"Decay",.001,8,.3,true},{"Sustain",0,1,.7},{"Release",.005,12,.3,true}})
            {v.name=prefix+v.name;p.push_back(v);}
        }
        for(auto v:std::vector<Parameter>{{"Cutoff",20,20000,16000,true},{"Resonance",.1,1,.1},{"Filter pan",-1,1,0},{"Drive",1,8,1},{"Fat",0,1,0},{"Filter mix",0,1,1}})p.push_back(v);
        for(int lfo=0;lfo<4;++lfo)
        {
            const auto prefix="LFO "+std::to_string(lfo+1)+" ";
            for(auto v:std::vector<Parameter>{{"Rate",.01,30,1,true},{"Rise",0,8,0},{"Delay",0,8,0},{"Smooth",0,1,0}})
            {v.name=prefix+v.name;p.push_back(v);}
        }
        return p;
    }
    if(kind==DeviceKind::sampler)return {{"Level",0,1,.7},{"Root note",0,127,60},{"Attack",.001,4,.005,true},{"Release",.005,8,.1,true}};
    if(kind==DeviceKind::reverb)return {{"Delay ms",0,250,20},{"Size",.1,1,.65},{"Mod",0,1,.2},{"Diff speed",.1,8,1},{"High cut",1000,20000,9000,true},{"Low cut",20,2000,120,true},{"Decay s",.1,12,2.5,true},{"Dry",0,1,1},{"Wet",0,1,.25}};
    if(kind==DeviceKind::equalizer)
    {
        std::vector<Parameter> p;constexpr double hz[]={60,150,400,1000,2500,6000,12000};
        for(int b=0;b<7;++b){const auto n="Band "+std::to_string(b+1)+" ";p.push_back({n+"Hz",20,20000,hz[b],true});p.push_back({n+"dB",-18,18,0});p.push_back({n+"Q",.1,12,.707,true});p.push_back({n+"Type",0,4,0});}
        return p;
    }
    if(kind==DeviceKind::delay)return {{"Time ms",1,1800,375,true},{"Feedback",0,.95,.35},{"Mix",0,1,.25},
        {"Tempo sync",0,1,0},{"Beats",.125,4,.75},{"Offset %",-50,50,3},{"Model",0,2,2},
        {"Low cut Hz",20,2000,20,true},{"High cut Hz",200,20000,20000,true},{"Feedback drive",1,10,1},
        {"Mod rate Hz",.01,10,.5,true},{"Mod depth ms",0,20,0},{"Smoothing ms",1,500,20,true},
        {"Diffusion",0,1,0},{"Dry level",0,1,1},{"Wet level",0,1,1}};
    if(kind==DeviceKind::compressor)return {{"Threshold dB",-60,0,-18},{"Ratio",1,20,4},{"Attack ms",.1,100,10,true},{"Release ms",10,1000,150,true},{"Makeup dB",0,24,0}};
    if(kind==DeviceKind::distortion)return {{"Drive",1,30,3},{"Mix",0,1,.3}};
    return {{"Rate Hz",.01,10,.4,true},{"Depth",0,1,.5},{"Feedback",-.9,.9,.15},{"Mix",0,1,.3}};
}
struct Modulation { int source=0,target=0;double amount=.25; };
struct DeviceState
{
    int id=0;
    DeviceKind kind=DeviceKind::synth;
    bool bypass=false;
    std::string pluginDescription,pluginName;
    bool pluginInstrument=false;
    std::vector<uint8_t> pluginState;
    std::vector<std::string> parameterIds,parameterNames;
    std::vector<double> externalValues;
    std::shared_ptr<HostedProcessor> hosted;
    bool instrument() const{return isInstrument(kind)||(kind==DeviceKind::external&&pluginInstrument);}
    std::string displayName() const{return kind==DeviceKind::external?pluginName:deviceName(kind);}
    std::array<double,devices::maximumParameters> values{};
    std::vector<Modulation> modulation;
    std::shared_ptr<const AudioData> sample;
    explicit DeviceState(DeviceKind k=DeviceKind::synth,int identity=0):id(identity),kind(k)
    {const auto params=deviceParameters(kind);for(size_t i=0;i<params.size();++i)values[i]=params[i].initial;}
};
}
