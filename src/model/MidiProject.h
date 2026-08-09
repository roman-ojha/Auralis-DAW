#pragma once
#include "constants/Editing.h"
#include "AudioClip.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>
#include <string>

namespace auralis
{
using editing::Tick;
struct MidiNote
{
    int id = 0, pitch = 60, velocity = 100, pan = 0, finePitch = 0;
    Tick start = 0, length = editing::ppq;
    bool muted = false;
};
enum class EventTarget { volume, pan, pitch };
struct MidiEvent { Tick time = 0; EventTarget target = EventTarget::volume; double value = 1; };
struct MidiClip
{
    int id = 0, track = 1;
    std::string name = "MIDI clip";
    Tick start = 0, length = editing::ppq*4, cycle = editing::ppq*4, offset = 0;
    std::vector<MidiNote> notes;
    std::vector<MidiEvent> events;
};
// UI-thread composition document. Values own their content: never shared patterns.
class MidiProject
{
public:
    std::vector<MidiClip> clips;
    std::vector<AudioClip> audioClips;
    std::vector<int> instrumentTracks{1,2,3};
    int activeAudio=0;
    AudioClip* audioClip(int id) { for(auto& c:audioClips)if(c.id==id)return &c;return nullptr; }
    int active = 0;
    Tick barTicks = editing::ppq*4;
    int snapIndex = 7;
    std::function<void()> onChanged;
    MidiClip* clip(int id) { for (auto& c : clips) if (c.id == id) return &c; return nullptr; }
    const MidiClip* clip(int id) const { for (const auto& c : clips) if (c.id == id) return &c; return nullptr; }
    Tick step() const { return snapIndex == 11 ? barTicks : static_cast<Tick>(editing::snapBeats[std::clamp(snapIndex,0,11)]*editing::ppq); }
    Tick gridStep(double pixelsPerQuarter = editing::pixelsPerBeat) const
    {
        if (snapIndex != 1) return step();
        Tick grid = editing::ppq;
        while (grid > 1 && static_cast<double>(grid / 2) / editing::ppq * pixelsPerQuarter >= editing::lineGridPixels)
            grid /= 2;
        return grid;
    }
    Tick snap(Tick t, bool bypass = false, double pixelsPerQuarter = editing::pixelsPerBeat) const
    {
        const Tick grid = bypass || step() == 0 ? 1 : gridStep(pixelsPerQuarter);
        return std::clamp<Tick>(static_cast<Tick>(std::llround(static_cast<double>(t)/grid))*grid, 0, editing::maximumTime);
    }
    void restoreDocument(std::vector<MidiClip> nextMidi,std::vector<AudioClip> nextAudio)
    {
        clips=std::move(nextMidi);audioClips=std::move(nextAudio);undoStack.clear();redoStack.clear();nextClip=nextNote=1;
        for(const auto& c:clips){nextClip=std::max(nextClip,c.id+1);for(const auto& n:c.notes)nextNote=std::max(nextNote,n.id+1);}
        for(const auto& c:audioClips)nextClip=std::max(nextClip,c.id+1);
    }
    void changed() { if (onChanged) onChanged(); }
    void checkpoint()
    {
        undoStack.push_back({clips,active,audioClips,activeAudio});
        if (undoStack.size() > editing::historyLimit) undoStack.erase(undoStack.begin());
        redoStack.clear();
    }
    void undo() { restore(undoStack, redoStack); }
    void redo() { restore(redoStack, undoStack); }
    int create(int track, Tick start)
    {
        if (std::find(instrumentTracks.begin(),instrumentTracks.end(),track)==instrumentTracks.end() || clips.size() >= editing::maximumClips) return 0;
        checkpoint();
        MidiClip c; c.id = nextClip++; c.track = track; c.start = std::clamp<Tick>(start,0,editing::maximumTime-barTicks);
        c.length = c.cycle = barTicks; clips.push_back(c); active = c.id; changed(); return c.id;
    }
    int addNote(MidiClip& c, Tick start, int pitch, Tick length)
    {
        if (c.notes.size() >= editing::maximumNotes) return 0;
        MidiNote n; n.id = nextNote++; n.pitch = std::clamp(pitch,0,127);
        n.start = std::clamp<Tick>(start,0,c.cycle-editing::minimumNote);
        n.length = std::clamp<Tick>(length,editing::minimumNote,c.cycle-n.start);
        c.notes.push_back(n); return n.id;
    }
    int freshNoteId() { return nextNote++; }
    int freshClipId() { return nextClip++; }
    AudioClip sliceAudio(const AudioClip& source,Tick from,Tick to,double bpm)
    {
        auto out=source;out.id=nextClip++;out.start=from;out.length=to-from;
        out.phase+=(from-source.start)*60.0/(bpm*editing::ppq)*source.speed();
        if(out.loop&&out.sourceEnd>out.sourceStart)out.phase=std::fmod(out.phase,out.sourceEnd-out.sourceStart);
        if(from>source.start)out.fadeIn=0;
        if(to<source.start+source.length)out.fadeOut=0;
        return out;
    }
    double tempo=120;
    // Repeat the clip-local loop; edge resizing changes duration, never another clip.
    static std::vector<MidiNote> renderedNotes(const MidiClip& c)
    {
        std::vector<MidiNote> result;
        if (c.cycle <= 0) return result;
        for (Tick repeat = -1; repeat*c.cycle < c.length+c.offset; ++repeat)
            for (auto n : c.notes)
            {
                const Tick begin = n.start+repeat*c.cycle-c.offset;
                const Tick end = begin+n.length;
                if (end <= 0 || begin >= c.length) continue;
                n.start = std::max<Tick>(0,begin); n.length = std::min(c.length,end)-n.start;
                result.push_back(n);
                if (result.size() >= editing::maximumNotes) return result;
            }
        return result;
    }
    MidiClip slice(const MidiClip& source, Tick from, Tick to)
    {
        MidiClip out = source; out.id = nextClip++;
        out.start = from; out.length = to-from;
        out.offset = (source.offset+from-source.start)%source.cycle;
        // vectors are deep copied; fragments have independent future edits.
        return out;
    }
    void deleteRange(Tick from, Tick to, int firstTrack, int lastTrack)
    {
        if (to <= from) return;
        auto count=audioClips.size();
        for(const auto& c:audioClips)if(c.track>=firstTrack&&c.track<=lastTrack&&c.start<from&&c.start+c.length>to)++count;
        if(count>audio::maximumVoices)return;
        std::vector<MidiClip> result;
        for (const auto& c : clips)
        {
            const Tick end = c.start+c.length;
            if (c.track < firstTrack || c.track > lastTrack || end <= from || c.start >= to) result.push_back(c);
            else
            {
                if (c.start < from) result.push_back(slice(c,c.start,from));
                if (end > to) result.push_back(slice(c,to,end));
            }
        }
        clips = std::move(result);
        std::vector<AudioClip> remaining;
        for(const auto& c:audioClips)
        {
            const auto end=c.start+c.length;
            if(c.track<firstTrack||c.track>lastTrack||end<=from||c.start>=to)remaining.push_back(c);
            else
            {
                if(c.start<from)remaining.push_back(sliceAudio(c,c.start,from,tempo));
                if(end>to)remaining.push_back(sliceAudio(c,to,end,tempo));
            }
        }
        audioClips=std::move(remaining);
        if(!audioClip(activeAudio))activeAudio=0;
        if (!clip(active)) active = 0;
    }
    void moveRange(Tick from, Tick to, int firstTrack, int lastTrack, Tick delta, int trackDelta)
    {
        if (to <= from) return;
        auto count=audioClips.size();
        for(const auto& c:audioClips)if(c.track>=firstTrack&&c.track<=lastTrack&&c.start<to&&c.start+c.length>from)
        {if(c.start<from)++count;if(c.start+c.length>to)++count;}
        if(count>audio::maximumVoices)return;
        delta = std::clamp<Tick>(delta,-from,editing::maximumTime-to);
        trackDelta = lastTrack>3?0:std::clamp(trackDelta,1-firstTrack,3-lastTrack);
        std::vector<AudioClip> movingAudio;
        for(const auto& c:audioClips)
            if(c.track>=firstTrack&&c.track<=lastTrack&&c.start<to&&c.start+c.length>from)
            {
                auto fragment=sliceAudio(c,std::max(from,c.start),std::min(to,c.start+c.length),tempo);
                fragment.start+=delta;movingAudio.push_back(std::move(fragment));
            }
        std::vector<MidiClip> moving;
        for (const auto& c : clips)
            if (c.track >= firstTrack && c.track <= lastTrack && c.start < to && c.start+c.length > from)
            {
                auto fragment = slice(c,std::max(from,c.start),std::min(to,c.start+c.length));
                fragment.start += delta; fragment.track += trackDelta; moving.push_back(std::move(fragment));
            }
        deleteRange(from,to,firstTrack,lastTrack);
        for (auto& c : moving) clips.push_back(std::move(c));
        for(auto& c:movingAudio)audioClips.push_back(std::move(c));
    }
private:
    struct Snapshot { std::vector<MidiClip> clips; int active; std::vector<AudioClip> audioClips; int activeAudio; };
    std::vector<Snapshot> undoStack, redoStack;
    int nextClip = 1, nextNote = 1;
    void restore(std::vector<Snapshot>& source, std::vector<Snapshot>& destination)
    {
        if (source.empty()) return;
        destination.push_back({clips,active,audioClips,activeAudio}); auto old = std::move(source.back()); source.pop_back();
        clips = std::move(old.clips); active = old.active;
        audioClips=std::move(old.audioClips);activeAudio=old.activeAudio;changed();
    }
};
}

