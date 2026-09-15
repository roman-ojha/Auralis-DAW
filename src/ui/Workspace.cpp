#include "Workspace.h"
#include "EditorKeys.h"
namespace auralis
{
Workspace::Workspace()
{
    setLookAndFeel(&theme); setWantsKeyboardFocus(true);
    for (juce::Component* component : std::initializer_list<juce::Component*>{&menu, &transport, &browser, &arrangement, &piano, &mixer, &devices, &browserDivider, &devicesDivider}) addAndMakeVisible(component);
    mixer.setVisible(false); piano.setVisible(false);
    addChildComponent(audioEditor);
    browser.onPreview=[this](std::shared_ptr<const AudioData> source)
    {
        AudioClip clip;clip.track=0;clip.source=std::move(source);clip.sourceEnd=clip.source->seconds();
        clip.length=static_cast<Tick>(clip.sourceEnd*state.tempo/60.0*editing::ppq);previewAudio(clip);
    };
    browser.onStopPreview=[this]{stopPreview();};
    audioEditor.onPreview=[this](const AudioClip& clip){previewAudio(clip);};
    audioEditor.onStop=[this]{stopPreview();};
    transport.onStopAudition=[this]{stopPreview();};
    arrangement.onAudioOpen=[this](int id){if(auto* c=midi.audioClip(id))tracks.select(c->track);midi.activeAudio=id;audioEditor.refresh();resized();};
    arrangement.onDrop=[this](const juce::String& item,int track,Tick time){dropItem(item,track,time,false);};
    devices.onDrop=[this](const juce::String& item,int track){dropItem(item,track,0,true);};
    devices.onKeyboard=[this](bool enabled)
    {
        if(enabled){const auto error=audioOutput.open();if(error.isNotEmpty())browser.infoView.show({"Audio output unavailable",error});}
        else {auditionUntil.fill(0);liveKeys.fill(false);audioDirty=true;}
    };
    piano.onAudition=[this](int note){mouseNote=note;if(note>=0&&note<128)auditionUntil[note]=juce::Time::getMillisecondCounterHiRes()+devices::minimumAuditionMs;audioDirty=true;};
    arrangement.onImport=[this](const juce::File& file,Tick time)
    {
        if(importQueue.size()+midi.audioClips.size()>=audio::maximumVoices){browser.infoView.show({"Import queue full","Maximum 128 audio clips per session."});return;}
        importQueue.push_back({file,std::min(time,editing::maximumTime-editing::minimumNote),4,false});nextImport();
    };
    importer.onLoaded=[this](std::shared_ptr<const AudioData> source)
    {
        std::erase_if(importedMedia,[](const auto& media){return media.expired();});
        size_t samples=source->left.size()*2;
        for(const auto& weak:importedMedia)if(auto media=weak.lock())samples+=media->left.size()*2;
        if(samples*sizeof(float)>audio::maximumSampleBytes*4||midi.audioClips.size()>=audio::maximumVoices)
        {browser.infoView.show({"Audio import limit","Session audio limit: 512 MiB / 128 clips. Undo history retains media until its edits expire."});importing=false;nextImport();return;}
        if(importSample)
        {
            auto* channel=tracks.find(importTrack);bool assigned=false;
            if(channel)for(auto& d:channel->devices)if(d.kind==DeviceKind::sampler){d.sample=source;assigned=true;}
            if(!assigned)assigned=tracks.addDevice(importTrack,DeviceKind::sampler,source);
            if(assigned){importedMedia.push_back(source);tracks.select(importTrack);tracks.notifyDevices();}
            importing=false;nextImport();return;
        }
        midi.checkpoint();AudioClip clip;clip.track=importTrack;clip.id=midi.freshClipId();clip.source=source;
        clip.name=juce::File(juce::String(source->path)).getFileNameWithoutExtension().toStdString();
        clip.sourceEnd=source->seconds();clip.start=importTime;
        clip.length=std::clamp<Tick>(static_cast<Tick>(source->seconds()*state.tempo/60.0*editing::ppq),editing::minimumNote,editing::maximumTime-clip.start);
        midi.audioClips.push_back(clip);midi.activeAudio=clip.id;midi.active=0;importedMedia.push_back(source);
        tracks.select(importTrack);midi.activeAudio=clip.id;transport.showView(0);midi.changed();audioEditor.refresh();resized();
        importing=false;nextImport();
    };
    importer.onError=[this](const juce::String& error){browser.infoView.show({"Audio import failed",error});importing=false;nextImport();};
    transport.onViewChanged = [this](int view)
    {
        activeView=view; arrangement.setVisible(view==0); mixer.setVisible(view==1); piano.setVisible(view==2);
        if(view==0)arrangement.focusTimeline();
        if(view==2) { piano.refresh(); piano.grabKeyboardFocus(); }
        resized();
    };
    transport.onSnapChanged=[this](int index){midi.snapIndex=index;midi.changed();};
    piano.onCreateClip=[this]
    {
        const int selected=tracks.selectedId();
        const int track=tracks.find(selected)->type=="MIDI"?selected:1;
        const Tick time=midi.snap(static_cast<Tick>(state.seconds*state.tempo/60.0*editing::ppq));
        if(midi.create(track,time)){tracks.select(track);transport.showView(2);}
    };
    arrangement.onOpen=[this](int id){midi.active=id;if(const auto* c=midi.clip(id))tracks.select(c->track);transport.showView(2);};
    arrangement.onLoop=[this](Tick start,Tick end){state.loopStartBeats=static_cast<double>(start)/editing::ppq;state.loopEndBeats=static_cast<double>(end)/editing::ppq;state.loop=true;transport.refresh();};
    midi.onChanged=[this]{if(!loadingProject){dirty=true;++projectRevision;updateTitle();}++noteRevision;audioDirty=true;arrangement.refresh();piano.refresh();audioEditor.refresh();transport.setSnap(midi.snapIndex);resized();};
    browserDivider.onDrag = [this](int delta) { layout.browser += delta; resized(); };
    devicesDivider.onDrag = [this](int delta) { layout.devices -= delta; resized(); };
    browserDivider.onReset = [this] { layout.browser = design::browserWidth; resized(); };
    devicesDivider.onReset = [this] { layout.devices = design::devicesHeight; resized(); };
    menu.onPluginSettings=[this]{if(!pluginWindow)pluginWindow=std::make_unique<PluginSettingsWindow>(pluginSettings);pluginWindow->setVisible(true);pluginWindow->toFront(true);};
    pluginSettings.onChanged=[this]{refreshPlugins();};refreshPlugins();
    devices.onEditor=[this](int id){for(const auto& c:tracks.all())for(const auto& d:c.devices)if(d.id==id)if(auto plugin=std::dynamic_pointer_cast<NativePlugin>(d.hosted))plugin->showEditor();};
    menu.onProject=[this](int command){projectCommand(command);};
    menu.onReset = [this] { resetLayout(); };
    menu.onView = [this](int view) { transport.showView(view); };
    menu.onCreateClip=[this]{arrangement.createClip();};
    menu.onShortcuts=[this]{showShortcuts();};
    menu.onUndo=[this](bool redo){if(redo)midi.redo();else midi.undo();};
    menu.onChooseAudio = [this] { browser.chooseAudio(); };
    menu.onTransportChange = [this] { if(!state.playing)stopPreview();transport.refresh(); };
    tracks.onSelected=[this](int){midi.activeAudio=0;mouseNote=-1;audioDirty=true;devices.refresh();resized();};
    tracks.onChanged = [this]
    {
        if(!loadingProject){dirty=true;++projectRevision;updateTitle();}
        arrangement.refresh(); mixer.refresh();
        audioDirty=true;
        devices.refresh();
        auto* hovered = juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
        if (hovered && isParentOf(hovered) && hovered != &browser.infoView && !browser.infoView.isParentOf(hovered))
            browser.infoView.show(resolveHelp(hovered, juce::Desktop::getMousePosition()));
    };
    devices.analysis=[this](int id){return audioOutput.deviceAnalysis(id);};
    tracks.onRenameRequested=[this](int id)
    {
        const auto* c=tracks.find(id);if(!c)return;
        auto* dialog=new juce::AlertWindow("Rename channel","Arrangement and mixer share this channel name.",juce::MessageBoxIconType::NoIcon);
        dialog->addTextEditor("name",c->name,"Name");
        dialog->addButton("Rename",1,juce::KeyPress(juce::KeyPress::returnKey));
        dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
        dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<Workspace>(this),dialog,id](int result)
        {if(safe&&result==1)safe->tracks.rename(id,dialog->getTextEditorContents("name").trim().toStdString());}),true);
    };
    setSize(design::initialWidth, design::initialHeight);
    lastTick = juce::Time::getMillisecondCounterHiRes(); lastMetrics = lastTick; startTimerHz(design::timerHz);
}
Workspace::~Workspace() { if(shortcutFocus)shortcutFocus->removeKeyListener(this);stopTimer();projectWorker.removeAllJobs(true,-1); tracks.onChanged = {}; tracks.onSelected={}; midi.onChanged={}; shortcuts.reset(); setLookAndFeel(nullptr); }
void Workspace::resetLayout() { layout = {}; browser.resetLayout(); arrangement.resetLayout(); mixer.resetLayout(); resized(); }
void Workspace::resized()
{
    layout.constrain(getWidth(), getHeight()); auto area = getLocalBounds();
    menu.setBounds(area.removeFromTop(design::menuHeight));
    transport.setBounds(area.removeFromTop(design::transportHeight));
    area.removeFromBottom(design::footerHeight); area.reduce(design::panelGap, 0);
    browser.setBounds(area.removeFromLeft(layout.browser)); browserDivider.setBounds(area.removeFromLeft(design::panelGap));
    const bool showAudio=activeView==0&&midi.audioClip(midi.activeAudio)!=nullptr;
    devices.setVisible(activeView!=2&&!showAudio);audioEditor.setVisible(showAudio);devicesDivider.setVisible(showAudio);
    if(activeView!=2)
    {
        const auto bottom=area.removeFromBottom(showAudio?layout.devices:std::min(devices::rackHeight,area.getHeight()/2));devices.setBounds(bottom);audioEditor.setBounds(bottom);devicesDivider.setBounds(area.removeFromBottom(design::panelGap));
    }
    arrangement.setBounds(area);
    mixer.setBounds(area); piano.setBounds(area);
}
void Workspace::timerCallback()
{
    if(state.tempo!=observedTransport.tempo||state.numerator!=observedTransport.numerator||state.denominator!=observedTransport.denominator||state.loop!=observedTransport.loop||state.loopStartBeats!=observedTransport.loopStartBeats||state.loopEndBeats!=observedTransport.loopEndBeats||state.metronome!=observedTransport.metronome||state.recordArmed!=observedTransport.recordArmed)
    {if(!loadingProject){dirty=true;++projectRevision;updateTitle();}observedTransport=state;}
    transport.signal=audioOutput.masterAnalysis.consume();
    tracks.automationTime=midi.snap(static_cast<Tick>(state.seconds*state.tempo/60*editing::ppq));
    for(const auto& c:tracks.all())for(const auto& d:c.devices)if(auto plugin=std::dynamic_pointer_cast<NativePlugin>(d.hosted))
        plugin->poll([this,track=c.id,id=d.id](int parameter,double value){tracks.parameter(track,id,parameter,value);});
    const auto pointer = juce::Desktop::getMousePosition();
    auto* focus = juce::Component::getCurrentlyFocusedComponent();
    if(shortcutFocus.getComponent()!=focus)
    {
        if(shortcutFocus)shortcutFocus->removeKeyListener(this);
        shortcutFocus=nullptr;
        if(focus&&isParentOf(focus)){shortcutFocus=focus;focus->addKeyListener(this);}
    }
    const bool pointerMoved = pointer != lastPointer;
    if (pointerMoved || focus != lastFocus.getComponent())
    {
        auto* target = pointerMoved ? juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse() : focus;
        if (target && (target == this || isParentOf(target))
            && target != &browser.infoView && !browser.infoView.isParentOf(target))
            browser.infoView.show(resolveHelp(target, pointerMoved ? pointer : target->localPointToGlobal(target->getLocalBounds().getCentre()), !pointerMoved));
        lastPointer = pointer;
        lastFocus = focus;
    }
    const double now = juce::Time::getMillisecondCounterHiRes();
    std::array<bool,128> keys{};
    bool typing=false;for(auto* c=focus;c;c=c->getParentComponent())if(dynamic_cast<juce::TextEditor*>(c))typing=true;
    piano.computerKeyboardMode=devices.keyboardEnabled();arrangement.setKeyboardMode(devices.keyboardEnabled());
    if(devices.keyboardEnabled()&&!typing&&juce::Process::isForegroundProcess()&&!juce::ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown())
    {
        const juce::String mapping="AWSEDFTGYHUJKOLP";
        for(int i=0;i<mapping.length();++i)keys[60+i]=juce::KeyPress::isKeyCurrentlyDown(mapping[i]);
    }
    if(mouseNote>=0&&mouseNote<128)keys[mouseNote]=true;
    if(!typing&&juce::Process::isForegroundProcess())for(int note=0;note<128;++note)keys[note]=keys[note]||now<auditionUntil[note];
    if(keys!=liveKeys){liveKeys=keys;audioDirty=true;}
    const bool wasPlaying = state.playing;
    syncAudio();
    for(const auto& c:tracks.all())
    {const auto peak=audioOutput.meter(c.id);auto* channel=tracks.find(c.id);channel->peakLeft=peak.first;channel->peakRight=peak.second;}
    arrangement.refresh();mixer.refresh();if(devices.isVisible())devices.refresh();transport.repaint();
    if(!audioOutput.running())state.advance((now-lastTick)/1000.0);
    lastTick = now;
    const bool metricsDue = now-lastMetrics >= design::metricsIntervalMs;
    if (metricsDue) { transport.sampleMetrics(); lastMetrics = now; }
    if (wasPlaying || metricsDue) transport.refresh();
    const Tick bar=editing::ppq*state.numerator*4/state.denominator;
    if(midi.barTicks!=bar){midi.barTicks=bar;midi.changed();}
    arrangement.setPlayhead(state.barPosition(), state.numerator);
    arrangement.setLoop(static_cast<Tick>(state.loopStartBeats*editing::ppq),static_cast<Tick>(state.loopEndBeats*editing::ppq),state.loop);
    piano.setPlayhead(static_cast<Tick>(state.seconds*state.tempo/60.0*editing::ppq),state.playing);
}
bool Workspace::projectShortcut(const juce::KeyPress& key)
{
    const EditorKey input(key);const int code=input.code;const bool ctrl=input.control;
    if(ctrl&&code=='S'){saveProject(key.getModifiers().isShiftDown());return true;}
    if(ctrl&&code=='N'){projectCommand(0);return true;}
    if(ctrl&&code=='O'){projectCommand(1);return true;}
    if(ctrl&&key.getModifiers().isShiftDown()&&code=='R'){exportAudio();return true;}
    return false;
}
bool Workspace::keyPressed(const juce::KeyPress& key)
{
    const EditorKey input(key);const int code=input.code;const bool ctrl=input.control;
    if(projectShortcut(key))return true;
    // Do not steal shortcuts from an editor (including gain/tempo numeric entry).
    auto* focus=juce::Component::getCurrentlyFocusedComponent();
    for(auto* c=focus;c&&c!=this;c=c->getParentComponent())if(dynamic_cast<juce::TextEditor*>(c))return false;
    if(key.getKeyCode()==juce::KeyPress::F2Key){if(tracks.onRenameRequested)tracks.onRenameRequested(tracks.selectedId());return true;}
    if(!ctrl&&key.getModifiers().isShiftDown()&&code=='A'){transport.showView(0);arrangement.toggleAutomation();browser.infoView.show({"Automation",tracks.automationMode?"Move a channel or device control to create its child automation lane. Click to add points, Alt-drag to bend curves.":"Automation lanes hidden; enabled curves still play."});return true;}
    if(code==juce::KeyPress::F6Key){devices.toggleKeyboard();return true;}
    if(devices.keyboardEnabled()&&!ctrl&&code>=65&&code<=90)
    {
        const int index=juce::String("AWSEDFTGYHUJKOLP").indexOfChar(static_cast<juce::juce_wchar>(code));
        if(index>=0)auditionUntil[60+index]=juce::Time::getMillisecondCounterHiRes()+devices::minimumAuditionMs;
        return true;
    }
    if(code==juce::KeyPress::F1Key){showShortcuts();return true;}
    if(code==juce::KeyPress::F5Key||code==juce::KeyPress::F7Key||code==juce::KeyPress::F9Key){transport.showView(code==juce::KeyPress::F5Key?0:code==juce::KeyPress::F9Key?1:2);return true;}
    if(ctrl&&(code=='Z'||code=='Y')){if(code=='Y'||key.getModifiers().isShiftDown())midi.redo();else midi.undo();return true;}
    if(code==juce::KeyPress::spaceKey){if(ctrl)state.playing=!state.playing;else if(state.playing)state.stop();else state.playing=true;}
    else if(code==juce::KeyPress::escapeKey){state.stop();stopPreview();}
    else if(code==juce::KeyPress::homeKey)state.seconds=0;
    else if(!ctrl&&code=='R')state.recordArmed=!state.recordArmed;
    else if(ctrl&&code=='M')state.metronome=!state.metronome;
    else if(!ctrl&&code=='L')state.loop=!state.loop;
    else return false;
    transport.refresh();return true;
}
void Workspace::showShortcuts()
{
    if(!shortcuts)shortcuts=std::make_unique<ShortcutWindow>();
    shortcuts->setVisible(true);shortcuts->toFront(true);
}
void Workspace::paint(juce::Graphics& g)
{
    g.fillAll(colour(design::colour::background));
}
void Workspace::previewAudio(const AudioClip& source)
{
    const auto error=audioOutput.open();
    if(error.isNotEmpty()){browser.infoView.show({"Audio output unavailable",error});return;}
    state.playing=false;lastAudioPlaying=false;previewMode=true;auditionClip=source;
    AudioOutput::Plan plan;plan.clips={source};plan.clips[0].start=0;
    plan.clips[0].gainDb+=20*std::log10(audio::previewGain);
    plan.tempo=state.tempo;plan.playing=true;plan.audition=true;plan.seek=++audioSeek;audioOutput.connect(plan,tracks,midi,false);
    audioOutput.publish(std::move(plan));transport.refresh();
}
void Workspace::nextImport()
{
    if(importing||importQueue.empty())return;
    const auto next=importQueue.front();importQueue.pop_front();importing=true;importTime=next.time;importTrack=next.track;importSample=next.sample;
    importer.load(next.file);browser.infoView.show({"Import audio","Decoding audio for the destination channel..."});
}
void Workspace::stopPreview()
{
    if(!previewMode)return;
    previewMode=false;AudioOutput::Plan plan;plan.seek=++audioSeek;audioOutput.publish(std::move(plan));audioDirty=true;
}
void Workspace::syncAudio()
{
    audioOutput.collect();midi.tempo=state.tempo;
    if(previewMode&&(audioOutput.auditionFinished()||std::any_of(liveKeys.begin(),liveKeys.end(),[](bool held){return held;})))stopPreview();
    if(previewMode&&!state.playing)
    {
        double progress=-1;
        const auto* playing=auditionClip.id?midi.audioClip(auditionClip.id):&auditionClip;
        if(playing&&playing->source&&!audioOutput.auditionFinished())
        {
            const double region=playing->sourceEnd-playing->sourceStart;
            double local=playing->phase+audioOutput.position()*playing->speed();
            if(playing->loop&&region>0)local=std::fmod(local,region);
            if(local>=0&&local<region)progress=(playing->reverse?playing->sourceEnd-local:playing->sourceStart+local)/playing->source->seconds();
        }
        browser.setPreviewProgress(auditionClip.id==0?progress:-1);
        audioEditor.progress=auditionClip.id!=0&&auditionClip.id==midi.activeAudio?progress:-1;audioEditor.repaint();
        if(audioDirty)
        {
            if(const auto* c=auditionClip.id?midi.audioClip(auditionClip.id):&auditionClip)
            {
                AudioOutput::Plan plan;plan.clips={*c};plan.clips[0].start=0;plan.clips[0].gainDb+=20*std::log10(audio::previewGain);
                plan.tempo=state.tempo;plan.playing=true;plan.audition=true;plan.seek=audioSeek;audioOutput.connect(plan,tracks,midi,false);audioOutput.publish(std::move(plan));
            }
            else stopPreview();
        }
        audioDirty=false;return;
    }
    browser.setPreviewProgress(-1);audioEditor.progress=-1;
    if((state.playing||std::any_of(liveKeys.begin(),liveKeys.end(),[](bool held){return held;}))&&!audioOutput.running())
    {
        const auto error=audioOutput.open();
        if(error.isNotEmpty()){state.playing=false;browser.infoView.show({"Audio output unavailable",error});return;}
    }
    const bool seek=previewMode||state.playing!=lastAudioPlaying||std::abs(state.seconds-lastAudioPosition)>0.001;
    const bool changed=audioDirty||seek||state.tempo!=lastAudioTempo||state.loop!=lastLoop||state.loopStartBeats!=lastLoopStart||state.loopEndBeats!=lastLoopEnd;
    if(changed)
    {
        AudioOutput::Plan plan;plan.clips=midi.audioClips;plan.tempo=state.tempo;plan.start=state.seconds;
        plan.noteRevision=noteRevision;plan.liveTrack=tracks.selectedId();plan.held=liveKeys;audioOutput.connect(plan,tracks,midi);
        if(seek)++audioSeek;
        plan.seek=audioSeek;plan.playing=state.playing;plan.loop=state.loop;
        plan.loopStart=state.loopStartBeats*60/state.tempo;plan.loopEnd=state.loopEndBeats*60/state.tempo;
        audioOutput.publish(std::move(plan));audioDirty=false;previewMode=false;
    }
    if(audioOutput.running()&&state.playing&&!seek)state.seconds=audioOutput.position();
    lastAudioPosition=state.seconds;lastAudioPlaying=state.playing;lastAudioTempo=state.tempo;
    lastLoop=state.loop;lastLoopStart=state.loopStartBeats;lastLoopEnd=state.loopEndBeats;
}
void Workspace::dropItem(const juce::String& item,int track,Tick time,bool deviceTarget)
{
    if(item.startsWith("plugin:")){addPlugin(item.substring(7),track);return;}
    const bool device=item.startsWith("device:");
    const int kind=item.fromFirstOccurrenceOf(":",false,false).getIntValue();
    if(device&&(kind<0||kind>9))return;
    const auto type=static_cast<DeviceKind>(kind);
    if(track<0)
    {
        if(device&&!isInstrument(type)){browser.infoView.show({"Effect needs a channel","Drop effects on an existing channel or its lower device chain."});return;}
        track=tracks.addTrack(device);
    }
    auto* channel=tracks.find(track);if(!channel)return;
    if(device)
    {
        if(!tracks.addDevice(track,type)){browser.infoView.show({"Device not added","An instrument track can contain one source and up to twelve devices. Remove its source before replacing it. Audio, send and Master channels accept effects."});return;}
        if(isInstrument(type)){const auto error=audioOutput.open();if(error.isNotEmpty())browser.infoView.show({"Audio output unavailable",error});}
        tracks.select(track);return;
    }
    const juce::File file(item);if(!file.hasFileExtension("wav;aif;aiff;flac"))return;
    const bool sample=deviceTarget&&channel->type=="MIDI";
    if(channel->type!="AUDIO"&&!sample){browser.infoView.show({"Choose an audio destination","Drop audio on an Audio lane or below the lanes to create a new track. Drop on a MIDI track's device panel to load Atlas."});return;}
    if(sample&&std::any_of(channel->devices.begin(),channel->devices.end(),[](const auto& d){return d.kind==DeviceKind::synth;}))
    {browser.infoView.show({"Instrument already loaded","Remove Prism before dropping a sample to replace it with Atlas."});return;}
    if(importQueue.size()+midi.audioClips.size()>=audio::maximumVoices)return;
    importQueue.push_back({file,std::min(time,editing::maximumTime-editing::minimumNote),track,sample});nextImport();
}}





