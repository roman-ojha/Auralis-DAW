#pragma once
#include "model/DeviceState.h"
#include "SignalAnalysis.h"
namespace auralis
{
struct StereoSample { double left=0,right=0; };
struct FilterCoefficients
{
    double b0=1,b1=0,b2=0,a1=0,a2=0;
    static FilterCoefficients make(double frequency,double gain,double q,int type,double rate);
    double magnitude(double frequency,double rate) const;
};
// Constructed off the callback. Only the audio thread mutates processing history.
class DeviceProcessor
{
public:
    explicit DeviceProcessor(DeviceKind);
    void prepare(const DeviceState&,double rate,double bpm=120);
    StereoSample process(StereoSample,const DeviceState&,double rate);
    SignalAnalysis analysis;
    void noteOn(int key,int pitch,double velocity,double pan=0,double fine=0);
    void noteOff(int key);
    void releaseAll();
    void reset();
private:
    StereoSample processSignal(StereoSample,const DeviceState&,double);
    double reductionDb=0,tempo=120,smoothedDelay=0;
    DeviceKind kind;
    std::vector<Parameter> specs;
    struct Voice
    {
        int key=-1,pitch=60;bool held=false;double age=0,releaseAge=0,velocity=0,pan=0,fine=0;
        std::array<double,24> phase{};
        std::array<double,3> releaseLevel{};
        std::array<double,7> modulationSignals{};
        std::array<double,4> lfoPhase{},lfoSmooth{};
        double lowL=0,lowR=0,bandL=0,bandR=0;
    };
    std::array<Voice,devices::maximumVoices> voices{};
    std::vector<double> delayL,delayR;
    size_t write=0;
    double clock=0,envelope=0,lowL=0,lowR=0,highL=0,highR=0;
    std::array<double,8> allpassL{},allpassR{};
    std::array<FilterCoefficients,7> coefficients{};
    std::array<std::array<double,4>,7> filterState{};
    double env(const Voice&,const std::array<double,devices::maximumParameters>&,int) const;
    StereoSample synth(const DeviceState&,double);
    double read(const std::vector<double>&,double) const;
};
}

