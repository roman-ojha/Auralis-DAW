#include "model/MidiProject.h"
#include <iostream>
#include <stdexcept>
using namespace auralis;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(float actual,float expected,const char* message){require(std::abs(actual-expected)<0.0001f,message);}
int main()
{
    try
    {
        auto media=std::make_shared<AudioData>();media->sampleRate=1000;
        media->left.resize(1000);media->right.resize(1000);
        for(int i=0;i<1000;++i){media->left[i]=i/1000.0f;media->right[i]=-i/1000.0f;}
        AudioClip c;c.source=media;c.sourceEnd=1;c.length=1920;
        near(audioSample(c,0.25,1,0),0.25f,"Native-rate position");
        near(audioSample(c,0.25,1,1),-0.25f,"Stereo channels preserved");
        near(audioSample(c,1,1,0),0,"Clip end is silent");
        c.reverse=true;near(audioSample(c,0.25,1,0),0.749f,"Reverse sample offset");
        c.reverse=false;c.semitones=12;near(audioSample(c,0.25,1,0),0.5f,"Octave uses double playback speed");
        c.semitones=0;c.fadeIn=0.5;near(audioSample(c,0.25,1,0),0.125f,"Linear fade in");
        c.fadeIn=0;c.fadeOut=0.5;near(audioSample(c,0.75,1,0),0.375f,"Linear fade out");
        c.fadeOut=0;c.sourceStart=0.2;c.sourceEnd=0.4;c.loop=true;
        near(audioSample(c,0.3,1,0),0.3f,"Source loop wraps within trimmed region");
        c.loop=false;near(audioSample(c,0.3,1,0),0,"Unlooped source end silent");
        c.muted=true;near(audioSample(c,0.1,1,0),0,"Muted clip silent");c.muted=false;
        MidiProject project;c.id=project.freshClipId();c.start=960;c.sourceStart=0;c.sourceEnd=1;
        project.audioClips.push_back(c);project.activeAudio=c.id;project.checkpoint();
        auto copy=c;copy.id=project.freshClipId();copy.start=3840;project.audioClips.push_back(copy);
        project.audioClips[1].gainDb=-12;project.audioClips[1].reverse=true;
        require(project.audioClips[0].gainDb==0&&!project.audioClips[0].reverse,"Copies own independent clip properties");
        require(project.audioClips[0].source==project.audioClips[1].source,"Immutable source media is shared");
        project.undo();require(project.audioClips.size()==1,"Audio edits included in document history");
        project.redo();require(project.audioClips.size()==2,"Audio redo restores independent copies");
        auto fragment=project.sliceAudio(c,1440,2400,120);
        near(static_cast<float>(fragment.phase),0.25f,"Split preserves source phase");
        project.audioClips={c};project.deleteRange(1440,1920,4,4);
        require(project.audioClips.size()==2,"Cut leaves two audio fragments");
        require(project.audioClips[0].id!=project.audioClips[1].id,"Split IDs independent");
        near(static_cast<float>(project.audioClips[1].phase),0.5f,"Right fragment source position");
        project.audioClips={c};project.moveRange(960,2880,4,4,960,0);
        require(project.audioClips.size()==1&&project.audioClips[0].start==1920,"Audio-only range moves correctly");
        std::cout<<"Audio rendering, boundaries, independent clips, history and range edits passed.\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
