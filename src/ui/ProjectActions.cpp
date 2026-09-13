#include "Workspace.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace auralis
{
ProjectSnapshot Workspace::snapshot() const
{
    ProjectSnapshot result;
    result.transport=state;result.channels=tracks.all();result.routes=tracks.sends();
    result.midi=midi.clips;result.audio=midi.audioClips;result.selected=tracks.selectedId();
    for(auto& c:result.channels)for(auto& d:c.devices)if(d.hosted)d.pluginState=d.hosted->saveState();
    result.active=midi.active;result.activeAudio=midi.activeAudio;result.snap=midi.snapIndex;result.view=activeView;
    result.ui.automation=tracks.automationMode;result.ui.browser=layout.browser;result.ui.editor=layout.devices;arrangement.captureView(result.ui);piano.captureView(result.ui);
    return result;
}
void Workspace::captureProject(std::function<void(ProjectSnapshot)> done)
{
    audioOutput.suspendForState(true);
    if(!audioOutput.stateIsIdle())
    {juce::Timer::callAfterDelay(10,[safe=juce::Component::SafePointer<Workspace>(this),done]{if(safe)safe->captureProject(done);});return;}
    try{auto captured=snapshot();audioOutput.suspendForState(false);done(std::move(captured));}
    catch(const std::exception& error){audioOutput.suspendForState(false);juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Plugin state capture failed",error.what());}
}
void Workspace::updateTitle()
{
    if(onTitle)onTitle("Auralis | "+(projectFile==juce::File{}?juce::String("Untitled project"):projectFile.getFileName())+(dirty?" *":""));
}
void Workspace::applyProject(ProjectSnapshot next)
{
    loadingProject=true;
    for(const auto& c:tracks.all())for(const auto& d:c.devices)if(auto plugin=std::dynamic_pointer_cast<NativePlugin>(d.hosted))plugin->closeEditor();
    state.playing=false;stopPreview();liveKeys.fill(false);auditionUntil.fill(0);mouseNote=-1;
    arrangement.clearRows();mixer.clearRows();devices.invalidate();
    tracks.restore(std::move(next.channels),std::move(next.routes),next.selected);
    midi.restoreDocument(std::move(next.midi),std::move(next.audio));
    midi.active=next.active;midi.activeAudio=next.activeAudio;midi.snapIndex=next.snap;
    state=next.transport;state.playing=false;observedTransport=state;midi.tempo=state.tempo;midi.barTicks=editing::ppq*state.numerator*4/state.denominator;
    restorePlugins();audioOutput.resetProcessors();++audioSeek;++noteRevision;audioDirty=true;lastAudioPlaying=false;
    arrangement.refresh();mixer.refresh();devices.refresh();piano.refresh();audioEditor.refresh();
    transport.setSnap(midi.snapIndex);transport.refresh();transport.showView(next.view);
    importedMedia.clear();for(const auto& c:midi.audioClips)importedMedia.push_back(c.source);
    for(const auto& c:tracks.all())for(const auto& d:c.devices)if(d.sample)importedMedia.push_back(d.sample);
    tracks.automationMode=next.ui.automation;layout.browser=next.ui.browser;layout.devices=next.ui.editor;resized();arrangement.restoreView(next.ui);piano.restoreView(next.ui);
    loadingProject=false;dirty=false;++projectRevision;updateTitle();resized();
}
void Workspace::requestClose(std::function<void()> continuation)
{
    if(projectBusy||importing)
    {juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Operation in progress","Wait for the project operation or audio import to finish before replacing this session.");return;}
    if(!dirty){continuation();return;}
    auto* dialog=new juce::AlertWindow("Save project?","This project has unsaved changes.",juce::MessageBoxIconType::QuestionIcon);
    dialog->addButton("Save",1);dialog->addButton("Discard",2);dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<Workspace>(this),continuation=std::move(continuation)](int result)
    {if(!safe)return;if(result==1)safe->saveProject(false,continuation);else if(result==2)continuation();}),true);
}
void Workspace::projectCommand(int command)
{
    if(command==2||command==3){saveProject(command==3);return;}
    if(command==4){exportAudio();return;}
    if(command==0)
    {
        requestClose([safe=juce::Component::SafePointer<Workspace>(this)]
        {if(safe){ProjectSnapshot empty;MixerState mixer;empty.channels=mixer.all();empty.routes=mixer.sends();safe->projectFile={};safe->applyProject(std::move(empty));}});
        return;
    }
    projectChooser=std::make_unique<juce::FileChooser>("Open Auralis project",projectFile,"*.aup");
    projectChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe=juce::Component::SafePointer<Workspace>(this)](const juce::FileChooser& chooser)
        {if(safe&&chooser.getResult()!=juce::File{})safe->openProject(chooser.getResult());});
}
void Workspace::saveProject(bool saveAs,std::function<void()> after)
{
    if(projectBusy||importing){browser.infoView.show({"Save pending","Wait for the current project operation or audio import to finish."});return;}
    if(saveAs||projectFile==juce::File{})
    {
        projectChooser=std::make_unique<juce::FileChooser>("Save Auralis project",projectFile==juce::File{}?juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Untitled.aup"):projectFile,"*.aup");
        projectChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,
            [safe=juce::Component::SafePointer<Workspace>(this),after](const juce::FileChooser& chooser)
            {if(safe&&chooser.getResult()!=juce::File{}){safe->projectFile=chooser.getResult().withFileExtension("aup");safe->saveProject(false,after);}});
        return;
    }
    audioOutput.suspendForState(true);
    if(!audioOutput.stateIsIdle()){juce::Timer::callAfterDelay(10,[safe=juce::Component::SafePointer<Workspace>(this),after]{if(safe)safe->saveProject(false,after);});return;}
    ProjectSnapshot captured;
    try{captured=snapshot();}catch(const std::exception& error){audioOutput.suspendForState(false);juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Plugin state save failed",error.what());return;}
    audioOutput.suspendForState(false);
    for(auto& channel:captured.channels)for(auto& device:channel.devices)device.hosted.reset();
    projectBusy=true;const auto revision=projectRevision;
    browser.infoView.show({"Saving project","Writing device settings, notes, routing and embedded audio. The original project is preserved if writing fails."});
    projectWorker.addJob([safe=juce::Component::SafePointer<Workspace>(this),file=projectFile,data=std::move(captured),revision,after]()
    {
        const auto result=ProjectFile::save(file,data);
        juce::MessageManager::callAsync([safe,result,revision,after]
        {
            if(!safe)return;safe->projectBusy=false;
            if(result.failed()){juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Save failed",result.getErrorMessage());return;}
            if(revision==safe->projectRevision)safe->dirty=false;safe->updateTitle();
            safe->browser.infoView.show({"Project saved","The .aup file includes the session and its embedded sample data."});
            if(after&&!safe->dirty)after();
        });
    });
}
void Workspace::openProject(const juce::File& file)
{
    requestClose([safe=juce::Component::SafePointer<Workspace>(this),file]
    {
        if(!safe)return;safe->projectBusy=true;safe->setEnabled(false);
        safe->projectWorker.addJob([safe,file]
        {
            auto data=std::make_shared<ProjectSnapshot>();const auto result=ProjectFile::load(file,*data);
            juce::MessageManager::callAsync([safe,file,data,result]
            {
                if(!safe)return;safe->setEnabled(true);safe->projectBusy=false;
                if(result.failed()){juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Open failed",result.getErrorMessage());return;}
                safe->projectFile=file;safe->applyProject(std::move(*data));
            });
        });
    });
}
void Workspace::exportAudio()
{
    if(projectBusy||importing)return;
    auto* dialog=new juce::AlertWindow("Export audio","Render the routed Master output to a stereo WAV. Includes instruments, clip edits, effects, sends and Master processing. Playback does not need to be running.",juce::MessageBoxIconType::NoIcon);
    dialog->addComboBox("rate",{"44100 Hz","48000 Hz","96000 Hz"},"Sample rate");dialog->getComboBoxComponent("rate")->setSelectedId(2);
    dialog->addComboBox("bits",{"16 bit","24 bit","32 bit"},"PCM resolution");dialog->getComboBoxComponent("bits")->setSelectedId(2);
    dialog->addComboBox("range",{"Whole arrangement","Loop range"},"Range");
    dialog->addTextEditor("tail","2","Tail (seconds)");
    dialog->addButton("Export...",1);dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<Workspace>(this),dialog](int answer)
    {
        if(!safe||answer!=1)return;
        const double rates[]={44100,48000,96000};const int depths[]={16,24,32};
        const double rate=rates[dialog->getComboBoxComponent("rate")->getSelectedItemIndex()];const int bits=depths[dialog->getComboBoxComponent("bits")->getSelectedItemIndex()];
        const bool loop=dialog->getComboBoxComponent("range")->getSelectedItemIndex()==1;
        const double tail=std::clamp(dialog->getTextEditorContents("tail").getDoubleValue(),0.0,30.0);
        safe->projectChooser=std::make_unique<juce::FileChooser>("Export Master audio",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Auralis export.wav"),"*.wav");
        safe->projectChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe,rate,bits,loop,tail](const juce::FileChooser& chooser)
        {
            if(!safe||chooser.getResult()==juce::File{})return;
            const auto exportFile=chooser.getResult().withFileExtension("wav");
            safe->captureProject([safe,rate,bits,loop,tail,exportFile](ProjectSnapshot data)
            {
            if(!safe)return;
            safe->exportPlugins.clear();
            for(auto& channel:data.channels)for(auto& d:channel.devices)if(d.kind==DeviceKind::external)
            {
                juce::String error;auto instance=NativePlugin::create(d,rate,error);
                if(!instance){juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Export cancelled",juce::String(d.pluginName)+": "+error);safe->exportPlugins.clear();return;}
                d.hosted=instance;safe->exportPlugins.push_back(instance);
            }
            safe->projectBusy=true;
            safe->browser.infoView.show({"Exporting Master","Rendering audio to a temporary WAV. The destination is replaced only after a successful render."});
            safe->projectWorker.addJob([safe,data=std::move(data),file=exportFile,rate,bits,loop,tail]
            {
                auto result=juce::Result::ok();
                try
                {
                    MixerState mixer;mixer.restore(data.channels,data.routes,data.selected);MidiProject document;document.clips=data.midi;document.audioClips=data.audio;
                    AudioOutput engine;AudioOutput::Plan plan;plan.clips=data.audio;plan.tempo=data.transport.tempo;plan.playing=true;plan.seek=1;
                    Tick end=0;for(const auto& c:data.midi)end=std::max(end,c.start+c.length);for(const auto& c:data.audio)end=std::max(end,c.start+c.length);
                    plan.start=loop?data.transport.loopStartBeats*60/plan.tempo:0;
                    const double finish=loop?data.transport.loopEndBeats*60/plan.tempo:end*60.0/(plan.tempo*editing::ppq);
                    if(finish<=plan.start)throw std::runtime_error("The export range is empty.");
                    const double start=plan.start;plan.renderEnd=finish;engine.connect(plan,mixer,document);engine.publish(std::move(plan));
                    juce::TemporaryFile temp(file);std::unique_ptr<juce::OutputStream> stream=temp.getFile().createOutputStream();if(!stream)throw std::runtime_error("Cannot create export file.");
                    juce::WavAudioFormat format;auto writer=format.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(rate).withNumChannels(2).withBitsPerSample(bits));
                    if(!writer)throw std::runtime_error("Cannot create WAV writer.");
                    std::array<float,512> left{},right{};const float* buffers[]={left.data(),right.data()};
                    const auto frames=static_cast<int64_t>(std::ceil((finish-start+tail)*rate));
                    for(int64_t position=0;position<frames;)
                    {
                        const int count=static_cast<int>(std::min<int64_t>(512,frames-position));engine.renderOffline(left.data(),right.data(),count,rate);
                        if(!writer->writeFromFloatArrays(buffers,2,count))throw std::runtime_error("Writing exported audio failed.");position+=count;
                    }
                    if(!writer->flush())throw std::runtime_error("Flushing exported audio failed.");
                    writer.reset();if(!temp.overwriteTargetFileWithTemporary())throw std::runtime_error("Cannot replace the export destination.");
                }
                catch(const std::exception& error){result=juce::Result::fail(error.what());}
                juce::MessageManager::callAsync([safe,result,file]
                {if(!safe)return;safe->projectBusy=false;juce::AlertWindow::showMessageBoxAsync(result.wasOk()?juce::MessageBoxIconType::InfoIcon:juce::MessageBoxIconType::WarningIcon,result.wasOk()?"Export complete":"Export failed",result.wasOk()?file.getFullPathName():result.getErrorMessage());});
            });
            });
        });
    }),true);
}
}
