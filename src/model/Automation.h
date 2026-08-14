#pragma once
#include "constants/Editing.h"
#include "constants/Project.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace auralis
{
struct AutomationPoint { editing::Tick time=0;double value=0,curve=0; };
struct AutomationLane
{
    int id=0,device=-1,parameter=0;
    std::string name;
    bool enabled=true,logarithmic=false;
    double minimum=0,maximum=1;
    std::vector<AutomationPoint> points;
    double normalize(double value) const
    {return std::clamp(logarithmic?std::log(std::max(minimum,value)/minimum)/std::log(maximum/minimum):(value-minimum)/(maximum-minimum),0.0,1.0);}
    double denormalize(double value) const
    {return logarithmic?minimum*std::pow(maximum/minimum,value):minimum+value*(maximum-minimum);}
    double valueAt(double time) const
    {
        if(points.empty())return minimum;
        auto right=std::upper_bound(points.begin(),points.end(),time,[](double t,const auto& p){return t<p.time;});
        if(right==points.begin())return denormalize(right->value);
        if(right==points.end())return denormalize(points.back().value);
        const auto& left=*(right-1);double t=(time-left.time)/(right->time-left.time);
        t=std::pow(std::clamp(t,0.0,1.0),std::pow(2.0,left.curve*4));
        return denormalize(left.value+(right->value-left.value)*t);
    }
    void setPoint(editing::Tick time,double value)
    {
        auto found=std::lower_bound(points.begin(),points.end(),time,[](const auto& p,auto t){return p.time<t;});
        if(found!=points.end()&&found->time==time)found->value=normalize(value);
        else if(points.size()<project::maximumAutomationPoints)points.insert(found,{time,normalize(value),0});
    }
};
}
