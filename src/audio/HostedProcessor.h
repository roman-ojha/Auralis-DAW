#pragma once
#include <span>
#include <string>
#include <vector>
#include <cstdint>
namespace auralis
{
struct HostMidiEvent {int offset=0,pitch=60;float velocity=0;bool on=false;};
// Native plugin adapter owns lifecycle and preallocated block/MIDI storage.
class HostedProcessor
{
public:
    virtual ~HostedProcessor()=default;
    virtual void prepare(double)=0;
    virtual void process(float*,float*,int,std::span<const HostMidiEvent>,double,double,bool)=0;
    virtual void parameter(int,double)=0;
    virtual void reset()=0;
    virtual int latency() const=0;
    virtual std::vector<uint8_t> saveState()=0;
};
}
