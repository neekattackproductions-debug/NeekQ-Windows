
#pragma once
#include <cmath>

class Biquad
{
public:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

    float processSample(float input)
    {
        float output = (b0 * input) + (b1 * x1) + (b2 * x2) - (a1 * y1) - (a2 * y2);
        x2 = x1; x1 = input;
        y2 = y1; y1 = output;
        return output;
    }

    void setLowShelf(float sampleRate, float frequency, float gainDB, float Q)
    {
        float A = std::pow(10.0f, gainDB / 40.0f);
        float omega = 2.0f * M_PI * frequency / sampleRate;
        float sn = std::sin(omega);
        float cs = std::cos(omega);
        float beta = std::sqrt(A) / Q;

        float a0_temp = (A + 1.0f) + (A - 1.0f) * cs + beta * sn;

        b0 = (A * ((A + 1.0f) - (A - 1.0f) * cs + beta * sn)) / a0_temp;
        b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs)) / a0_temp;
        b2 = (A * ((A + 1.0f) - (A - 1.0f) * cs - beta * sn)) / a0_temp;
        a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cs)) / a0_temp;
        a2 = ((A + 1.0f) + (A - 1.0f) * cs - beta * sn) / a0_temp;
    }

    void setHighShelf(float sampleRate, float frequency, float gainDB, float Q)
    {
        float A = std::pow(10.0f, gainDB / 40.0f);
        float omega = 2.0f * M_PI * frequency / sampleRate;
        float sn = std::sin(omega);
        float cs = std::cos(omega);
        float beta = std::sqrt(A) / Q;

        float a0_temp = (A + 1.0f) - (A - 1.0f) * cs + beta * sn;

        b0 = (A * ((A + 1.0f) + (A - 1.0f) * cs + beta * sn)) / a0_temp;
        b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs)) / a0_temp;
        b2 = (A * ((A + 1.0f) + (A - 1.0f) * cs - beta * sn)) / a0_temp;
        a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cs)) / a0_temp;
        a2 = ((A + 1.0f) - (A - 1.0f) * cs - beta * sn) / a0_temp;
    }

    void setPeak(float sampleRate, float frequency, float gainDB, float Q)
    {
        float A = std::pow(10.0f, gainDB / 40.0f);
        float omega = 2.0f * M_PI * frequency / sampleRate;
        float sn = std::sin(omega);
        float cs = std::cos(omega);
        float alpha = sn / (2.0f * Q);

        float a0_temp = 1.0f + alpha / A;

        b0 = (1.0f + alpha * A) / a0_temp;
        b1 = (-2.0f * cs) / a0_temp;
        b2 = (1.0f - alpha * A) / a0_temp;
        a1 = (-2.0f * cs) / a0_temp;
        a2 = (1.0f - alpha / A) / a0_temp;
    }

    void setHighPass(float sampleRate, float frequency, float Q)
    {
        float omega = 2.0f * M_PI * frequency / sampleRate;
        float sn = std::sin(omega);
        float cs = std::cos(omega);
        float alpha = sn / (2.0f * Q);

        float a0_temp = 1.0f + alpha;

        b0 = ((1.0f + cs) / 2.0f) / a0_temp;
        b1 = (-(1.0f + cs)) / a0_temp;
        b2 = ((1.0f + cs) / 2.0f) / a0_temp;
        a1 = (-2.0f * cs) / a0_temp;
        a2 = (1.0f - alpha) / a0_temp;
    }

    void setLowPass(float sampleRate, float frequency, float Q)
    {
        float omega = 2.0f * M_PI * frequency / sampleRate;
        float sn = std::sin(omega);
        float cs = std::cos(omega);
        float alpha = sn / (2.0f * Q);

        float a0_temp = 1.0f + alpha;

        b0 = ((1.0f - cs) / 2.0f) / a0_temp;
        b1 = (1.0f - cs) / a0_temp;
        b2 = ((1.0f - cs) / 2.0f) / a0_temp;
        a1 = (-2.0f * cs) / a0_temp;
        a2 = (1.0f - alpha) / a0_temp;
    }
};

class ThreeBandEQ
{
public:
    Biquad lowShelf;
    Biquad peak;
    Biquad highShelf;
    
    float currentSampleRate = 44100.0f;

    void setup(float sampleRate)
    {
        currentSampleRate = sampleRate;
        updateParameters(0.0f, 1000.0f, 0.0f, 0.0f);
    }

    void updateParameters(float lowGainDB, float midFreq, float midGainDB, float highGainDB)
    {
        lowShelf.setLowShelf(currentSampleRate, 200.0f, lowGainDB, 0.707f);
        peak.setPeak(currentSampleRate, midFreq, midGainDB, 1.0f);
        highShelf.setHighShelf(currentSampleRate, 8000.0f, highGainDB, 0.707f);
    }

    float processSample(float input)
    {
        float afterLow  = lowShelf.processSample(input);
        float afterPeak = peak.processSample(afterLow);
        float afterHigh = highShelf.processSample(afterPeak);
        return afterHigh;
    }
};
