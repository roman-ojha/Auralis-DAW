#pragma once
#include "MixerState.h"
#include "MidiProject.h"
#include "SessionState.h"
#include "SessionView.h"
#include <juce_core/juce_core.h>

namespace auralis
{
struct ProjectSnapshot
{
    TransportState transport;
    SessionView ui;
    std::deque<ChannelState> channels;
    std::vector<SendRoute> routes;
    std::vector<MidiClip> midi;
    std::vector<AudioClip> audio;
    int selected=1,active=0,activeAudio=0,snap=7,view=0;
};
class ProjectFile
{
public:
    static juce::Result save(const juce::File&,const ProjectSnapshot&);
    static juce::Result load(const juce::File&,ProjectSnapshot&);
};
}
