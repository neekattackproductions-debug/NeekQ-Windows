#pragma once
#include "ThreeBandEQ.h"

class FourKnobEQ
{
public:
    Biquad lowShelf;
    Biquad midPeak;
    Biquad hiShelf;

    float currentSampleRate = 44100.0f;

    void setup(float sampleRate)
    {
        currentSampleRate = sampleRate;
        updateParameters(-3.0f, 200.0f, 0.0f, 3.0f);
    }

    void updateParameters(float lowGainDB, float midFreq, float midGainDB, float hiGainDB)
    {
        lowShelf.setLowShelf(currentSampleRate, 200.0f, lowGainDB, 0.707f);
        midPeak.setPeak(currentSampleRate, midFreq, midGainDB, 1.0f);
        hiShelf.setHighShelf(currentSampleRate, 8000.0f, hiGainDB, 0.707f);
    }

    float processSample(float input)
    {
        float afterLow = lowShelf.processSample(input);
        float afterMid = midPeak.processSample(afterLow);
        float afterHi  = hiShelf.processSample(afterMid);
        return afterHi;
    }
};
