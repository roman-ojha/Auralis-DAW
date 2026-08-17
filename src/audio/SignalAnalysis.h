#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include "constants/Devices.h"
#include "constants/Project.h"

namespace auralis
{
// Single audio producer / message-thread consumer. Full queues drop analysis
// samples, never delay audio. FFT and history construction run on the consumer.
class SignalAnalysis
{
public:
    static constexpr unsigned fftSize = project::analysisFftSize, capacity = project::analysisCapacity, bands = project::analysisBands;
    struct Snapshot
    {
        std::array<float,bands> spectrum{};
        float input = 0, output = 0, reduction = 0;
        double sampleRate = 48000;
    };
    void push(double sampleLeft,double sampleRight,double input,double reduction,double rate) noexcept
    {
        const auto w = write.load(std::memory_order_relaxed);
        const auto next = (w+1)%capacity;
        if(next == read.load(std::memory_order_acquire)) return;
        queue[w] = {static_cast<float>(sampleLeft),static_cast<float>(sampleRight),
                    static_cast<float>(input),static_cast<float>(reduction)};
        sampleRate.store(rate,std::memory_order_relaxed);
        write.store(next,std::memory_order_release);
    }
    Snapshot consume()
    {
        Snapshot result; result.sampleRate = sampleRate.load(std::memory_order_relaxed);
        auto r = read.load(std::memory_order_relaxed);
        const auto end = write.load(std::memory_order_acquire);
        while(r != end)
        {
            const auto& frame = queue[r];
            left[position] = frame.left; right[position] = frame.right;
            position = (position+1)%fftSize;
            result.input = std::max(result.input,frame.input);
            result.output = std::max({result.output,std::abs(frame.left),std::abs(frame.right)});
            result.reduction = std::max(result.reduction,frame.reduction);
            r = (r+1)%capacity;
        }
        read.store(r,std::memory_order_release);
        if(r == lastRead) { for(auto& v:smoothed) v *= .85f; }
        else
        {
            transform(left); auto power = magnitudes;
            transform(right);
            for(unsigned b=0;b<bands;++b)
            {
                const double lo = 20*std::pow(1000.0,static_cast<double>(b)/bands);
                const double hi = 20*std::pow(1000.0,static_cast<double>(b+1)/bands);
                const int first = std::clamp(static_cast<int>(lo*fftSize/result.sampleRate),1,static_cast<int>(fftSize/2));
                const int last = std::clamp(static_cast<int>(std::ceil(hi*fftSize/result.sampleRate)),first,static_cast<int>(fftSize/2));
                double peak = 0;
                for(int k=first;k<=last;++k) peak = std::max({peak,power[k],magnitudes[k]});
                const float level = static_cast<float>(std::clamp((20*std::log10(std::max(1e-6,peak))+90)/90,0.0,1.0));
                smoothed[b] = std::max(level,smoothed[b]*.85f);
            }
        }
        lastRead = r; result.spectrum = smoothed; return result;
    }
private:
    struct Frame { float left=0,right=0,input=0,reduction=0; };
    std::array<Frame,capacity> queue{};
    std::atomic<unsigned> write{0},read{0};
    std::atomic<double> sampleRate{48000};
    std::array<float,fftSize> left{},right{};
    std::array<float,bands> smoothed{};
    std::array<std::complex<double>,fftSize> work{};
    std::array<double,fftSize/2+1> magnitudes{};
    unsigned position=0,lastRead=0;
    void transform(const std::array<float,fftSize>& samples)
    {
        for(unsigned i=0;i<fftSize;++i)
            work[i] = samples[(position+i)%fftSize]*(.5-.5*std::cos(2*devices::pi*i/(fftSize-1)));
        for(unsigned i=1,j=0;i<fftSize;++i)
        {
            unsigned bit=fftSize>>1; for(;j&bit;bit>>=1) j^=bit; j^=bit;
            if(i<j) std::swap(work[i],work[j]);
        }
        for(unsigned length=2;length<=fftSize;length<<=1)
        {
            const auto step=std::polar(1.0,-2*devices::pi/length);
            for(unsigned start=0;start<fftSize;start+=length)
            {
                std::complex<double> w=1;
                for(unsigned j=0;j<length/2;++j)
                { const auto a=work[start+j],b=work[start+j+length/2]*w;
                  work[start+j]=a+b;work[start+j+length/2]=a-b;w*=step; }
            }
        }
        for(unsigned i=0;i<=fftSize/2;++i) magnitudes[i]=std::abs(work[i])*4/fftSize;
    }
};
}
