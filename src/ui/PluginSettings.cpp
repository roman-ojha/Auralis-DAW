#include "PluginSettings.h"
namespace auralis
{
PluginSettings::PluginSettings()
{
    preferences=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Auralis").getChildFile("plugin-catalogue.xml");
    paths.setMultiLine(true);paths.setReturnKeyStartsNewLine(true);paths.setTextToShowWhenEmpty("One plugin folder per line",colour(design::colour::muted));
    setHelp(paths,"Plugin search folders","One folder per line. Scan searches these folders for Windows x64 VST3 plugins. No binary is loaded until you explicitly scan or insert it.");
    if(preferences.getSize()<4*1024*1024)if(auto xml=juce::parseXML(preferences))
    {
        paths.setText(xml->getStringAttribute("paths"));
        for(const auto* child:xml->getChildIterator())
        {if(child->hasTagName("Quarantine")){quarantine.addIfNotAlreadyThere(child->getStringAttribute("path"));continue;}PluginRecord record;if(record.description.loadFromXml(*child)){record.favourite=child->getBoolAttribute("favourite");plugins.push_back(record);}}
    }
    configureButton(addFolder,"Add folder...","Add a plugin search folder. Existing folder links and favorites are preserved.");
    addFolder.onClick=[this]
    {
        chooser=std::make_unique<juce::FileChooser>("Choose plugin folder");
        chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,[safe=juce::Component::SafePointer<PluginSettings>(this)](const juce::FileChooser& dialog)
        {if(safe&&dialog.getResult().isDirectory()){safe->paths.setText(safe->paths.getText()+"\n"+dialog.getResult().getFullPathName());safe->save();}});
    };
    configureButton(scan,"Scan folders","Scan VST3 candidates in a separate helper process. Crashes and a 20-second timeout cannot close this DAW. Star results to expose them in the Plug-Ins browser.");scan.onClick=[this]{scanPlugins();};
    configureButton(cancel,"Cancel scan","Stop scanning and terminate the current scanner helper. Previously discovered plugins are kept.");cancel.onClick=[this]{cancelled=true;};cancel.setEnabled(false);
    configureButton(retry,"Retry failed","Clear scan quarantine. The next scan retries failed or timed-out plugins only when you explicitly click Scan.");retry.onClick=[this]{quarantine.clear();save();status.setText("Quarantine cleared. Click Scan to retry.",juce::dontSendNotification);};
    setHelp(list,"Discovered VST3 plugins","Click a row to star/unstar the plugin in the browser. Native plugin editors open from their rack cards. VST2 DLL, CLAP, AU and 32-bit bridging are not supported by this build.");
    for(juce::Component* c:std::initializer_list<juce::Component*>{&paths,&addFolder,&scan,&cancel,&retry,&status,&list})addAndMakeVisible(c);
    list.setRowHeight(38);setSize(780,570);
}
PluginSettings::~PluginSettings(){cancelled=true;worker.removeAllJobs(true,-1);save();list.setModel(nullptr);}
void PluginSettings::save()
{
    juce::XmlElement xml("AuralisPluginCatalogue");xml.setAttribute("paths",paths.getText());
    for(const auto& record:plugins){auto item=record.description.createXml();item->setAttribute("favourite",record.favourite);xml.addChildElement(item.release());}
    for(const auto& path:quarantine){auto* entry=xml.createNewChildElement("Quarantine");entry->setAttribute("path",path);}
    if(preferences.getParentDirectory().createDirectory().failed()){status.setText("Cannot create plugin preferences folder.",juce::dontSendNotification);return;}
    juce::TemporaryFile temp(preferences);if(!xml.writeTo(temp.getFile())||!temp.overwriteTargetFileWithTemporary())status.setText("Could not save plugin preferences.",juce::dontSendNotification);
}
void PluginSettings::scanPlugins()
{
    if(worker.getNumJobs()>0)return;
    save();cancelled=false;scan.setEnabled(false);cancel.setEnabled(true);
    status.setText("Scanning selected folders...",juce::dontSendNotification);
    const auto folders=juce::StringArray::fromLines(paths.getText());const auto previous=plugins;const auto previousQuarantine=quarantine;
    worker.addJob([this,folders,previous,previousQuarantine,safe=juce::Component::SafePointer<PluginSettings>(this)]
    {
        auto found=previous;auto blocked=previousQuarantine;juce::StringArray failures;
        const auto helper=juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("AuralisPluginScanner.exe");
        if(!helper.existsAsFile())failures.add("Scanner helper is missing beside Auralis.exe.");
        else
        {
            juce::FileSearchPath search;for(const auto& path:folders)if(path.trim().isNotEmpty())search.add(juce::File(path.trim()));
            juce::VST3PluginFormat format;auto candidates=format.searchPathsForPlugins(search,true,false);
            if(candidates.size()>2048)candidates.removeRange(2048,candidates.size()-2048);
            for(const auto& candidate:candidates)
            {
                if(cancelled.load())break;
                if(blocked.contains(candidate)){failures.add(juce::File(candidate).getFileName()+": quarantined (Retry failed to rescan)");continue;}
                juce::TemporaryFile result(".xml");juce::ChildProcess process;
                if(!process.start({helper.getFullPathName(),candidate,result.getFile().getFullPathName()},0)){failures.add(juce::File(candidate).getFileName()+": scanner could not start");continue;}
                const auto started=juce::Time::getMillisecondCounter();
                while(process.isRunning()&&!cancelled.load()&&juce::Time::getMillisecondCounter()-started<20000)juce::Thread::sleep(20);
                if(process.isRunning()){process.kill();if(!cancelled.load())blocked.addIfNotAlreadyThere(candidate);failures.add(juce::File(candidate).getFileName()+": timed out or cancelled");continue;}
                if(process.getExitCode()!=0||result.getFile().getSize()>2*1024*1024){blocked.addIfNotAlreadyThere(candidate);failures.add(juce::File(candidate).getFileName()+": scan failed");continue;}
                auto xml=juce::parseXML(result.getFile());if(!xml||xml->getNumChildElements()==0){blocked.addIfNotAlreadyThere(candidate);failures.add(juce::File(candidate).getFileName()+": no valid scan result");continue;}
                for(const auto* entry:xml->getChildIterator())
                {
                    PluginRecord record;if(!record.description.loadFromXml(*entry))continue;
                    const auto existing=std::find_if(found.begin(),found.end(),[&](const auto& p){return p.description.createIdentifierString()==record.description.createIdentifierString();});
                    if(existing==found.end()&&found.size()<2048)found.push_back(record);
                }
            }
        }
        juce::MessageManager::callAsync([safe,found=std::move(found),failures,blocked]
        {
            if(!safe)return;safe->plugins=found;safe->quarantine=blocked;safe->list.updateContent();safe->scan.setEnabled(true);safe->cancel.setEnabled(false);safe->save();
            safe->status.setText(juce::String(found.size())+" plugins. Click rows to star favorites. "+juce::String(failures.size())+" scan failures.",juce::dontSendNotification);
            if(failures.size()>0)juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Plugin scan results",failures.joinIntoString("\n").substring(0,8000));
            if(safe->onChanged)safe->onChanged();
        });
    });
}
void PluginSettings::paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool selected)
{
    if(row<0||row>=getNumRows())return;const auto& item=plugins[row];
    g.fillAll(colour(selected?design::colour::raised:design::colour::panel));
    text(g,item.favourite?juce::String::charToString(0x2605):juce::String::charToString(0x2606),{8,0,28,height},21,design::colour::amber);
    text(g,item.description.name,{42,0,width/2-42,height},13,design::colour::text,true);
    text(g,"VST3 / "+juce::String(item.description.isInstrument?"Instrument":"Effect")+" / "+item.description.manufacturerName,{width/2,0,width/2-8,height},11,design::colour::muted);
}
void PluginSettings::listBoxItemClicked(int row,const juce::MouseEvent&)
{if(row>=0&&row<getNumRows()){plugins[row].favourite=!plugins[row].favourite;list.repaintRow(row);save();if(onChanged)onChanged();}}
void PluginSettings::paint(juce::Graphics& g)
{
    g.fillAll(colour(design::colour::background));text(g,"PLUG-INS / WINDOWS x64 VST3",{18,10,getWidth()-36,26},17,design::colour::mint,true);
    text(g,"VST2 .dll / CLAP / AU are unsupported. Scanning is isolated; loaded plugins run inside Auralis.",{18,38,getWidth()-36,24},11,design::colour::muted);
}
void PluginSettings::resized()
{
    paths.setBounds(18,70,getWidth()-36,95);addFolder.setBounds(18,176,130,28);scan.setBounds(158,176,130,28);cancel.setBounds(298,176,110,28);retry.setBounds(418,176,130,28);
    status.setBounds(18,210,getWidth()-36,32);list.setBounds(18,248,getWidth()-36,getHeight()-266);
}
}
