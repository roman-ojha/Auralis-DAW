#include "DeviceArea.h"
namespace auralis
{
namespace
{
class ModKnob final : public Knob,public juce::DragAndDropTarget
{
public:
    ModKnob(const Parameter& p,std::function<void(int)> callback):Knob(p.name,"Drag vertically or type a value. Drop ENV / LFO here to assign modulation; click the source chip to adjust assignments.",p.minimum,p.maximum,p.initial),assign(std::move(callback))
    {setNumDecimalPlacesToDisplay(p.maximum-p.minimum<=2?3:p.maximum-p.minimum<=30?2:1);setRange(p.minimum,p.maximum,p.maximum-p.minimum>100?1:.001);if(p.logarithmic)setSkewFactorFromMidPoint(std::sqrt(p.minimum*p.maximum));}
    bool isInterestedInDragSource(const SourceDetails& d) override{return d.description.toString().startsWith("mod:");}
    void itemDropped(const SourceDetails& d) override{assign(d.description.toString().fromFirstOccurrenceOf(":",false,false).getIntValue());}
private:std::function<void(int)> assign;
};
class ModSource final : public juce::TextButton
{
public:
    int source=0;
    void mouseDrag(const juce::MouseEvent& e) override
    {if(e.getDistanceFromDragStart()>4)if(auto* c=juce::DragAndDropContainer::findParentDragContainerFor(this))c->startDragging("mod:"+juce::String(source),this);}
};
}
class DeviceCard final : public juce::Component,public HelpProvider
{
public:
    DeviceCard(MixerState& m,int track,int device):model(m),trackId(track),deviceId(device)
    {
        auto* d=state();kind=d->kind;params=deviceParameters(kind);
        if(kind==DeviceKind::external)
        {
            for(size_t i=0;i<d->parameterNames.size();++i)params.push_back({d->parameterNames[i],0,1,d->externalValues[i]});
            configureButton(openEditor,"Open plug-in","Open this third-party plugin's native editor. Parameter gestures can create automation lanes when Shift+A is enabled.");
            openEditor.onClick=[this]{if(auto* s=state();s&&s->hosted)if(onEditor)onEditor(s->id);};
            addAndMakeVisible(openEditor);
        }
        configureButton(bypass,"On","Enable/bypass this device. Bypass preserves its settings.",true);bypass.setToggleState(!d->bypass,juce::dontSendNotification);
        bypass.onClick=[this]{if(auto* s=state()){s->bypass=!bypass.getToggleState();model.notifyDevices();}};
        configureButton(remove,"x","Remove this device from the selected channel.");
        remove.onClick=[safe=juce::Component::SafePointer<DeviceCard>(this)]
        {juce::MessageManager::callAsync([safe]{if(!safe)return;if(auto* c=safe->model.find(safe->trackId)){std::erase_if(c->devices,[&](const auto& d){return d.id==safe->deviceId;});safe->model.notifyDevices();}});};
        addAndMakeVisible(bypass);addAndMakeVisible(remove);addAndMakeVisible(page);
        setHelp(page,"Device section","Choose an oscillator, envelope, filter, LFO or EQ band. Each device owns independent parameters.");
        if(kind==DeviceKind::synth){for(const auto* name:{"OSC 1","OSC 2","OSC 3","ENV 1","ENV 2","ENV 3","Filter","LFO 1","LFO 2","LFO 3","LFO 4"})page.addItem(name,page.getNumItems()+1);}
        else if(kind==DeviceKind::equalizer)for(int b=0;b<7;++b)page.addItem("Band "+juce::String(b+1),b+1);
        else if(kind==DeviceKind::external){for(int start=0;start<static_cast<int>(params.size());start+=8)page.addItem("Parameters "+juce::String(start+1)+"-"+juce::String(std::min(start+8,static_cast<int>(params.size()))),page.getNumItems()+1);}
        else page.addItem("Parameters",1);
        page.setSelectedId(1,juce::dontSendNotification);page.onChange=[this]{rebuild();};
        if(kind==DeviceKind::synth)
        {
            page.setVisible(false);
            for(int i=0;i<page.getNumItems();++i)
            {
                auto tab=std::make_unique<juce::TextButton>();
                configureButton(*tab,page.getItemText(i),"Open "+page.getItemText(i)+" parameters and shape. Modulation badges show assigned sources and depths.",true);
                tab->onClick=[this,i]{page.setSelectedId(i+1,juce::dontSendNotification);rebuild();};
                addAndMakeVisible(*tab);tabs.push_back(std::move(tab));
            }
        }
        for(int i=0;i<7;++i)
        {
            auto button=std::make_unique<ModSource>();button->source=i;
            configureButton(*button,(i<3?"ENV ":"LFO ")+juce::String(i<3?i+1:i-2),"Drag this modulation source onto a Prism knob. Click to adjust or remove its assignments.");
            button->onClick=[this,i]{assignments(i);};addAndMakeVisible(*button);sources.push_back(std::move(button));
        }
        scroll.setViewedComponent(&knobPanel,false);scroll.setScrollBarsShown(true,false);addAndMakeVisible(scroll);rebuild();
    }
    ~DeviceCard() override{scroll.setViewedComponent(nullptr,false);}
    DeviceState* state(){auto* c=model.find(trackId);if(c)for(auto& d:c->devices)if(d.id==deviceId)return &d;return nullptr;}
    void rebuild()
    {
        knobs.clear();labels.clear();parameterIds.clear();int start=0,end=static_cast<int>(params.size()),section=page.getSelectedItemIndex();
        if(kind==DeviceKind::synth){if(section<3){start=section*8;end=start+8;}else if(section<6){start=24+(section-3)*5;end=start+5;}else if(section==6){start=39;end=45;}else{start=45+(section-7)*4;end=start+4;}}
        if(kind==DeviceKind::equalizer){start=section*4;end=start+4;}
        if(kind==DeviceKind::external){start=std::max(0,section)*8;end=std::min(start+8,static_cast<int>(params.size()));}
        for(int i=start;i<end;++i)
        {
            auto knob=std::make_unique<ModKnob>(params[i],[this,i](int source)
            {
                if(kind!=DeviceKind::synth)return;
                if(auto* d=state();d&&d->modulation.size()<devices::maximumModulations)
                {d->modulation.push_back({source,i,.25});model.notifyDevices();repaint();}
            });
            if(params[i].name.ends_with("Wave")||params[i].name.ends_with("Type")||params[i].name.ends_with("Unison")||params[i].name=="Root note"||params[i].name=="Model"||params[i].name=="Tempo sync")knob->setRange(params[i].minimum,params[i].maximum,1);
            if(auto* d=state())knob->setValue(kind==DeviceKind::external?d->externalValues[i]:d->values[i],juce::dontSendNotification);
            knob->onValueChange=[this,i,pointer=knob.get()]{if(auto* d=state()){model.parameter(trackId,deviceId,i,pointer->getValue());repaint();}};
            juce::String parameterHelp=juce::String(params[i].name)+": "+juce::String(params[i].minimum)+" to "+juce::String(params[i].maximum)+". Drag vertically or type a value; double-click resets. Shift+A captures touched parameters into automation.";
            if(kind==DeviceKind::synth)parameterHelp+=" Drag an ENV/LFO chip here; the badge shows its assignment and depth.";
            if(kind==DeviceKind::external)parameterHelp+=" This rack value is normalized 0 to 1; use Open plug-in for native units and controls.";
            if(kind==DeviceKind::delay)parameterHelp+=" Tempo sync: 0 off / 1 on; Beats sets synchronized time. Model: 0 mono / 1 stereo / 2 ping-pong. Dry/Wet levels trim the Mix balance.";
            setHelp(*knob,params[i].name,parameterHelp);
            parameterIds.push_back(i);
            knobPanel.addAndMakeVisible(*knob);knobs.push_back(std::move(knob));
            auto label=std::make_unique<juce::Label>();label->setText(params[i].name,juce::dontSendNotification);label->setFont(juce::Font(juce::FontOptions(10)));label->setJustificationType(juce::Justification::centred);knobPanel.addAndMakeVisible(*label);labels.push_back(std::move(label));
        }
        updateLabels();resized();repaint();
    }
    void assignments(int source)
    {
        auto* d=state();if(!d)return;juce::PopupMenu menu;
        for(size_t i=0;i<d->modulation.size();++i)if(d->modulation[i].source==source)
        {
            const auto& m=d->modulation[i];juce::PopupMenu amounts;
            amounts.addItem(static_cast<int>(i)*10+1,"Depth -100%");amounts.addItem(static_cast<int>(i)*10+2,"Depth -25%");amounts.addItem(static_cast<int>(i)*10+3,"Depth +25%");amounts.addItem(static_cast<int>(i)*10+4,"Depth +100%");amounts.addItem(static_cast<int>(i)*10+5,"Remove");
            menu.addSubMenu(juce::String(params[m.target].name)+" ("+juce::String(m.amount*100,0)+"%)",amounts);
        }
        if(menu.getNumItems()==0)menu.addItem(1,"Drag this source onto a Prism knob",false);
        menu.showMenuAsync({},[safe=juce::Component::SafePointer<DeviceCard>(this)](int result)
        {if(!safe||result<=0)return;auto* state=safe->state();const int i=(result-1)/10,choice=(result-1)%10;if(!state||i>=static_cast<int>(state->modulation.size()))return;if(choice==4)state->modulation.erase(state->modulation.begin()+i);else{constexpr double depths[]={-1,-.25,.25,1};state->modulation[i].amount=depths[choice];}safe->model.notifyDevices();});
    }
    std::function<void(int)> onEditor;
    int preferredWidth() const {return kind==DeviceKind::synth?devices::synthWidth:kind==DeviceKind::equalizer?devices::equalizerWidth:kind==DeviceKind::compressor?devices::compressorWidth:devices::cardWidth;}
    int identity() const {return deviceId;}
    void updateLabels()
    {
        auto* d=state();if(!d)return;
        for(size_t i=0;i<labels.size();++i)
        {
            juce::String caption=params[parameterIds[i]].name,details;
            for(const auto& m:d->modulation)if(m.target==parameterIds[i])
                details+=(m.source<3?"E":"L")+juce::String(m.source<3?m.source+1:m.source-2)+" "+juce::String(m.amount*100,0)+"% ";
            labels[i]->setText(caption+(details.isEmpty()?"":"\n"+details),juce::dontSendNotification);
            labels[i]->setColour(juce::Label::textColourId,colour(details.isEmpty()?design::colour::text:design::colour::mint));
            knobs[i]->setTooltip(caption+(details.isEmpty()?" / No modulation":" / Modulated by "+details));
        }
        for(size_t i=0;i<tabs.size();++i)tabs[i]->setToggleState(static_cast<int>(i)==page.getSelectedItemIndex(),juce::dontSendNotification);
    }
    void poll(SignalAnalysis::Snapshot next)
    {
        signal=next;
        std::move(history.begin()+1,history.end(),history.begin());
        history.back()={signal.input,signal.output,signal.reduction};
        updateLabels();repaint();
    }
    void resized() override
    {
        bypass.setBounds(8,7,40,24);remove.setBounds(getWidth()-32,7,24,24);
        page.setBounds(getWidth()-180,7,138,24);
        if(kind==DeviceKind::external){page.setVisible(true);page.setBounds(12,getHeight()-29,170,24);openEditor.setBounds(getWidth()-180,7,138,24);openEditor.setEnabled(state()&&state()->hosted!=nullptr);if(state()&&!state()->hosted)openEditor.setButtonText("Unavailable");}
        int top=42;
        if(kind==DeviceKind::synth)
        {
            const int width=(getWidth()-16)/static_cast<int>(tabs.size());
            for(size_t i=0;i<tabs.size();++i)tabs[i]->setBounds(8+static_cast<int>(i)*width,40,width-3,26);
            top=76;
        }
        const int graphWidth=kind==DeviceKind::equalizer?getWidth()-224:(kind==DeviceKind::synth||kind==DeviceKind::compressor)?300:170;
        graph={12,top,graphWidth,std::max(50,getHeight()-top-34)};
        for(size_t i=0;i<sources.size();++i)
        {
            sources[i]->setVisible(kind==DeviceKind::synth);
            sources[i]->setBounds(12+static_cast<int>(i)*41,getHeight()-27,39,22);
        }
        scroll.setBounds(graph.getRight()+10,top,getWidth()-graph.getRight()-18,std::max(30,getHeight()-top-8));
        const int columns=kind==DeviceKind::equalizer?2:kind==DeviceKind::synth?4:3;
        const int height=kind==DeviceKind::synth?118:112;
        knobPanel.setSize(scroll.getWidth()-12,static_cast<int>((knobs.size()+columns-1)/columns)*height);
        const int width=knobPanel.getWidth()/columns;
        for(size_t i=0;i<knobs.size();++i)
        {
            const int x=static_cast<int>(i%columns)*width,y=static_cast<int>(i/columns)*height;
            labels[i]->setBounds(x,y,width,34);knobs[i]->setBounds(x,y+34,width,height-37);
        }
    }
    void drawAnalysis(juce::Graphics& g)
    {
        g.setColour(colour(design::colour::panel));g.fillRect(graph);
        if(kind==DeviceKind::compressor)
        {
            for(int db=-48;db<=0;db+=12)
            {
                const int y=graph.getBottom()-(db+60)*graph.getHeight()/60;
                g.setColour(colour(design::colour::line));g.drawHorizontalLine(y,static_cast<float>(graph.getX()),static_cast<float>(graph.getRight()));
                text(g,juce::String(db),{graph.getX()+3,y+1,25,13},9,design::colour::muted);
            }
            const auto level=[](float v){return std::clamp((20*std::log10(std::max(1e-5f,v))+60)/60,0.0f,1.0f);};
            for(int series=0;series<3;++series)
            {
                juce::Path path;
                for(size_t i=0;i<history.size();++i)
                {
                    const float x=graph.getX()+static_cast<float>(i)*graph.getWidth()/(history.size()-1);
                    const float value=series==2?std::clamp(history[i][2]/36,0.0f,1.0f):level(history[i][series]);
                    const float y=graph.getBottom()-value*graph.getHeight();
                    if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);
                }
                g.setColour(colour(series==0?design::colour::muted:series==1?design::colour::mint:design::colour::coral));
                g.strokePath(path,juce::PathStrokeType(1.5f));
            }
            text(g,"IN / OUT / GR "+juce::String(signal.reduction,1)+" dB",graph.reduced(6).withHeight(22),10,design::colour::coral);
        }
        else
        {
            const float width=static_cast<float>(graph.getWidth())/signal.spectrum.size();
            g.setColour(colour(design::colour::mint).withAlpha(.28f));
            for(size_t i=0;i<signal.spectrum.size();++i)
            {const float height=signal.spectrum[i]*graph.getHeight();g.fillRect(graph.getX()+i*width,graph.getBottom()-height,std::max(1.0f,width-1),height);}
            for(const double hz:{20,100,1000,10000,20000})
            {
                const int x=graph.getX()+static_cast<int>(std::log(hz/20)/std::log(1000.0)*graph.getWidth());
                g.setColour(colour(design::colour::line));g.drawVerticalLine(x,static_cast<float>(graph.getY()),static_cast<float>(graph.getBottom()));
                text(g,hz<1000?juce::String(hz,0):juce::String(hz/1000,0)+"k",{std::clamp(x-12,graph.getX(),graph.getRight()-29),graph.getBottom()-16,29,16},9,design::colour::muted);
            }
        }
        text(g,"OUT "+meterText(signal.output,signal.output)+" dBFS",{getWidth()-310,8,120,23},10,design::colour::mint);
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour(colour(design::colour::background));g.fillRoundedRectangle(getLocalBounds().toFloat(),5);
        text(g,state()?juce::String(state()->displayName()):juce::String(deviceName(kind)),{58,5,140,28},18,design::colour::mint,true);
        auto* d=state();if(!d)return;
        drawAnalysis(g);
        if(kind==DeviceKind::synth&&graph.getHeight()>0)
        {
            g.setColour(colour(design::colour::line));g.drawRect(graph);juce::Path curve;
            const int section=page.getSelectedItemIndex();
            for(int x=0;x<graph.getWidth();++x)
            {
                const double t=static_cast<double>(x)/graph.getWidth();double value=.5;
                if(section<3){const int wave=static_cast<int>(d->values[section*8]);value=wave==0?.5+.45*std::sin(t*4*devices::pi):wave==1?std::fmod(t*2,1.0):wave==2?(std::fmod(t*2,1.0)<.5?.9:.1):std::abs(1-2*std::fmod(t*2,1.0));}
                else if(section<6){const int e=24+(section-3)*5;const double a=d->values[e],h=d->values[e+1],dec=d->values[e+2],release=d->values[e+4],sustain=d->values[e+3],time=t*(a+h+dec+release+.5);value=time<a?time/a:time<a+h?1:time<a+h+dec?1-(1-sustain)*(time-a-h)/dec:time<a+h+dec+.5?sustain:sustain*std::max(0.0,1-(time-a-h-dec-.5)/release);}
                else if(section==6)value=1/(1+std::pow(20*std::pow(1000.0,t)/d->values[39],2));
                else {const int i=45+(section-7)*4;value=.5+.45*std::sin(t*4*devices::pi)*std::min(1.0,t/std::max(.01,d->values[i+1]*.1));}
                const float y=static_cast<float>(graph.getBottom()-4-value*(graph.getHeight()-8));if(x==0)curve.startNewSubPath(static_cast<float>(graph.getX()),y);else curve.lineTo(static_cast<float>(graph.getX()+x),y);
            }
            g.setColour(colour(design::colour::violet));g.strokePath(curve,juce::PathStrokeType(1.5f));return;
        }
        if(kind!=DeviceKind::equalizer)return;
        g.setColour(colour(design::colour::line));g.drawRect(graph);
        for(int db=-12;db<=12;db+=6)g.drawHorizontalLine(graph.getCentreY()-db*graph.getHeight()/36,static_cast<float>(graph.getX()),static_cast<float>(graph.getRight()));
        juce::Path curve;
        for(int x=0;x<graph.getWidth();++x)
        {
            const double hz=20*std::pow(1000.0,static_cast<double>(x)/graph.getWidth());double amp=1;
            for(int b=0;b<7;++b)amp*=FilterCoefficients::make(d->values[b*4],d->values[b*4+1],d->values[b*4+2],static_cast<int>(d->values[b*4+3]),signal.sampleRate).magnitude(hz,signal.sampleRate);
            const float y=static_cast<float>(graph.getCentreY()-std::clamp(20*std::log10(std::max(1e-6,amp)),-18.0,18.0)*graph.getHeight()/36);
            if(x==0)curve.startNewSubPath(static_cast<float>(graph.getX()),y);else curve.lineTo(static_cast<float>(graph.getX()+x),y);
        }
        g.setColour(colour(design::colour::mint));g.strokePath(curve,juce::PathStrokeType(1.5f));
        for(int b=0;b<7;++b){const auto p=bandPoint(*d,b);g.setColour(juce::Colour::fromHSV(b/7.0f,.45f,.9f,1.0f));g.fillEllipse(p.x-7,p.y-7,14,14);text(g,juce::String(b+1),{static_cast<int>(p.x)-7,static_cast<int>(p.y)-8,14,16},9,design::colour::background,true,juce::Justification::centred);}
    }
    juce::Point<float> bandPoint(const DeviceState& d,int b) const{return {static_cast<float>(graph.getX()+std::log(d.values[b*4]/20)/std::log(1000.0)*graph.getWidth()),static_cast<float>(graph.getCentreY()-d.values[b*4+1]*graph.getHeight()/36)};}
    void mouseDown(const juce::MouseEvent& e) override
    {
        if(kind!=DeviceKind::equalizer||!graph.contains(e.getPosition()))return;auto* d=state();if(!d)return;double distance=20;dragBand=-1;
        for(int b=0;b<7;++b){const auto delta=bandPoint(*d,b).getDistanceFrom(e.position);if(delta<distance){distance=delta;dragBand=b;}}
        if(dragBand>=0)page.setSelectedId(dragBand+1);
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if(dragBand<0)return;if(auto* d=state())
        {d->values[dragBand*4]=std::clamp(20*std::pow(1000.0,static_cast<double>(e.x-graph.getX())/graph.getWidth()),20.0,20000.0);d->values[dragBand*4+1]=std::clamp((graph.getCentreY()-e.y)*36.0/graph.getHeight(),-18.0,18.0);model.capture(trackId,deviceId,dragBand*4,d->values[dragBand*4],20,20000,"Contour / Band "+std::to_string(dragBand+1)+" Hz",true);model.capture(trackId,deviceId,dragBand*4+1,d->values[dragBand*4+1],-18,18,"Contour / Band "+std::to_string(dragBand+1)+" dB");model.notifyDevices();repaint();}
    }
    void mouseUp(const juce::MouseEvent&) override{if(dragBand>=0){dragBand=-1;rebuild();}}
    HelpContent helpAt(juce::Point<int>,bool=false) const override
    {return {deviceName(kind),kind==DeviceKind::equalizer?"Drag a numbered band: horizontal frequency, vertical gain. Select its band page to change Q and type: 0 bell, 1 low shelf, 2 high shelf, 3 low-pass, 4 high-pass. Seven filters run in series.":"Embedded channel device. Prism wave: 0 sine, 1 saw, 2 pulse, 3 triangle. Drag ENV/LFO chips to parameters, click a chip for assignment depth/removal. Scroll controls when the panel is short. Effects process left to right before channel gain and sends. The spectrum shows measured output from 20 Hz to 20 kHz. Gravity history: gray input, mint output, coral gain reduction in dB."};}
private:
    MixerState& model;int trackId,deviceId,dragBand=-1;DeviceKind kind;
    std::vector<Parameter> params;
    std::vector<int> parameterIds;
    std::vector<std::unique_ptr<juce::TextButton>> tabs;
    SignalAnalysis::Snapshot signal;
    std::array<std::array<float,3>,180> history{};
    juce::TextButton bypass,remove,openEditor;juce::ComboBox page;
    juce::Viewport scroll;juce::Component knobPanel;juce::Rectangle<int> graph;
    std::vector<std::unique_ptr<ModKnob>> knobs;std::vector<std::unique_ptr<juce::Label>> labels;std::vector<std::unique_ptr<ModSource>> sources;
};
DeviceArea::DeviceArea(MixerState& m):model(m)
{
    viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(false,true);addAndMakeVisible(viewport);
    configureButton(add,"+ Device","Add a built-in device to the selected channel. You can also drag from the browser.");
    add.onClick=[this]
    {
        juce::PopupMenu menu;for(int i=0;i<10;++i){const auto k=static_cast<DeviceKind>(i);menu.addItem(i+1,deviceName(k),!isInstrument(k)||model.selectedChannel().type=="MIDI");}
        menu.showMenuAsync({},[safe=juce::Component::SafePointer<DeviceArea>(this),id=model.selectedId()](int result){if(safe&&result>0&&safe->onDrop)safe->onDrop("device:"+juce::String(result-1),id);});
    };
    configureButton(keyboard,"Keys (F6)","F6: computer keyboard audition. A W S E D F T G Y H U J K play chromatic notes from C4. Disable to restore letter editing shortcuts. Space still controls transport.",true);
    keyboard.onClick=[this]{if(onKeyboard)onKeyboard(keyboard.getToggleState());};
    addAndMakeVisible(add);addAndMakeVisible(keyboard);
    setHelp(*this,"Channel device chain","Drop an instrument or sample on a MIDI track, then drop effects to append right. Audio, send and Master channels accept effects. Click a track header or mixer strip to return here from the audio clip editor. Built-ins are created on demand. Star scanned VST3 plugins in Options > Plug-In Settings, then drag them from Plug-Ins. Native editors open from their rack cards.");
    refresh();
}
DeviceArea::~DeviceArea(){viewport.setViewedComponent(nullptr,false);}
void DeviceArea::refresh()
{
    const auto& c=model.selectedChannel();std::string next=std::to_string(c.id);for(const auto& d:c.devices)next+=":"+std::to_string(d.id);
    if(next!=signature){signature=next;cards.clear();for(const auto& d:c.devices){auto card=std::make_unique<DeviceCard>(model,c.id,d.id);card->onEditor=onEditor;content.addAndMakeVisible(*card);cards.push_back(std::move(card));}resized();}
    for(auto& card:cards)if(analysis)card->poll(analysis(card->identity()));
    repaint();
}
void DeviceArea::resized()
{
    add.setBounds(getWidth()-105,8,95,25);keyboard.setBounds(getWidth()-220,8,110,25);
    viewport.setBounds(8,42,getWidth()-16,getHeight()-48);
    chainWidth=0;for(const auto& card:cards)chainWidth+=card->preferredWidth()+8;
    content.setSize(std::max(viewport.getWidth(),chainWidth+240),std::max(60,viewport.getHeight()-12));
    int x=0;for(auto& card:cards){card->setBounds(x,0,card->preferredWidth(),content.getHeight());x+=card->preferredWidth()+8;}
}
void DeviceArea::paint(juce::Graphics& g)
{
    panel(g,getLocalBounds());text(g,"Device path / "+juce::String(model.selectedChannel().name),{16,7,getWidth()-245,27},15,design::colour::text,true);
    const int x=16+chainWidth-viewport.getViewPositionX();
    const bool source=model.selectedChannel().type=="MIDI"&&std::none_of(model.selectedChannel().devices.begin(),model.selectedChannel().devices.end(),[](const auto& d){return d.instrument();});
    if(x<getWidth())text(g,source?"Drop any instrument or Sample here":"Drop Audio Effect Here",{std::max(16,x),55,std::min(getWidth()-std::max(16,x)-12,240),getHeight()-70},12,design::colour::muted,true,juce::Justification::centred);
}
bool DeviceArea::isInterestedInDragSource(const SourceDetails& d){return d.description.toString().startsWith("device:")||d.description.toString().startsWith("plugin:")||juce::File(d.description.toString()).hasFileExtension("wav;aif;aiff;flac");}
void DeviceArea::itemDropped(const SourceDetails& d){if(onDrop)onDrop(d.description.toString(),model.selectedId());}
}



