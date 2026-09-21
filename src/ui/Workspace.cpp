#include "Workspace.h"
namespace auralis
{
void DeviceArea::paint(juce::Graphics& g)
{
    panel(g, getLocalBounds());
    text(g, "Device path", {18, 10, 118, 26}, 17, design::colour::text, true);
    text(g, trackName, {146, 12, 180, 24}, 12, tint, true);
    text(g, "SIGNAL FLOWS LEFT TO RIGHT", {getWidth()-260, 14, 240, 20}, 9, design::colour::muted, true, juce::Justification::centredRight);
    g.setColour(colour(design::colour::line)); g.drawHorizontalLine(45, 16, static_cast<float>(getWidth()-16));
    auto chain = getLocalBounds().withTrimmedTop(58).withTrimmedBottom(16).reduced(22, 0);
    const int centre = chain.getCentreY();
    auto input = chain.removeFromLeft(78); auto output = chain.removeFromRight(80);
    text(g, "INPUT", input.withHeight(24).withY(centre-24), 9, design::colour::muted, true, juce::Justification::centred);
    text(g, "No source", input.withHeight(24).withY(centre-2), 11, design::colour::text, false, juce::Justification::centred);
    text(g, "OUTPUT", output.withHeight(24).withY(centre-24), 9, design::colour::muted, true, juce::Justification::centred);
    text(g, "Master", output.withHeight(24).withY(centre-2), 11, design::colour::text, false, juce::Justification::centred);
    g.setColour(colour(design::colour::line)); g.drawHorizontalLine(centre, static_cast<float>(chain.getX()), static_cast<float>(chain.getRight()));
    auto blank = chain.reduced(30, 4);
    g.setColour(colour(design::colour::background)); g.fillRoundedRectangle(blank.toFloat(), 9);
    g.setColour(colour(design::colour::line)); g.drawRoundedRectangle(blank.toFloat(), 9, 1);
    text(g, "Your sound starts here", {blank.getX(), centre-27, blank.getWidth(), 26}, 18, design::colour::text, true, juce::Justification::centred);
    text(g, "A space for instruments, samples and effects.", {blank.getX()+10, centre+1, blank.getWidth()-20, 21}, 12, design::colour::muted, false, juce::Justification::centred);
    if (blank.getHeight() > 95) text(g, "No devices loaded  /  Device editing comes next", {blank.getX(), centre+27, blank.getWidth(), 18}, 10, design::colour::muted, false, juce::Justification::centred);
    text(g, ">", {chain.getRight()-25, centre-12, 23, 24}, 16, design::colour::mint, false, juce::Justification::centred);
}
Workspace::Workspace()
{
    setLookAndFeel(&theme); setWantsKeyboardFocus(true);
    for (juce::Component* component : std::initializer_list<juce::Component*>{&menu, &transport, &browser, &arrangement, &piano, &mixer, &devices, &browserDivider, &devicesDivider}) addAndMakeVisible(component);
    mixer.setVisible(false); piano.setVisible(false);
    transport.onViewChanged = [this](int view)
    {
        activeView=view; arrangement.setVisible(view==0); mixer.setVisible(view==1); piano.setVisible(view==2);
        if(view==0)arrangement.focusTimeline();
        if(view==2) { piano.refresh(); piano.grabKeyboardFocus(); }
        resized();
    };
    transport.onSnapChanged=[this](int index){midi.snapIndex=index;midi.changed();};
    arrangement.onOpen=[this](int id){midi.active=id;if(const auto* c=midi.clip(id))tracks.select(c->track);transport.showView(2);};
    arrangement.onLoop=[this](Tick start,Tick end){state.loopStartBeats=static_cast<double>(start)/editing::ppq;state.loopEndBeats=static_cast<double>(end)/editing::ppq;state.loop=true;transport.refresh();};
    midi.onChanged=[this]{arrangement.refresh();piano.refresh();transport.setSnap(midi.snapIndex);};
    browserDivider.onDrag = [this](int delta) { layout.browser += delta; resized(); };
    devicesDivider.onDrag = [this](int delta) { layout.devices -= delta; resized(); };
    browserDivider.onReset = [this] { layout.browser = design::browserWidth; resized(); };
    devicesDivider.onReset = [this] { layout.devices = design::devicesHeight; resized(); };
    menu.onReset = [this] { resetLayout(); };
    menu.onView = [this](int view) { transport.showView(view); };
    menu.onCreateClip=[this]{arrangement.createClip();};
    menu.onShortcuts=[this]{showShortcuts();};
    menu.onUndo=[this](bool redo){if(redo)midi.redo();else midi.undo();};
    menu.onChooseAudio = [this] { browser.chooseAudio(); };
    menu.onTransportChange = [this] { transport.refresh(); };
    setHelp(devices, "Device path", "This panel follows the selected track. Future instruments, samples and effects will appear left to right. No devices or processing are loaded yet.");
    tracks.onChanged = [this]
    {
        arrangement.refresh(); mixer.refresh();
        devices.trackName = juce::String(tracks.selectedChannel().name);
        devices.tint = tracks.selectedChannel().tint;
        devices.repaint();
        auto* hovered = juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
        if (hovered && isParentOf(hovered) && hovered != &browser.infoView && !browser.infoView.isParentOf(hovered))
            browser.infoView.show(resolveHelp(hovered, juce::Desktop::getMousePosition()));
    };
    setSize(design::initialWidth, design::initialHeight);
    lastTick = juce::Time::getMillisecondCounterHiRes(); lastMetrics = lastTick; startTimerHz(design::timerHz);
}
Workspace::~Workspace() { stopTimer(); tracks.onChanged = {}; midi.onChanged={}; shortcuts.reset(); setLookAndFeel(nullptr); }
void Workspace::resetLayout() { layout = {}; browser.resetLayout(); arrangement.resetLayout(); mixer.resetLayout(); resized(); }
void Workspace::resized()
{
    layout.constrain(getWidth(), getHeight()); auto area = getLocalBounds();
    menu.setBounds(area.removeFromTop(design::menuHeight));
    transport.setBounds(area.removeFromTop(design::transportHeight));
    area.removeFromBottom(design::footerHeight); area.reduce(design::panelGap, 0);
    browser.setBounds(area.removeFromLeft(layout.browser)); browserDivider.setBounds(area.removeFromLeft(design::panelGap));
    devices.setBounds(area.removeFromBottom(layout.devices)); devicesDivider.setBounds(area.removeFromBottom(design::panelGap)); arrangement.setBounds(area);
    mixer.setBounds(area); piano.setBounds(area);
}
void Workspace::timerCallback()
{
    const auto pointer = juce::Desktop::getMousePosition();
    auto* focus = juce::Component::getCurrentlyFocusedComponent();
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
    const bool wasPlaying = state.playing;
    state.advance((now-lastTick)/1000.0); lastTick = now;
    const bool metricsDue = now-lastMetrics >= design::metricsIntervalMs;
    if (metricsDue) { transport.sampleMetrics(); lastMetrics = now; }
    if (wasPlaying || metricsDue) transport.refresh();
    const Tick bar=editing::ppq*state.numerator*4/state.denominator;
    if(midi.barTicks!=bar){midi.barTicks=bar;midi.changed();}
    arrangement.setPlayhead(state.barPosition(), state.numerator);
    arrangement.setLoop(static_cast<Tick>(state.loopStartBeats*editing::ppq),static_cast<Tick>(state.loopEndBeats*editing::ppq),state.loop);
}
bool Workspace::keyPressed(const juce::KeyPress& key)
{
    juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("key-debug.txt").appendText("Workspace " + juce::String(key.getKeyCode())+" mods="+juce::String(key.getModifiers().getRawFlags())+" text="+juce::String(static_cast<int>(key.getTextCharacter()))+"\n"); // TEMP_KEY_DIAGNOSTIC
    // Do not steal shortcuts from an editor (including gain/tempo numeric entry).
    auto* focus=juce::Component::getCurrentlyFocusedComponent();
    for(auto* c=focus;c&&c!=this;c=c->getParentComponent())if(dynamic_cast<juce::TextEditor*>(c))return false;
    const int rawCode=key.getKeyCode();const int code=(rawCode>='a'&&rawCode<='z')?rawCode-'a'+'A':rawCode;const bool ctrl=key.getModifiers().isCtrlDown();
    if(code==juce::KeyPress::F1Key){showShortcuts();return true;}
    if(code==juce::KeyPress::F5Key||code==juce::KeyPress::F7Key||code==juce::KeyPress::F9Key){transport.showView(code==juce::KeyPress::F5Key?0:code==juce::KeyPress::F9Key?1:2);return true;}
    if(ctrl&&(code=='Z'||code=='Y')){if(code=='Y'||key.getModifiers().isShiftDown())midi.redo();else midi.undo();return true;}
    if(code==juce::KeyPress::spaceKey){if(ctrl)state.playing=!state.playing;else if(state.playing)state.stop();else state.playing=true;}
    else if(code==juce::KeyPress::escapeKey)state.stop();
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
}

