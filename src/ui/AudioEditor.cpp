#include "AudioEditor.h"
namespace auralis
{
AudioEditor::AudioEditor(MidiProject& m):model(m)
{
    configureButton(play,"Play clip","Audition this audio clip with its own gain, pitch, reverse, loop and fades. Arrangement playback pauses.");
    configureButton(stop,"Stop","Stop audio audition.");
    configureButton(loop,"Loop","Repeat the source region when the arrangement clip is extended.",true);
    configureButton(reverse,"Reverse","Reverse this clip's source region without changing the file or other copies.",true);
    configureButton(mute,"Mute","Disable audio from this clip only.",true);
    for(auto* b:{&play,&stop,&loop,&reverse,&mute})addAndMakeVisible(b);
    play.onClick=[this]{if(auto* c=model.audioClip(model.activeAudio);c&&onPreview)onPreview(*c);};
    stop.onClick=[this]{if(onStop)onStop();};
    loop.onClick=[this]{if(auto* c=model.audioClip(model.activeAudio)){model.checkpoint();c->loop=loop.getToggleState();model.changed();}};
    reverse.onClick=[this]{if(auto* c=model.audioClip(model.activeAudio)){model.checkpoint();c->reverse=reverse.getToggleState();model.changed();}};
    mute.onClick=[this]{if(auto* c=model.audioClip(model.activeAudio)){model.checkpoint();c->muted=mute.getToggleState();model.changed();}};
    int id=0;
    for(auto* slider:{&gain,&pitch,&start,&end,&fadeIn,&fadeOut})
    {
        const int index=id++;
        slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,54,20);
        slider->onDragStart=[this]{model.checkpoint();gesture=true;};
        slider->onDragEnd=[this]{gesture=false;};
        slider->onValueChange=[this,slider,index]{if(!refreshing)update(index,slider->getValue());};
        addAndMakeVisible(slider);
    }
    gain.setRange(-60,12,0.1);pitch.setRange(-36,36,0.01);
    setHelp(gain,"Clip gain","-60 to +12 dB, independent for every copied clip. Source file stays unchanged.");
    setHelp(pitch,"Clip pitch / resample","-36 to +36 semitones. Resampling changes pitch AND playback speed; independent time-stretch/warp is not implemented.");
    setHelp(start,"Source start","Source-region start in seconds. Drag the left waveform marker or enter a value.");
    setHelp(end,"Source end","Source-region end in seconds. Drag the right waveform marker or enter a value.");
    setHelp(fadeIn,"Fade in","Linear fade duration in seconds from arrangement clip start. Also drag its upper-left fade handle in the arrangement.");
    setHelp(fadeOut,"Fade out","Linear fade duration in seconds before arrangement clip end. Also drag its upper-right fade handle in the arrangement.");
}
HelpContent AudioEditor::helpAt(juce::Point<int>,bool) const
{
    return {"Audio clip / source waveform","Click the waveform to audition. Drag its amber start/end markers to trim the source region. Each duplicate has independent settings. Editing never overwrites the audio file. Pitch uses resampling; warp, stretch modes and envelopes are not yet available. Ctrl+Z undoes clip edits."};
}
void AudioEditor::refresh()
{
    refreshing=true;
    if(auto* c=model.audioClip(model.activeAudio);c&&c->source)
    {
        gain.setValue(c->gainDb,juce::dontSendNotification);pitch.setValue(c->semitones,juce::dontSendNotification);
        start.setRange(0,std::max(audio::minimumRegion,c->sourceEnd-audio::minimumRegion),0.001);
        end.setRange(c->sourceStart+audio::minimumRegion,std::max(c->sourceStart+audio::minimumRegion,c->source->seconds()),0.001);
        start.setValue(c->sourceStart,juce::dontSendNotification);end.setValue(c->sourceEnd,juce::dontSendNotification);
        const double duration=std::max(audio::minimumRegion,c->length*60.0/(model.tempo*editing::ppq));
        for(auto* s:{&fadeIn,&fadeOut})s->setRange(0,duration,0.001);
        fadeIn.setValue(c->fadeIn,juce::dontSendNotification);fadeOut.setValue(c->fadeOut,juce::dontSendNotification);
        loop.setToggleState(c->loop,juce::dontSendNotification);reverse.setToggleState(c->reverse,juce::dontSendNotification);mute.setToggleState(c->muted,juce::dontSendNotification);
    }
    refreshing=false;repaint();
}
void AudioEditor::update(int id,double value)
{
    if(auto* c=model.audioClip(model.activeAudio))
    {
        if(!gesture)model.checkpoint();
        if(id==0)c->gainDb=value;if(id==1)c->semitones=value;
        if(id==2){c->sourceStart=value;c->phase=0;}
        if(id==3){c->sourceEnd=value;c->phase=0;}
        if(id==4)c->fadeIn=value;if(id==5)c->fadeOut=value;
        model.changed();
    }
}
void AudioEditor::resized()
{
    int x=12;
    for(auto* b:{&play,&stop,&loop,&reverse,&mute}){b->setBounds(x,35,66,24);x+=70;}
    int i=0;
    for(auto* s:{&gain,&pitch,&start,&end,&fadeIn,&fadeOut})
    {
        s->setBounds(55+(i%2)*178,66+(i/2)*juce::jmin(32,(getHeight()-65)/3),127,25);++i;
    }
    plot={376,36,juce::jmax(1,getWidth()-390),juce::jmax(1,getHeight()-52)};
}
void AudioEditor::paint(juce::Graphics& g)
{
    panel(g,getLocalBounds());const auto* c=model.audioClip(model.activeAudio);
    text(g,c?"Audio clip / "+juce::String(c->name):"Audio clip",{14,5,getWidth()-28,25},15,design::colour::mint,true);
    const char* labels[]={"Gain","Pitch","Start","End","Fade in","Fade out"};
    for(int i=0;i<6;++i)text(g,labels[i],{12+(i%2)*178,66+(i/2)*juce::jmin(32,(getHeight()-65)/3),44,25},10,design::colour::muted);
    g.setColour(colour(design::colour::background));g.fillRect(plot);
    if(!c||!c->source)return;
    const auto& source=*c->source;
    g.setColour(colour(design::colour::mint));
    for(int x=0;x<plot.getWidth();++x)
    {
        const auto bin=std::min(audio::peaks-1,x*audio::peaks/plot.getWidth());
        const float amplitude=source.peaks[static_cast<size_t>(bin)]*(plot.getHeight()-25)*0.5f;
        g.drawVerticalLine(plot.getX()+x,plot.getCentreY()-amplitude,plot.getCentreY()+amplitude);
    }
    const int left=plot.getX()+juce::roundToInt(c->sourceStart/source.seconds()*plot.getWidth());
    const int right=plot.getX()+juce::roundToInt(c->sourceEnd/source.seconds()*plot.getWidth());
    g.setColour(juce::Colours::black.withAlpha(0.5f));g.fillRect(plot.withRight(left));g.fillRect(plot.withLeft(right));
    g.setColour(colour(design::colour::amber));g.drawVerticalLine(left,static_cast<float>(plot.getY()),static_cast<float>(plot.getBottom()));g.drawVerticalLine(right-1,static_cast<float>(plot.getY()),static_cast<float>(plot.getBottom()));
    g.fillRect(left,plot.getY(),8,10);g.fillRect(right-8,plot.getY(),8,10);
    text(g,juce::String(source.seconds(),3)+" s / "+juce::String(source.sampleRate,0)+" Hz",plot.withHeight(20).reduced(12,0),10,design::colour::text);
    if(progress>=0&&progress<=1){g.setColour(juce::Colours::white);g.drawVerticalLine(plot.getX()+juce::roundToInt(progress*plot.getWidth()),static_cast<float>(plot.getY()),static_cast<float>(plot.getBottom()));}
}
double AudioEditor::timeAt(int x) const
{
    if(const auto* c=model.audioClip(model.activeAudio);c&&c->source)return std::clamp(static_cast<double>(x-plot.getX())/plot.getWidth(),0.0,1.0)*c->source->seconds();
    return 0;
}
void AudioEditor::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor(plot.contains(e.getPosition())?juce::MouseCursor::PointingHandCursor:juce::MouseCursor::NormalCursor);
}
void AudioEditor::mouseDown(const juce::MouseEvent& e)
{
    if(!plot.contains(e.getPosition()))return;
    if(const auto* c=model.audioClip(model.activeAudio);c&&c->source)
    {
        const double tolerance=8.0/plot.getWidth()*c->source->seconds();
        if(std::abs(timeAt(e.x)-c->sourceStart)<tolerance)trim=1;
        else if(std::abs(timeAt(e.x)-c->sourceEnd)<tolerance)trim=2;
        else{if(onPreview)onPreview(*c);return;}
        model.checkpoint();gesture=true;
    }
}
void AudioEditor::mouseDrag(const juce::MouseEvent& e)
{
    if(auto* c=model.audioClip(model.activeAudio);c&&trim)
    {
        if(trim==1)update(2,std::clamp(timeAt(e.x),0.0,c->sourceEnd-audio::minimumRegion));
        else update(3,std::clamp(timeAt(e.x),c->sourceStart+audio::minimumRegion,c->source->seconds()));
    }
}
void AudioEditor::mouseUp(const juce::MouseEvent&){trim=0;gesture=false;}
}
