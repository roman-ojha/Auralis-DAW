#include "DeviceProcessor.h"
#include <complex>
namespace auralis
{
FilterCoefficients FilterCoefficients::make(double hz,double db,double q,int type,double rate)
{
    hz=std::clamp(hz,10.0,rate*.45);q=std::clamp(q,.1,12.0);
    const double w=2*devices::pi*hz/rate,c=std::cos(w),s=std::sin(w),alpha=s/(2*q),a=std::pow(10.0,db/40.0);
    double b0=1,b1=0,b2=0,a0=1,a1=0,a2=0;
    if(type==0){b0=1+alpha*a;b1=-2*c;b2=1-alpha*a;a0=1+alpha/a;a1=-2*c;a2=1-alpha/a;}
    if(type==1||type==2)
    {
        const double root=2*std::sqrt(a)*alpha;
        if(type==1){b0=a*((a+1)-(a-1)*c+root);b1=2*a*((a-1)-(a+1)*c);b2=a*((a+1)-(a-1)*c-root);a0=(a+1)+(a-1)*c+root;a1=-2*((a-1)+(a+1)*c);a2=(a+1)+(a-1)*c-root;}
        else {b0=a*((a+1)+(a-1)*c+root);b1=-2*a*((a-1)+(a+1)*c);b2=a*((a+1)+(a-1)*c-root);a0=(a+1)-(a-1)*c+root;a1=2*((a-1)-(a+1)*c);a2=(a+1)-(a-1)*c-root;}
    }
    if(type==3||type==4){b0=(1+(type==3?-c:c))/2;b1=type==3?1-c:-(1+c);b2=b0;a0=1+alpha;a1=-2*c;a2=1-alpha;}
    return {b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
}
double FilterCoefficients::magnitude(double hz,double rate) const
{
    const auto z=std::polar(1.0,-2*devices::pi*hz/rate);
    return std::abs((b0+b1*z+b2*z*z)/(1.0+a1*z+a2*z*z));
}
DeviceProcessor::DeviceProcessor(DeviceKind k):kind(k),specs(deviceParameters(k))
{
    if(k==DeviceKind::reverb||k==DeviceKind::delay||k==DeviceKind::chorus||k==DeviceKind::flanger)
    {delayL.resize(768000);delayR.resize(768000);}
}
void DeviceProcessor::prepare(const DeviceState& d,double rate,double bpm)
{
    tempo=bpm;
    if(kind==DeviceKind::equalizer)for(int b=0;b<7;++b)coefficients[b]=FilterCoefficients::make(d.values[b*4],d.values[b*4+1],d.values[b*4+2],static_cast<int>(d.values[b*4+3]),rate);
}
void DeviceProcessor::reset()
{
    for(auto& v:voices)v=Voice{};
    // Delay tails are preserved across transport seeks; no large clear on callback.
}
void DeviceProcessor::releaseAll(){for(auto& v:voices)if(v.held){v.held=false;v.releaseAge=v.age;}}
void DeviceProcessor::noteOn(int key,int pitch,double velocity,double pan,double fine)
{
    auto* target=&voices[0];
    for(auto& v:voices)if(v.key==key){target=&v;break;}else if(v.key<0){target=&v;break;}else if(v.age>target->age)target=&v;
    *target=Voice{};target->key=key;target->pitch=pitch;target->velocity=velocity;target->pan=pan;target->fine=fine;target->held=true;
}
void DeviceProcessor::noteOff(int key){for(auto& v:voices)if(v.key==key&&v.held){v.held=false;v.releaseAge=v.age;}}
double DeviceProcessor::env(const Voice& v,const std::array<double,devices::maximumParameters>& p,int e) const
{
    const int i=24+e*5;
    const double t=v.held?v.age:v.releaseAge;
    double level=1;
    if(t<p[i])level=t/std::max(.001,p[i]);
    else if(t>p[i]+p[i+1])level=p[i+3]+(1-p[i+3])*std::max(0.0,1-(t-p[i]-p[i+1])/std::max(.001,p[i+2]));
    if(!v.held)level*=std::max(0.0,1-(v.age-v.releaseAge)/std::max(.005,p[i+4]));
    return level;
}
namespace
{
double blep(double t,double dt)
{
    if(t<dt){t/=dt;return t+t-t*t-1;}
    if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}
    return 0;
}
}
StereoSample DeviceProcessor::synth(const DeviceState& d,double rate)
{
    StereoSample out;
    for(auto& v:voices)
    {
        if(v.key<0)continue;
        if(kind==DeviceKind::sampler)
        {
            const double amp=v.velocity*d.values[0]*std::min(1.0,v.age/std::max(.001,d.values[2]))*(v.held?1:std::max(0.0,1-(v.age-v.releaseAge)/std::max(.005,d.values[3])));
            if(!d.sample||(amp<=0&&v.age>0)){v.key=-1;continue;}
            const double pos=v.age*d.sample->sampleRate*std::pow(2.0,(v.pitch-d.values[1]+v.fine)/12);
            const auto index=static_cast<size_t>(pos);
            if(index+1>=d.sample->left.size()){v.key=-1;continue;}
            const double f=pos-index;
            out.left+=(d.sample->left[index]*(1-f)+d.sample->left[index+1]*f)*amp*(v.pan>0?1-v.pan:1);
            out.right+=(d.sample->right[index]*(1-f)+d.sample->right[index+1]*f)*amp*(v.pan<0?1+v.pan:1);
            v.age+=1/rate;continue;
        }
        auto mod=v.modulationSignals; // One-sample feedback delay keeps modulation cycles bounded.
        auto p=d.values;
        for(const auto& m:d.modulation)if(m.target>=0&&m.target<61&&m.source>=0&&m.source<7)
        {
            double span=1;
            if(m.target==39)span=16000;else if(m.target<24&&m.target%8==5)span=24;else if(m.target>=24&&m.target<39)span=4;else if(m.target>=45)span=5;
            p[m.target]=std::clamp(p[m.target]+mod[m.source]*m.amount*span,specs[m.target].minimum,specs[m.target].maximum);
        }
        for(int e=0;e<3;++e)mod[e]=env(v,p,e);
        for(int lfo=0;lfo<4;++lfo)
        {
            const int i=45+lfo*4;const double time=std::max(0.0,v.age-p[i+2]);
            if(v.age>=p[i+2]){v.lfoPhase[lfo]+=p[i]/rate;v.lfoPhase[lfo]-=std::floor(v.lfoPhase[lfo]);}
            const double raw=v.age<p[i+2]?0:std::sin(v.lfoPhase[lfo]*2*devices::pi);
            const double coefficient=1-std::exp(-1/(rate*(.0001+p[i+3]*.05)));
            v.lfoSmooth[lfo]+=coefficient*(raw-v.lfoSmooth[lfo]);
            mod[3+lfo]=v.lfoSmooth[lfo]*(p[i+1]>0?std::min(1.0,time/p[i+1]):1);
        }
        v.modulationSignals=mod;
        if(!v.held&&mod[0]<=0){v.key=-1;continue;}
        StereoSample voice;
        for(int osc=0;osc<3;++osc)
        {
            const int i=osc*8,n=std::clamp(static_cast<int>(p[i+1]),1,devices::maximumUnison);
            for(int u=0;u<n;++u)
            {
                const double spread=n==1?0:2.0*u/(n-1)-1;
                const double hz=440*std::pow(2.0,(v.pitch-69+v.fine+std::clamp(p[i+5],-48.0,48.0)+spread*std::clamp(p[i+2],0.0,1.0))/12);
                const double dt=std::min(.45,hz/rate);
                auto& phase=v.phase[osc*8+u];double t=phase+p[i+4];t-=std::floor(t);
                double value=std::sin(t*2*devices::pi);
                const int wave=static_cast<int>(p[i]);
                if(wave==1)value=2*t-1-blep(t,dt);
                if(wave==2)value=(t<.5?1:-1)+blep(t,dt)-blep(std::fmod(t+.5,1.0),dt);
                if(wave==3)value=1-4*std::abs(t-.5);
                phase+=dt;phase-=std::floor(phase);
                const double weight=n==1?1:(u==n/2?1:std::clamp(p[i+3],0.0,1.0));
                const double pan=std::clamp(p[i+6]+v.pan+spread*.4,-1.0,1.0);
                const double amp=value*weight*std::clamp(p[i+7],0.0,1.0)/n;
                voice.left+=amp*(pan>0?1-pan:1);voice.right+=amp*(pan<0?1+pan:1);
            }
        }
        // Stable topology-preserving state-variable low-pass, per voice.
        const double g=std::tan(devices::pi*std::clamp(p[39],20.0,rate*.4)/rate),k=2-1.85*std::clamp(p[40],.1,1.0);
        auto filter=[&](double x,double& low,double& band)
        {
            x=std::tanh(x*std::clamp(p[42],1.0,8.0));
            const double b=(band+g*(x-low))/(1+g*(g+k)),l=low+g*b;
            band=2*b-band;low=2*l-low;
            return l*(1+std::clamp(p[43],0.0,1.0)*.5);
        };
        const double wet=std::clamp(p[44],0.0,1.0),pan=std::clamp(p[41],-1.0,1.0);
        const double l=filter(voice.left,v.lowL,v.bandL),r=filter(voice.right,v.lowR,v.bandR);
        out.left+=(voice.left*(1-wet)+l*wet*(pan>0?1-pan:1))*mod[0]*v.velocity*.22;
        out.right+=(voice.right*(1-wet)+r*wet*(pan<0?1+pan:1))*mod[0]*v.velocity*.22;
        v.age+=1/rate;
    }
    return out;
}
double DeviceProcessor::read(const std::vector<double>& buffer,double offset) const
{
    const double p=std::fmod(static_cast<double>(write)+buffer.size()-std::clamp(offset,1.0,static_cast<double>(buffer.size()-2)),static_cast<double>(buffer.size()));
    const auto i=static_cast<size_t>(p);return buffer[i]*(1-(p-i))+buffer[(i+1)%buffer.size()]*(p-i);
}
StereoSample DeviceProcessor::process(StereoSample input,const DeviceState& d,double rate)
{
    reductionDb=0;
    const auto output=processSignal(input,d,rate);
    analysis.push(output.left,output.right,std::max(std::abs(input.left),std::abs(input.right)),reductionDb,rate);
    return output;
}
StereoSample DeviceProcessor::processSignal(StereoSample input,const DeviceState& d,double rate)
{
    if(d.bypass||kind==DeviceKind::external)return input;
    if(isInstrument(kind)){const auto s=synth(d,rate);return {input.left+s.left,input.right+s.right};}
    const auto& p=d.values;StereoSample wet=input;double mix=1;
    if(kind==DeviceKind::equalizer)
    {
        for(int b=0;b<7;++b)
        {
            const auto c=coefficients[b];auto& s=filterState[b];
            auto f=[&](double x,int offset){const double y=c.b0*x+s[offset];s[offset]=c.b1*x-c.a1*y+s[offset+1];s[offset+1]=c.b2*x-c.a2*y;return y;};
            wet.left=f(wet.left,0);wet.right=f(wet.right,2);
        }
    }
    else if(kind==DeviceKind::distortion){wet={std::tanh(input.left*p[0]),std::tanh(input.right*p[0])};mix=p[1];}
    else if(kind==DeviceKind::compressor)
    {
        const double peak=std::max(std::abs(input.left),std::abs(input.right));
        const double coefficient=std::exp(-1/(rate*.001*(peak>envelope?p[2]:p[3])));
        envelope=peak+(envelope-peak)*coefficient;
        const double db=20*std::log10(std::max(1e-9,envelope));
        reductionDb=std::max(0.0,db-p[0])*(1-1/p[1]);
        const double gain=std::pow(10.0,(p[4]-reductionDb)/20);
        wet={input.left*gain,input.right*gain};
    }
    else if(kind==DeviceKind::delay)
    {
        const double target=p[3]>=.5?60.0/tempo*p[4]:p[0]*.001;
        if(smoothedDelay<=0)smoothedDelay=target;
        smoothedDelay+=(target-smoothedDelay)*(1-std::exp(-1/(rate*p[12]*.001)));
        const double time=smoothedDelay+std::sin(clock*2*devices::pi*p[10])*p[11]*.001;
        wet={read(delayL,time*rate),read(delayR,time*rate*(1+p[5]/100))};
        const double hi=1-std::exp(-2*devices::pi*std::min(p[8],rate*.45)/rate),lo=1-std::exp(-2*devices::pi*p[7]/rate);
        lowL+=hi*(wet.left-lowL);lowR+=hi*(wet.right-lowR);
        highL+=lo*(lowL-highL);highR+=lo*(lowR-highR);
        wet={lowL-highL,lowR-highR};
        if(p[9]>1.001){wet.left=std::tanh(wet.left*p[9])/std::tanh(p[9]);wet.right=std::tanh(wet.right*p[9])/std::tanh(p[9]);}
        for(int i=0;i<3;++i)
        {
            const double a=.6*p[13],l=-a*wet.left+allpassL[i],r=-a*wet.right+allpassR[i];
            allpassL[i]=wet.left+a*l;allpassR[i]=wet.right+a*r;
            wet.left=wet.left*(1-p[13])+l*p[13];wet.right=wet.right*(1-p[13])+r*p[13];
        }
        if(p[6]<.5){const double mono=(wet.left+wet.right)*.5;wet={mono,mono};}
        const bool pingPong=p[6]>=1.5;
        delayL[write]=input.left+(pingPong?wet.right:wet.left)*p[1];
        delayR[write]=input.right+(pingPong?wet.left:wet.right)*p[1];
        write=(write+1)%delayL.size();clock+=1/rate;
        return {input.left*(1-p[2])*p[14]+wet.left*p[2]*p[15],input.right*(1-p[2])*p[14]+wet.right*p[2]*p[15]};
    }
    else if(kind==DeviceKind::phaser)
    {
        const double a=.15+.65*(.5+.5*std::sin(clock*2*devices::pi*p[0]))*p[1];
        wet.left+=allpassL[7]*p[2];wet.right+=allpassR[7]*p[2];
        for(int i=0;i<6;++i){double l=-a*wet.left+allpassL[i],r=-a*wet.right+allpassR[i];allpassL[i]=wet.left+a*l;allpassR[i]=wet.right+a*r;wet={l,r};}
        allpassL[7]=wet.left;allpassR[7]=wet.right;mix=p[3];
    }
    else if(kind==DeviceKind::reverb)
    {
        StereoSample sum;constexpr double taps[]={.0297,.0371,.0411,.0437};
        for(int i=0;i<4;++i)
        {
            const double seconds=taps[i]*(.5+p[1]*2)+p[0]*.001;
            const double modulation=std::sin(clock*2*devices::pi*p[3]+i)*p[2]*rate*.0007;
            const double decay=std::pow(.001,seconds/std::max(.1,p[6]));
            sum.left+=read(delayL,seconds*rate+modulation)*decay*.24;
            sum.right+=read(delayR,seconds*rate*1.071-modulation)*decay*.24;
        }
        const double hi=1-std::exp(-2*devices::pi*std::min(p[4],rate*.4)/rate),lo=1-std::exp(-2*devices::pi*p[5]/rate);
        lowL+=hi*(sum.left-lowL);lowR+=hi*(sum.right-lowR);highL+=lo*(lowL-highL);highR+=lo*(lowR-highR);
        wet={lowL-highL,lowR-highR};delayL[write]=input.left+wet.right;delayR[write]=input.right+wet.left;
        write=(write+1)%delayL.size();clock+=1/rate;
        return {input.left*p[7]+wet.left*p[8],input.right*p[7]+wet.right*p[8]};
    }
    else
    {
        double time=p[0]*.001,feedback=p[1];mix=p[2];
        if(kind==DeviceKind::chorus||kind==DeviceKind::flanger)
        {time=(kind==DeviceKind::chorus?.018:.002)+(.5+.5*std::sin(clock*2*devices::pi*p[0]))*p[1]*(kind==DeviceKind::chorus?.012:.004);feedback=p[2];mix=p[3];}
        wet={read(delayL,time*rate),read(delayR,time*rate*1.03)};
        delayL[write]=input.left+wet.right*feedback;delayR[write]=input.right+wet.left*feedback;write=(write+1)%delayL.size();
    }
    clock+=1/rate;
    return {input.left*(1-mix)+wet.left*mix,input.right*(1-mix)+wet.right*mix};
}
}


