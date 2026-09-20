#include "ui/PianoRoll.h"
#include "ui/ClipTimeline.h"
#include "ui/AutomationLaneView.h"
#include "ui/Arrangement.h"
#include "model/MidiEditing.h"
#include <iostream>
#include <stdexcept>

using namespace auralis;
namespace
{
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
juce::MouseEvent event(juce::Component& component,int x,int y,int flags,int clicks=1,juce::Point<float> origin={})
{
    const juce::Point<float> position{static_cast<float>(x),static_cast<float>(y)};
    if(origin==juce::Point<float>{})origin=position;
    return {juce::Desktop::getInstance().getMainMouseSource(),position,juce::ModifierKeys(flags),1,0,0,0,0,
        &component,&component,juce::Time::getCurrentTime(),origin,juce::Time::getCurrentTime(),clicks,origin!=position};
}
void key(PianoRoll& roll,int code,int flags=0){roll.keyPressed(juce::KeyPress(code,juce::ModifierKeys(flags),0));}
constexpr int left=juce::ModifierKeys::leftButtonModifier,right=juce::ModifierKeys::rightButtonModifier;
constexpr int ctrl=juce::ModifierKeys::ctrlModifier,shift=juce::ModifierKeys::shiftModifier;
}
int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        MixerState mixer;MidiProject automationDocument;mixer.automationMode=true;mixer.gain(1,-12);
        AutomationLaneView automation(mixer,automationDocument,1,1);automation.setSize(900,84);
        automation.mouseDown(event(automation,436,20,left));
        auto& points=mixer.find(1)->automation[0].points;
        require(points.size()==2,"Automation click adds a point");
        automation.mouseDown(event(automation,330,40,left|juce::ModifierKeys::altModifier));
        automation.mouseDrag(event(automation,330,70,left|juce::ModifierKeys::altModifier,1,{330,40}));
        require(points.front().curve>0,"Alt-drag bends automation segment");
        automation.keyPressed(juce::KeyPress('A',juce::ModifierKeys(ctrl),0));automation.keyPressed(juce::KeyPress('C',juce::ModifierKeys(ctrl),0));
        automation.keyPressed(juce::KeyPress(juce::KeyPress::deleteKey));require(points.empty(),"Selected automation points delete");
        mixer.automationTime=960;automation.keyPressed(juce::KeyPress('V',juce::ModifierKeys(ctrl),0));
        require(points.size()==2&&points.front().time==960,"Automation paste uses transport cursor");
        automation.keyPressed(juce::KeyPress('Z',juce::ModifierKeys(ctrl),0));require(points.empty(),"Automation paste undo");
        MixerState layoutMixer;MidiProject layoutDocument;Arrangement restored(layoutMixer,layoutDocument);restored.setSize(1000,500);restored.setVisible(true);
        restored.clearRows();restored.refresh();restored.resized();
        require(dynamic_cast<ClipTimeline*>(restored.getComponentAt(design::trackWidth+design::panelGap+20,design::arrangementToolbar+design::rulerHeight+20))!=nullptr,"Restored track rows must not cover clip canvas");
        MidiProject project;
        ClipTimeline timeline(project);timeline.setSize(900,400);
        double horizontalZoom=1,verticalZoom=1;
        timeline.onZoom=[&](double factor){horizontalZoom=factor;};timeline.onVerticalZoom=[&](double factor){verticalZoom=factor;};
        juce::MouseWheelDetails wheel;wheel.deltaY=.1f;
        timeline.mouseWheelMove(event(timeline,100,50,ctrl),wheel);
        timeline.mouseWheelMove(event(timeline,100,50,juce::ModifierKeys::altModifier),wheel);
        require(horizontalZoom>1&&verticalZoom>1,"Ctrl wheel zoom and Alt wheel track height");
        int opened=0;timeline.onOpen=[&](int id){opened=id;};
        timeline.mouseDoubleClick(event(timeline,100,50,left,2));
        require(project.clips.size()==1&&opened==project.active,"Left double-click must create and open a clip");
        require(project.clips.front().start==project.barTicks,"Clip appears at the clicked bar");
        timeline.mouseDoubleClick(event(timeline,300,150,right,2));
        require(project.clips.size()==2&&project.clips.back().track==2,"Right double-click must create on the clicked instrument track");
        timeline.mouseDoubleClick(event(timeline,100,350,left,2));
        require(project.clips.size()==2,"Audio lane must not accept MIDI");
        timeline.mouseDoubleClick(event(timeline,110,50,left,2));
        require(project.active==project.clips.front().id,"Double-click existing clip opens its independent document");

        MidiProject document;PianoRoll roll(document);roll.setSize(900,600);
        document.onChanged=[&]{roll.refresh();};roll.onCreateClip=[&]{document.create(1,0);};
        const int x=editing::keyboardWidth+40,y=editing::toolbarHeight+editing::rulerHeight+7*editing::defaultRowHeight+8;
        roll.mouseDown(event(roll,x,y,left));roll.mouseUp(event(roll,x,y,0));
        require(document.clips.size()==1&&document.clips.front().notes.size()==1,"Drawing on an empty editor must create a clip and note");
        auto* clip=document.clip(document.active);const Tick originalStart=clip->notes.front().start;
        roll.mouseDown(event(roll,x+20,y,left));
        roll.mouseDrag(event(roll,x+100,y,left,1,{static_cast<float>(x+20),static_cast<float>(y)}));
        roll.mouseUp(event(roll,x+100,y,0));
        require(clip->notes.front().start==originalStart+editing::ppq,"Mouse dragging moves a selected note in time");
        key(roll,'Z',ctrl);clip=document.clip(document.active);
        require(clip->notes.front().start==originalStart,"Undo restores a drag");
        key(roll,2);clip=document.clip(document.active); // Windows injected Ctrl+B without a scan code.
        require(clip->notes.size()==2&&clip->notes[0].id!=clip->notes[1].id,"Duplicate creates independent notes");
        roll.mouseDown(event(roll,x-10,y-15,right|ctrl));
        roll.mouseDrag(event(roll,x+190,y+15,right|ctrl,1,{static_cast<float>(x-10),static_cast<float>(y-15)}));
        roll.mouseUp(event(roll,x+190,y+15,0));key(roll,juce::KeyPress::deleteKey);
        require(clip->notes.empty(),"Ctrl+right rectangle and Delete remove selected notes");
        key(roll,'Z',ctrl);clip=document.clip(document.active);require(clip->notes.size()==2,"Undo restores deleted selection");
        key(roll,'A',ctrl);
        roll.mouseDown(event(roll,x+20,y,left|shift));
        roll.mouseDrag(event(roll,x+40,y,left|shift,1,{static_cast<float>(x+20),static_cast<float>(y)}));
        roll.mouseUp(event(roll,x+40,y,0));
        require(clip->notes.size()==4,"Shift-drag clones selected notes");
        key(roll,'Z',ctrl);clip=document.clip(document.active);
        roll.mouseDown(event(roll,x+5,y,right));
        roll.mouseDrag(event(roll,x+180,y,right,1,{static_cast<float>(x+5),static_cast<float>(y)}));
        roll.mouseUp(event(roll,x+180,y,0));require(clip->notes.empty(),"Swept right erase must not skip notes");
        key(roll,'Z',ctrl);clip=document.clip(document.active);
        key(roll,'T');roll.mouseDown(event(roll,x+5,y,left));
        roll.mouseDrag(event(roll,x+180,y,left,1,{static_cast<float>(x+5),static_cast<float>(y)}));
        roll.mouseUp(event(roll,x+180,y,0));require(clip->notes[0].muted&&clip->notes[1].muted,"Mute sweep toggles each note once");
        key(roll,'Z',ctrl);clip=document.clip(document.active);key(roll,'C');
        roll.mouseDown(event(roll,x+40,y-16,left));
        roll.mouseDrag(event(roll,x+40,y+16,left,1,{static_cast<float>(x+40),static_cast<float>(y-16)}));
        roll.mouseUp(event(roll,x+40,y+16,0));require(clip->notes.size()==3,"Dragged slice line splits an intersecting note");
        document.onChanged={};

        MidiProject edits;edits.create(1,0);auto* c=edits.clip(edits.active);
        edits.addNote(*c,0,60,editing::ppq);edits.addNote(*c,0,64,editing::ppq);edits.addNote(*c,0,67,editing::ppq);
        editNotes(edits,{},NoteEdit::arpeggiate);c=edits.clip(edits.active);
        require(c->notes[1].start==edits.step()&&c->notes[2].start==edits.step()*2,"Arpeggiation staggers a chord by grid");
        edits.undo();c=edits.clip(edits.active);require(c->notes[2].start==0,"Tool edit supports undo");
        editNotes(edits,{},NoteEdit::chop);c=edits.clip(edits.active);require(c->notes.size()==12,"Chop subdivides notes to grid");
        std::set<int> ids;for(const auto& note:c->notes)ids.insert(note.id);require(ids.size()==12,"Chop generates unique note identities");
        require(edits.snap(17,true)==17,"Alt bypass keeps tick precision");
        edits.snapIndex=0;require(edits.snap(17)==17,"None snap keeps tick precision");
        edits.snapIndex=1;require(edits.gridStep(320)<edits.gridStep(40),"Line snap adapts to zoom");
        std::cout<<"Editor gesture, creation, ownership, tool and snap regression checks passed.\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
