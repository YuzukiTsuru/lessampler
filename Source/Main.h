#pragma once

#include "Dialogs.h"
#include "LangPack.h"
#include "../JuceLibraryCode/JuceHeader.h"


typedef struct {
    int offset_ms;
    int cutoff_ms;
    double *f0;
    double *FixedF0;
    double *TimeAxis;
    double *y;
    double *x;
    double **Specgram;
    int NBit;
    int *SpecgramLength;
    int *SpecgramIndex;
    int *FixedSpecgramIndex;
    int PCount;
    double FramePeriod;
    int x_length;
    int y_length;
    int f0_length;
    int f0_length2;
    double GRatio;
    double *Volume;
    double *FixedVolume;
    int fs;
    double Velocity;
    double VRatio;
    int LengthMsec;
    int Sf0LengthMsec;
    int InputF0LengthMsec;
    double f0_floor;
    int DefaultF0;
} LESS;


/*
 * init Dialogs Object
 */
Dialogs dialog;

/*
 * Init Language Pack
 */
LangPack langpack;

