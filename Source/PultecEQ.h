#pragma once
#include "ThreeBandEQ.h"

class PultecEQ
{
public:
    Biquad lowShelf;
    Biquad highShelf;

    float currentSampleRate = 44100.0f;

    void setup(float sampleRate)
    {
        currentSampleRate = sampleRate;
        updateParameters(0.0f);
    }

    void updateParameters(float intensity)
    {
        float boostDB = 6.0f * intensity;
        lowShelf.setLowShelf(currentSampleRate, 100.0f, boostDB, 0.707f);
        highShelf.setHighShelf(currentSampleRate, 16000.0f, boostDB, 0.707f);
    }

    float processSample(float input)
    {
        float afterLow  = lowShelf.processSample(input);
        float afterHigh = highShelf.processSample(afterLow);
        return afterHigh;
    }
};
