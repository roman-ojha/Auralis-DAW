#include "model/MidiProject.h"
#include "model/SessionState.h"
#include <iostream>
#include <cstdlib>
using namespace auralis;
void check(bool condition,const char* message){if(!condition){std::cerr<<message<<'\n';std::exit(1);}}
int main()
{
    MidiProject p;
    const int id=p.create(1,0);check(id!=0,"instrument clip creation");check(p.create(4,0)==0,"audio lane rejects MIDI");
    auto* c=p.clip(id);p.checkpoint();p.addNote(*c,0,60,editing::ppq);c->events.push_back({0,EventTarget::volume,0.5});
    c->length=c->cycle*3;
    auto notes=MidiProject::renderedNotes(*c);check(notes.size()==3,"extension repeats original loop");
    check(notes[2].start==editing::ppq*8,"repeat timing");
    p.checkpoint();p.deleteRange(editing::ppq*3,editing::ppq*5,1,1);
    check(p.clips.size()==2,"range deletion preserves both sides");
    check(p.clips[0].length==editing::ppq*3&&p.clips[1].start==editing::ppq*5,"fragment timing");
    p.clips[0].notes[0].pitch=72;check(p.clips[1].notes[0].pitch==60,"split notes independent");
    p.clips[0].events[0].value=0.1;check(p.clips[1].events[0].value==0.5,"split automation independent");
    p.undo();check(p.clips.size()==1&&p.clips[0].length==editing::ppq*12,"undo cut");
    p.redo();check(p.clips.size()==2,"redo cut");
    p.undo();p.checkpoint();p.moveRange(0,editing::ppq*2,1,1,editing::ppq*16,1);
    check(p.clips.size()==2,"move creates fragment and remainder");
    check(p.clips.back().track==2&&p.clips.back().start==editing::ppq*16,"range moved in time and track");
    p.undo();c=p.clip(id);check(c!=nullptr,"undo restores active source");
    auto cropped=p.slice(*c,editing::ppq/2,editing::ppq*2);
    notes=MidiProject::renderedNotes(cropped);check(notes.size()==1&&notes[0].start==0&&notes[0].length==editing::ppq/2,"sustaining note trimmed at fragment boundary");
    check(p.snap(-100)==0,"negative time clamped");p.snapIndex=8;check(p.step()==320,"triplet grid");
    p.barTicks=editing::ppq*3;p.snapIndex=11;check(p.step()==editing::ppq*3,"signature-aware bar snap");
    TransportState transport;transport.playing=true;transport.loop=true;transport.loopStartBeats=4;transport.loopEndBeats=8;transport.seconds=3.9;transport.advance(0.2);
    check(std::abs(transport.seconds-2.1)<0.00001,"loop wraps to selected start");
    std::cout<<"MIDI clip ownership, edit history, range operations, snap and loop tests passed\n";
}
