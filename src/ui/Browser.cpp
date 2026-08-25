#include "Browser.h"
#include "model/DeviceState.h"
#include "constants/Library.h"
#include <BinaryData.h>

namespace auralis
{
namespace
{


void drawCategoryIcon(juce::Graphics& g, juce::Rectangle<float> r, int kind)
{
    if (kind == 0)
    {
        g.drawRoundedRectangle(r, 3, 1.3f);
        for (int i = 1; i < 4; ++i)
        {
            const float x = r.getX() + r.getWidth() * static_cast<float>(i) / 4;
            g.drawLine(x, r.getCentreY(), x, r.getBottom(), 1.2f);
        }
    }
    else if (kind == 1)
    {
        for (int i = 0; i < 5; ++i)
        {
            const float x = r.getX() + static_cast<float>(i) * r.getWidth() / 4;
            const float h = i == 2 ? 1.0f : (i % 2 == 0 ? 0.3f : 0.65f);
            g.drawLine(x, r.getCentreY()-h*r.getHeight()/2, x, r.getCentreY()+h*r.getHeight()/2, 1.7f);
        }
    }
    else
    {
        for (int i = 0; i < 3; ++i)
        {
            const float x = r.getX() + 2 + static_cast<float>(i) * 7;
            const float y = r.getY() + (i == 1 ? 12.0f : 5.0f);
            g.drawLine(x, r.getY(), x, r.getBottom(), 1.2f);
            g.fillRoundedRectangle(x-2.5f, y-2, 5, 4, 1);
        }
    }
}

class LibraryItem final : public juce::TreeViewItem
{
public:
    LibraryItem(juce::String title, juce::String detail, std::uint32_t ink, bool group,
                std::function<void(const juce::String&)> onSelect = {})
        : title(std::move(title)), detail(std::move(detail)), ink(ink), group(group), onSelect(std::move(onSelect)) {}
    juce::String pluginDrag;
    juce::var getDragSourceDescription() override
    {if(pluginDrag.isNotEmpty())return pluginDrag;if(!group)for(int i=0;i<10;++i)if(title==deviceName(static_cast<DeviceKind>(i)))return "device:"+juce::String(i);return {};}
    bool mightContainSubItems() override { return group; }
    bool canBeSelected() const override { return !group; }
    int getItemHeight() const override { return group ? design::browser::groupHeight : design::browser::itemHeight; }
    juce::String getUniqueName() const override { return title; }
    juce::String getTooltip() override { return title + (group ? ": expand or collapse this subsection." : ": " + detail + ". Drag into a compatible track or the lower device chain."); }
    juce::String getAccessibilityName() override { return title + (group ? ", subsection" : ", " + detail + ", built-in device"); }
    void itemClicked(const juce::MouseEvent&) override { if (group) setOpen(!isOpen()); }
    void itemSelectionChanged(bool isNowSelected) override { if (isNowSelected && onSelect) onSelect(title); }
    void paintItem(juce::Graphics& g, int width, int height) override
    {
        if (group)
        {
            text(g, title, {4, 0, width-32, height}, 12, design::colour::text, true);
            text(g, juce::String(getNumSubItems()), {width-27, 0, 23, height}, 10, design::colour::muted, false, juce::Justification::centred);
            return;
        }
        g.setColour(colour(isSelected() ? design::colour::raised : design::colour::panel));
        g.fillRoundedRectangle(0, 2, static_cast<float>(width-4), static_cast<float>(height-4), 6);
        g.setColour(colour(ink)); g.fillRoundedRectangle(4, 13, 3, 29, 1.5f);
        text(g, title, {15, 6, width-23, 21}, 13, design::colour::text, true);
        text(g, detail, {15, 28, width-23, 20}, 10, design::colour::muted);
    }
private:
    juce::String title, detail;
    std::uint32_t ink;
    bool group;
    std::function<void(const juce::String&)> onSelect;
};
}

CategoryButton::CategoryButton(const juce::String& name, int icon) : Button(name), icon(icon)
{
    setClickingTogglesState(true); setRadioGroupId(1);
    setTitle(name); setTooltip("Browse " + name.toLowerCase() + " devices");
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void CategoryButton::paintButton(juce::Graphics& g, bool hover, bool down)
{
    const auto ink = library::categoryInks[static_cast<size_t>(icon)];
    if (getToggleState() || hover || down)
    {
        g.setColour(getToggleState() ? colour(ink).withAlpha(0.12f) : colour(design::colour::raised));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 6);
    }
    if (getToggleState())
    {
        g.setColour(colour(ink)); g.fillRoundedRectangle(1, 10, 2, static_cast<float>(getHeight()-20), 1);
    }
    g.setColour(colour(getToggleState() ? ink : design::colour::muted));
    drawCategoryIcon(g, {10, static_cast<float>((getHeight()-design::browser::iconSize)/2),
                         static_cast<float>(design::browser::iconSize), static_cast<float>(design::browser::iconSize)}, icon);
    auralis::text(g, getName(), {37, 0, getWidth()-40, getHeight()}, 11, getToggleState() ? ink : design::colour::text, getToggleState());
    if (hasKeyboardFocus(true))
    {
        g.setColour(colour(ink)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 6, 1);
    }
}
Browser::Browser()
{
    addAndMakeVisible(infoView);
    addAndMakeVisible(waveform);
    waveform.onLoaded=[this](std::shared_ptr<const AudioData> media){previewMedia=std::move(media);if(onPreview)onPreview(previewMedia);};
    waveform.onReplay=[this]{if(previewMedia&&onPreview)onPreview(previewMedia);};
    waveform.onClear=[this]{previewMedia.reset();if(onStopPreview)onStopPreview();};
    juce::PropertiesFile::Options options;
    options.applicationName="Auralis";options.filenameSuffix="settings";options.folderName="Auralis";
    options.storageFormat=juce::PropertiesFile::storeAsXML;
    preferences=std::make_unique<juce::PropertiesFile>(options);
    folders.addLines(preferences->getValue("libraryFolders"));folders.removeEmptyStrings();folders.removeDuplicates(true);
    while(folders.size()>audio::maximumLibraries)folders.remove(folders.size()-1);
    configureButton(addLibrary,"+ Add Library","Link a sample-pack folder in the sidebar. Folder locations persist on this computer; source audio is never moved or changed. Maximum 32 folders.");
    addLibrary.onClick=[this]{addFolder();};addAndMakeVisible(addLibrary);
    configureButton(removeLibrary,"Unlink folder","Remove the selected folder link from this sidebar. Files on disk and imported clips are preserved.");
    removeLibrary.onClick=[this]
    {
        if(category<4)return;
        folders.remove(category-4);preferences->setValue("libraryFolders",folders.joinIntoString("\n"));
        if(!preferences->saveIfNeeded())infoView.show({"Library settings","Could not save folder settings."});
        selectCategory(0);rebuildFolders();
    };
    addAndMakeVisible(removeLibrary);
    categoryViewport.setViewedComponent(&categoryList,false);categoryViewport.setScrollBarsShown(true,false);addAndMakeVisible(categoryViewport);
    files.addListener(this);files.setTitle("Sample library folders and audio files");files.setDefaultOpenness(false);addChildComponent(files);
    directoryThread.startThread();rebuildFolders();
    setHelp(*this, "Library", "Choose Instruments, Sounds or Effects on the left; expand subsections on the right. Drag built-in devices to tracks or the lower device chain. Add Library links a sample folder. Select files to preview; drag them onto the Audio lane. Click the waveform to replay. Folder links persist across restarts.");
    const auto data = juce::JSON::parse(juce::String::fromUTF8(BinaryData::library_json, BinaryData::library_jsonSize));
    if (const auto* array = data.getArray())
        for (const auto& value : *array)
            entries.push_back({value["name"].toString(), value["kind"].toString(), value["description"].toString(), value["section"].toString()});
    search.setTextToShowWhenEmpty("Search instruments", colour(design::colour::muted));
    search.setTitle("Search selected library category");
    search.setDescription("Filter the current category by name, description or subsection. Changing category clears this search.");
    search.onTextChange = [this] { filter(); };
    addAndMakeVisible(search);
    std::array<CategoryButton*, 4> buttons{&instruments, &sounds, &effects, &plugins};
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        buttons[i]->onClick = [this, i] { selectCategory(static_cast<int>(i)); };
        addAndMakeVisible(*buttons[i]);
    }
    tree.setRootItemVisible(false);
    tree.setDefaultOpenness(true);
    tree.setMultiSelectEnabled(false);
    tree.setIndentSize(design::browser::indent);
    tree.setColour(juce::TreeView::backgroundColourId, colour(design::colour::panel));
    tree.setColour(juce::TreeView::selectedItemBackgroundColourId, colour(design::colour::raised));
    tree.setColour(juce::TreeView::linesColourId, colour(design::colour::muted));
    tree.setTitle("Library subsections and built-in devices");
    addAndMakeVisible(tree); addAndMakeVisible(divider);
    divider.onDrag = [this](int delta) { navigationWidth += delta; resized(); };
    divider.onReset = [this] { resetLayout(); };
    selectCategory(0);
}
Browser::~Browser() { files.removeListener(this); directoryThread.stopThread(-1); tree.setRootItem(nullptr); }
HelpContent Browser::helpAt(juce::Point<int> point, bool keyboard) const
{
    if(category>=4&&files.getBounds().contains(point))
        return {"Sample library", "Expand folders and click a WAV, AIFF or FLAC file to hear it. Drag a file onto an Audio lane or the empty drop zone to import it. Click the waveform to replay. Filename search filters files inside browsed folders; files remain unchanged."};
    if (tree.getBounds().contains(point))
        if (auto* item = keyboard ? tree.getSelectedItem(0) : tree.getItemAt(point.y-tree.getY()))
            return {item->getUniqueName(), item->getTooltip()};
    return {"Library", "Choose Instruments, Sounds or Effects on the left; expand subsections on the right. Drag built-in devices to tracks or the lower device chain. Add Library links a sample folder. Select files to preview; drag them onto the Audio lane. Click the waveform to replay. Folder links persist across restarts."};
}
void Browser::resetLayout() { navigationWidth = design::browser::navigationWidth; resized(); }
void Browser::selectCategory(int next)
{
    category = next;
    removeLibrary.setEnabled(category>=4);
    for(size_t i=0;i<folderButtons.size();++i)folderButtons[i]->setToggleState(category==static_cast<int>(i)+4,juce::dontSendNotification);
    instruments.setToggleState(category == 0, juce::dontSendNotification);
    sounds.setToggleState(category == 1, juce::dontSendNotification);
    effects.setToggleState(category == 2, juce::dontSendNotification);
    plugins.setToggleState(category == 3,juce::dontSendNotification);
    search.setTextToShowWhenEmpty("Search " + (category<4?juce::String(library::categoryNames[static_cast<size_t>(category)]).toLowerCase():juce::String("library")), colour(design::colour::muted));
    // A category switch starts with all of its subsections, never an old hidden query.
    search.setText({}, false);
    filter();
}
void Browser::filter()
{
    files.setVisible(category>=4);tree.setVisible(category<4);
    if(category>=4)
    {
        {const std::lock_guard lock(audioFilter.mutex);audioFilter.query=search.getText().trim();}
        const juce::File folder(folders[category-4]);
        if(!folder.isDirectory())infoView.show({"Library folder unavailable","This folder has moved or its drive is disconnected. Reconnect the drive, or unlink this entry and add its new location. No files were deleted."});
        if(directory.getDirectory()!=folder)directory.setDirectory(folder,true,true);else directory.refresh();
        waveform.clear();repaint();return;
    }
    tree.setRootItem(nullptr);
    root = std::make_unique<LibraryItem>("Library", "", design::colour::muted, true);
    const auto query = search.getText().trim();
    juce::StringArray sections;
    matchCount = 0; selection.clear(); waveform.clear();
    if(category==3)
    {
        for(const auto& entry:pluginEntries)if((entry.name+" "+entry.description).containsIgnoreCase(query))
        {
            auto* item=new LibraryItem(entry.name,entry.description,design::colour::blue,false);
            item->pluginDrag="plugin:"+entry.identity;root->addSubItem(item);++matchCount;
        }
        tree.setRootItem(root.get());root->setOpen(true);repaint();return;
    }
    const auto kind = library::categoryKinds[static_cast<size_t>(category)];
    for (const auto& entry : entries)
        if (entry.kind == kind && (entry.name+" "+entry.description+" "+entry.section).containsIgnoreCase(query))
            sections.addIfNotAlreadyThere(entry.section);
    for (const auto& section : sections)
    {
        auto group = std::make_unique<LibraryItem>(section, "", design::colour::text, true);
        for (const auto& entry : entries)
            if (entry.kind == kind && entry.section == section && (entry.name+" "+entry.description+" "+entry.section).containsIgnoreCase(query))
            {
                group->addSubItem(new LibraryItem(entry.name, entry.description, library::categoryInks[static_cast<size_t>(category)], false,
                    [this, description = entry.description](const juce::String& name)
                    {
                        selection = name;
                        waveform.clear();
                        infoView.show({name, description + ". Drag into a compatible track or the lower device chain."});
                        repaint();
                    }));
                ++matchCount;
            }
        root->addSubItem(group.release());
    }
    tree.setRootItem(root.get()); root->setOpen(true); repaint();
}
void Browser::paint(juce::Graphics& g)
{
    panel(g, getLocalBounds());
    text(g, "LIBRARY", {16, 12, 100, 24}, 11, design::colour::muted, true);
    text(g, "EXPLORE / PREVIEW", {getWidth()-162, 12, 146, 24}, 9, design::colour::mint, true, juce::Justification::centredRight);
    text(g, "CATEGORIES", {16, design::browser::headerHeight+7, navigationWidth-8, 24}, 9, design::colour::muted, true);
    text(g, category<4?juce::String(library::categoryNames[static_cast<size_t>(category)]):juce::File(folders[category-4]).getFileName(), resultsBounds.withHeight(27), 17, design::colour::text, true);
    text(g, category<4?juce::String(matchCount)+" devices":juce::String("Folders / audio files"), resultsBounds.withTrimmedTop(27).withHeight(18), 10, design::colour::muted);
    if (category<4 && matchCount == 0)
    {
        text(g, "No matches", resultsBounds.withTrimmedTop(72).withHeight(25), 13, design::colour::text, true);
        text(g, "Try another search.", resultsBounds.withTrimmedTop(98).withHeight(22), 11, design::colour::muted);
    }
}
void Browser::resized()
{
    const int maxNavigation = juce::jmin(design::browser::navigationMax, getWidth()-design::padding*2-design::panelGap-design::browser::resultsMin);
    navigationWidth = juce::jlimit(design::browser::navigationMin, juce::jmax(design::browser::navigationMin, maxNavigation), navigationWidth);
    search.setBounds(16, 44, getWidth()-32, 30);
    infoView.setBounds(getLocalBounds().removeFromBottom(preview::infoHeight));
    auto body = getLocalBounds().reduced(design::padding, 0).withTrimmedTop(design::browser::headerHeight).withTrimmedBottom(preview::infoHeight+design::panelGap);
    const auto navigation = body.removeFromLeft(navigationWidth);
    divider.setBounds(body.removeFromLeft(design::panelGap));
    resultsBounds = body;
    int y = navigation.getY()+design::browser::sectionHeader;
    for (auto* button : {&instruments, &sounds, &effects, &plugins})
    {
        button->setBounds(navigation.getX(), y, navigation.getWidth(), design::browser::categoryHeight);
        y += design::browser::categoryHeight+4;
    }
    addLibrary.setBounds(navigation.getX(),y+4,navigation.getWidth(),28);
    removeLibrary.setBounds(navigation.getX(),y+36,navigation.getWidth(),26);
    categoryViewport.setBounds(navigation.getX(),y+68,navigation.getWidth(),juce::jmax(0,navigation.getBottom()-y-68));
    categoryList.setSize(juce::jmax(1,navigation.getWidth()-12),static_cast<int>(folderButtons.size())*34);
    for(size_t i=0;i<folderButtons.size();++i)folderButtons[i]->setBounds(0,static_cast<int>(i)*34,categoryList.getWidth(),30);
    waveform.setBounds(body.removeFromBottom(preview::waveformHeight));
    tree.setBounds(body.withTrimmedTop(design::browser::sectionHeader).withTrimmedBottom(8));
    files.setBounds(tree.getBounds());
    repaint();
}
void Browser::rebuildFolders()
{
    folderButtons.clear();
    for(int i=0;i<folders.size();++i)
    {
        auto button=std::make_unique<juce::TextButton>();
        configureButton(*button,juce::File(folders[i]).getFileName(),"Browse linked sample folder: "+folders[i]+". Click audio files to preview; drag to the Audio track.");
        button->onClick=[this,i]{selectCategory(i+4);};
        categoryList.addAndMakeVisible(*button);folderButtons.push_back(std::move(button));
    }
    resized();
}
void Browser::addFolder()
{
    if(folderChooser||folders.size()>=audio::maximumLibraries)return;
    folderChooser=std::make_unique<juce::FileChooser>("Add sample library folder");
    folderChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,
        [safe=juce::Component::SafePointer<Browser>(this)](const juce::FileChooser& chooser)
        {
            if(!safe)return;
            const auto file=chooser.getResult();
            if(file.isDirectory())
            {
                safe->folders.addIfNotAlreadyThere(file.getFullPathName(),true);
                safe->preferences->setValue("libraryFolders",safe->folders.joinIntoString("\n"));
                if(!safe->preferences->saveIfNeeded())safe->infoView.show({"Library settings","Could not save library folder locations. Check settings-folder write permissions."});
                safe->rebuildFolders();safe->selectCategory(safe->folders.indexOf(file.getFullPathName(),true)+4);
            }
            safe->folderChooser.reset();
        });
}
void Browser::selectionChanged()
{
    const auto file=files.getSelectedFile();
    files.setDragAndDropDescription(file.hasFileExtension("wav;aif;aiff;flac")?file.getFullPathName():juce::String{});
}
void Browser::fileClicked(const juce::File& file,const juce::MouseEvent&)
{
    if(file.existsAsFile())waveform.load(file);
}
}





