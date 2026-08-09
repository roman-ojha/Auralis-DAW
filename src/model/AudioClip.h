#pragma once
#include "constants/Audio.h"
#include "constants/Editing.h"
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>
namespace auralis
{
struct AudioData
{
    std::string path;
    double sampleRate=44100;
    std::vector<float> left,right;
    std::array<float,audio::peaks> peaks{};
    double seconds() const { return static_cast<double>(left.size())/sampleRate; }
};
struct AudioClip
{
    int id=0,track=4;
    std::string name;
    editing::Tick start=0,length=editing::ppq*4;
    std::shared_ptr<const AudioData> source;
    double sourceStart=0,sourceEnd=0,phase=0,gainDb=0,semitones=0;
    double fadeIn=0,fadeOut=0;
    bool loop=false,reverse=false,muted=false;
    double speed() const { return std::pow(2.0,semitones/12.0); }
};
// Pure renderer: immutable media, clip-local non-destructive settings, no allocation.
inline float audioSamplePrepared(const AudioClip& clip,double elapsed,double duration,int channel,double speed,double gain)
{
    if(!clip.source||clip.muted||elapsed<0||elapsed>=duration)return 0;
    const double region=clip.sourceEnd-clip.sourceStart;
    if(region<audio::minimumRegion)return 0;
    double local=clip.phase+elapsed*speed;
    if(clip.loop)local=std::fmod(local,region);
    if(local<0||local>=region)return 0;
    const auto& data=*clip.source;
    const double sample=clip.reverse?(clip.sourceEnd-local)*data.sampleRate-1:(clip.sourceStart+local)*data.sampleRate;
    if(sample<0||sample>=static_cast<double>(data.left.size()))return 0;
    const auto index=static_cast<std::size_t>(sample),next=std::min(index+1,data.left.size()-1);
    const auto& samples=channel==0?data.left:data.right;
    const float fraction=static_cast<float>(sample-static_cast<double>(index));
    double fade=1;
    if(clip.fadeIn>0)fade=std::min(fade,elapsed/clip.fadeIn);
    if(clip.fadeOut>0)fade=std::min(fade,(duration-elapsed)/clip.fadeOut);
    return static_cast<float>((samples[index]+(samples[next]-samples[index])*fraction)*gain*std::clamp(fade,0.0,1.0));
}
inline float audioSample(const AudioClip& clip,double elapsed,double duration,int channel)
{ return audioSamplePrepared(clip,elapsed,duration,channel,clip.speed(),std::pow(10.0,clip.gainDb/20.0)); }
}
