#pragma once
#include "MixerControls.h"
#include "Theme.h"
#include "EditorKeys.h"
#include "model/MidiProject.h"

namespace auralis
{
class AutomationLaneView final : public juce::Component, public HelpProvider
{
public:
    AutomationLaneView(MixerState& state,MidiProject& project,int channel,int lane)
        : model(state),document(project),channelId(channel),laneId(lane)
    {
        setWantsKeyboardFocus(true);
        name.setEditable(false,true,false);name.setFont(juce::Font(juce::FontOptions(11)));
        name.onTextChange=[this]{if(auto* l=get()){l->name=name.getText().trim().substring(0,128).toStdString();model.notifyDevices();}};
        configureButton(enabled,"On","Enable or bypass this automation lane. Bypass restores the channel/device's manual value.",true);
        enabled.onClick=[this]{if(auto* l=get()){l->enabled=enabled.getToggleState();model.notifyDevices();}};
        setHelp(name,"Automation lane name","Double-click to rename this lane. It still controls the same device and parameter.");
        addAndMakeVisible(name);addAndMakeVisible(enabled);refresh();
    }
    int channelId,laneId,headerWidth=230,barWidth=100;
    double offsetBars=0;
    AutomationLane* get()
    {if(auto* c=model.find(channelId))for(auto& lane:c->automation)if(lane.id==laneId)return &lane;return nullptr;}
    void refresh()
    {if(auto* lane=get()){if(!name.isBeingEdited())name.setText(lane->name,juce::dontSendNotification);enabled.setToggleState(lane->enabled,juce::dontSendNotification);}repaint();}
    void resized() override
    {enabled.setBounds(18,9,37,23);name.setBounds(61,5,std::max(20,headerWidth-69),38);}
    HelpContent helpAt(juce::Point<int>,bool=false) const override
    {return {"Automation curve","Click empty space to add a point; drag points to move them. Right-click a point to delete it. Alt-drag a segment to bend its curve. Delete removes the selected point. Ctrl+A selects all points; Ctrl+C/X/V copy/cut/paste at the transport cursor, with normalized values across lanes. Ctrl+Z / Ctrl+Y undo/redo this lane edit. Double-click the lane name to rename. On bypasses/restores this lane. Shift+A hides or shows all child lanes."};}
    void paint(juce::Graphics& g) override
    {
        auto* lane=get();if(!lane)return;
        g.fillAll(colour(design::colour::background));
        g.setColour(colour(design::colour::line));g.drawHorizontalLine(getHeight()-1,0,static_cast<float>(getWidth()));
        g.drawLine(8,0,8,static_cast<float>(getHeight()),2);g.drawLine(8,20,17,20,2);
        text(g,juce::String(model.find(channelId)->name)+" / AUTOMATION",{20,49,headerWidth-28,16},9,design::colour::muted,true);
        const auto area=plot();juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(area);
        for(int b=static_cast<int>(offsetBars);b<=offsetBars+static_cast<double>(area.getWidth())/barWidth+1;++b)
        {g.setColour(colour(design::colour::line));g.drawVerticalLine(xAt(static_cast<Tick>(b)*document.barTicks),static_cast<float>(area.getY()),static_cast<float>(area.getBottom()));}
        juce::Path curve;
        for(int x=area.getX();x<area.getRight();++x)
        {
            const double time=(offsetBars+static_cast<double>(x-area.getX())/barWidth)*document.barTicks;
            const float y=static_cast<float>(area.getBottom()-lane->normalize(lane->valueAt(time))*area.getHeight());
            if(x==area.getX())curve.startNewSubPath(static_cast<float>(x),y);else curve.lineTo(static_cast<float>(x),y);
        }
        g.setColour(colour(lane->enabled?design::colour::coral:design::colour::muted));g.strokePath(curve,juce::PathStrokeType(1.8f));
        for(size_t i=0;i<lane->points.size();++i)
        {const auto p=point(lane->points[i]);g.fillEllipse(p.x-4,p.y-4,8,8);if((allSelected||static_cast<int>(i)==selected)){g.setColour(colour(design::colour::text));g.drawEllipse(p.x-6,p.y-6,12,12,1);}}
        if(selected>=0&&selected<static_cast<int>(lane->points.size()))
            text(g,juce::String(lane->denormalize(lane->points[selected].value),3),area.reduced(5).withHeight(18),10,design::colour::text);
    }
    void mouseDown(const juce::MouseEvent& event) override
    {
        if(!plot().contains(event.getPosition()))return;
        auto* lane=get();if(!lane)return;grabKeyboardFocus();selected=-1;allSelected=false;curving=false;
        for(size_t i=0;i<lane->points.size();++i)if(point(lane->points[i]).getDistanceFrom(event.position)<9){selected=static_cast<int>(i);break;}
        checkpoint(*lane);
        if(event.mods.isRightButtonDown())
        {if(selected>=0){lane->points.erase(lane->points.begin()+selected);selected=-1;model.notifyDevices();}return;}
        if(event.mods.isAltDown()&&selected<0)
        {
            const auto time=timeAt(event.x);for(size_t i=0;i+1<lane->points.size();++i)if(time>=lane->points[i].time&&time<lane->points[i+1].time){selected=static_cast<int>(i);curving=true;curveStart=lane->points[i].curve;anchorY=event.y;break;}
        }
        else if(selected<0)
        {
            const auto time=timeAt(event.x);lane->setPoint(time,lane->denormalize(valueAt(event.y)));
            for(size_t i=0;i<lane->points.size();++i)if(lane->points[i].time==time)selected=static_cast<int>(i);
        }
        model.notifyDevices();
    }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        auto* lane=get();if(!lane||selected<0||selected>=static_cast<int>(lane->points.size())||event.mods.isRightButtonDown())return;
        auto& p=lane->points[selected];
        if(curving)p.curve=std::clamp(curveStart+(event.y-anchorY)/100.0,-1.0,1.0);
        else
        {
            const Tick low=selected==0?0:lane->points[selected-1].time+1;
            const Tick high=selected+1==static_cast<int>(lane->points.size())?editing::maximumTime:lane->points[selected+1].time-1;
            p.time=std::clamp(timeAt(event.x),low,high);p.value=valueAt(event.y);
        }
        model.notifyDevices();
    }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const EditorKey input(key);
        auto* lane=get();if(!lane)return false;
        if(input.control)
        {
            const auto code=input.code;
            if(code=='A'){allSelected=true;repaint();return true;}
            if(code=='C'||code=='X')
            {
                clipboard.clear();if(allSelected)clipboard=lane->points;else if(selected>=0&&selected<static_cast<int>(lane->points.size()))clipboard.push_back(lane->points[selected]);
                if(code=='X'&&!clipboard.empty()){checkpoint(*lane);if(allSelected)lane->points.clear();else lane->points.erase(lane->points.begin()+selected);selected=-1;allSelected=false;model.notifyDevices();}return true;
            }
            if(code=='V'&&!clipboard.empty())
            {
                checkpoint(*lane);const auto origin=clipboard.front().time;
                for(const auto& p:clipboard)
                {
                    const auto time=std::clamp<editing::Tick>(model.automationTime+p.time-origin,0,editing::maximumTime);
                    lane->setPoint(time,lane->denormalize(p.value));
                    for(auto& added:lane->points)if(added.time==time){added.curve=p.curve;break;}
                }
                model.notifyDevices();return true;
            }
        }
        if(input.code==juce::KeyPress::deleteKey&&allSelected)
        {checkpoint(*lane);lane->points.clear();allSelected=false;selected=-1;model.notifyDevices();return true;}
        if(input.control&&(input.code=='Z' ||input.code=='Y'))
        {
            auto& from=input.code=='Z'?undo:redo;auto& to=input.code=='Z'?redo:undo;
            if(!from.empty()){to.push_back(lane->points);lane->points=std::move(from.back());from.pop_back();selected=-1;model.notifyDevices();}return true;
        }
        if(input.code==juce::KeyPress::deleteKey&&selected>=0&&selected<static_cast<int>(lane->points.size()))
        {checkpoint(*lane);lane->points.erase(lane->points.begin()+selected);selected=-1;model.notifyDevices();return true;}
        return false;
    }
private:
    MixerState& model;MidiProject& document;
    juce::Label name;juce::TextButton enabled;
    int selected=-1,anchorY=0;bool curving=false,allSelected=false;double curveStart=0;
    std::vector<std::vector<AutomationPoint>> undo,redo;
    inline static std::vector<AutomationPoint> clipboard;
    void checkpoint(const AutomationLane& lane){undo.push_back(lane.points);if(undo.size()>100)undo.erase(undo.begin());redo.clear();}
    juce::Rectangle<int> plot() const{return {headerWidth+design::panelGap,6,std::max(1,getWidth()-headerWidth-design::panelGap),getHeight()-13};}
    int xAt(Tick time) const{return plot().getX()+juce::roundToInt((static_cast<double>(time)/document.barTicks-offsetBars)*barWidth);}
    Tick timeAt(int x) const{return document.snap(static_cast<Tick>((offsetBars+static_cast<double>(x-plot().getX())/barWidth)*document.barTicks));}
    double valueAt(int y) const{return std::clamp(static_cast<double>(plot().getBottom()-y)/plot().getHeight(),0.0,1.0);}
    juce::Point<float> point(const AutomationPoint& p) const{return {static_cast<float>(xAt(p.time)),static_cast<float>(plot().getBottom()-p.value*plot().getHeight())};}
};
}
