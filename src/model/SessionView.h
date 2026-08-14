#pragma once
#include "constants/Design.h"
#include "constants/Editing.h"
namespace auralis
{
struct SessionView
{
    bool automation=false;
    int browser=design::browserWidth,editor=design::devicesHeight;
    int arrangementBar=design::defaultBarWidth,trackHeight=design::trackHeight,header=design::trackWidth,vertical=0;
    double arrangementOffset=0;
    double pianoPixels=editing::pixelsPerBeat,pianoOffset=0,pianoPitch=48;
    int pianoRow=editing::defaultRowHeight,pianoControl=editing::defaultControlHeight;
};
}
