#pragma once
#include "MidiProject.h"
#include <set>
#include <random>

namespace auralis
{
enum class NoteEdit { legato=1, glue, chop, reverse, flip, strum, arpeggiate, humanize, major, minor, majorChord, minorChord };

// Bounded, undoable UI-document operations. Empty selection means the whole clip.
inline void editNotes(MidiProject& project, const std::set<int>& selection, NoteEdit operation, unsigned seed=1)
{
    auto* clip=project.clip(project.active);
    if(!clip||clip->notes.empty())return;
    std::vector<MidiNote> notes, untouched;
    for(const auto& note:clip->notes)
        (selection.empty()||selection.count(note.id)?notes:untouched).push_back(note);
    if(notes.empty())return;
    std::sort(notes.begin(),notes.end(),[](const auto& a,const auto& b){return a.start==b.start?a.pitch<b.pitch:a.start<b.start;});
    const auto original=notes;
    const Tick step=std::max(editing::minimumNote,project.step());
    Tick start=notes.front().start,end=start;
    int low=127,high=0;
    for(const auto& note:notes){end=std::max(end,note.start+note.length);low=std::min(low,note.pitch);high=std::max(high,note.pitch);}
    std::mt19937 random(seed);
    std::uniform_int_distribution<int> velocity(-8,8), timing(-12,12);
    if(operation==NoteEdit::chop)
    {
        notes.clear();
        for(const auto& note:original)
            for(Tick time=note.start;time<note.start+note.length;time+=step)
            {
                auto part=note;part.start=time;part.length=std::min(step,note.start+note.length-time);
                if(part.length<editing::minimumNote){if(!notes.empty())notes.back().length+=part.length;break;}
                notes.push_back(part);
                if(notes.size()+untouched.size()>editing::maximumNotes)return;
            }
    }
    else if(operation==NoteEdit::glue)
    {
        std::sort(notes.begin(),notes.end(),[](const auto& a,const auto& b){return a.pitch==b.pitch?a.start<b.start:a.pitch<b.pitch;});
        std::vector<MidiNote> merged;
        for(const auto& note:notes)
        {
            if(!merged.empty()&&merged.back().pitch==note.pitch&&merged.back().muted==note.muted&&note.start<=merged.back().start+merged.back().length)
                merged.back().length=std::max(merged.back().start+merged.back().length,note.start+note.length)-merged.back().start;
            else merged.push_back(note);
        }
        notes=std::move(merged);
    }
    else
    {
        int chordIndex=0;Tick chordStart=-1;
        for(auto& note:notes)
        {
            if(note.start!=chordStart){chordStart=note.start;chordIndex=0;}else ++chordIndex;
            switch(operation)
            {
                case NoteEdit::legato:
                {
                    Tick next=clip->cycle;
                    for(const auto& other:original)if(other.start>note.start){next=other.start;break;}
                    note.length=std::max(editing::minimumNote,next-note.start);break;
                }
                case NoteEdit::reverse:note.start=start+end-note.start-note.length;break;
                case NoteEdit::flip:note.pitch=low+high-note.pitch;break;
                case NoteEdit::strum:note.start=std::min(clip->cycle-note.length,note.start+chordIndex*editing::minimumNote);break;
                case NoteEdit::arpeggiate:
                    note.start=std::min(clip->cycle-editing::minimumNote,note.start+chordIndex*step);
                    note.length=std::min(step,clip->cycle-note.start);break;
                case NoteEdit::humanize:
                    note.start=std::clamp<Tick>(note.start+timing(random),0,clip->cycle-note.length);
                    note.velocity=std::clamp(note.velocity+velocity(random),1,127);break;
                case NoteEdit::major:
                case NoteEdit::minor:
                {
                    const int major[]={0,2,4,5,7,9,11},minor[]={0,2,3,5,7,8,10};
                    const auto* scale=operation==NoteEdit::major?major:minor;
                    int closest=0;for(int pc:std::vector<int>(scale,scale+7))if(std::abs(pc-note.pitch%12)<std::abs(closest-note.pitch%12))closest=pc;
                    note.pitch=std::clamp(note.pitch/12*12+closest,0,127);break;
                }
                default:break;
            }
        }
    }
    if(operation==NoteEdit::majorChord||operation==NoteEdit::minorChord)
    {
        for(const auto& note:original)for(int interval:{operation==NoteEdit::majorChord?4:3,7})
        {
            if(note.pitch+interval>127)continue;
            auto added=note;added.pitch+=interval;added.id=0;notes.push_back(added);
        }
    }
    if(notes.size()+untouched.size()>editing::maximumNotes)return;
    project.checkpoint();
    std::set<int> used;
    for(auto& note:notes)
    {
        if(note.id==0||used.count(note.id))note.id=project.freshNoteId();
        used.insert(note.id);
    }
    untouched.insert(untouched.end(),notes.begin(),notes.end());clip->notes=std::move(untouched);
    project.changed();
}
}
