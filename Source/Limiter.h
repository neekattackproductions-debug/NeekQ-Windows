#pragma once
#include <cmath>
#include <vector>
#include <algorithm>

class Limiter
{
public:
    void setup(float sampleRate)
    {
        currentSampleRate = sampleRate;

        lookaheadSamples = (int) (currentSampleRate * 0.005f); // 5ms lookahead
        if (lookaheadSamples < 1)
            lookaheadSamples = 1;

        delayBuffer.assign ((size_t) lookaheadSamples, 0.0f);
        writeIndex = 0;
        gain = 1.0f;
    }

    float processSample(float input, float intensity)
    {
        // Ceiling is fixed, but the drive scales hard with intensity — turning the
        // knob up shoves the signal further and further into the brickwall, riding
        // it down into a dense, glued, always-on squeeze (R-Vox style vocal leveling)
        // instead of just occasionally catching a peak.
        const float thresholdDB  = -0.3f;
        const float thresholdLin = std::pow (10.0f, thresholdDB / 20.0f);
        const float driveDB      = 6.0f + 20.0f * intensity;
        const float driveLin     = std::pow (10.0f, driveDB / 20.0f);

        float driven = input * driveLin;

        float delayed = delayBuffer[(size_t) writeIndex];
        delayBuffer[(size_t) writeIndex] = driven;
        writeIndex = (writeIndex + 1) % lookaheadSamples;

        float peak = std::fabs (driven);
        for (auto s : delayBuffer)
            peak = std::max (peak, std::fabs (s));

        float targetGain = (peak > thresholdLin) ? (thresholdLin / peak) : 1.0f;

        if (targetGain < gain)
        {
            gain = targetGain; // instant attack — never let a peak through
        }
        else
        {
            float releaseCoef = std::exp (-1.0f / (currentSampleRate * 0.070f)); // 70ms release — tight and controlled
            gain = releaseCoef * gain + (1.0f - releaseCoef) * targetGain;
        }

        return delayed * gain;
    }

    int getLookaheadSamples() const { return lookaheadSamples; }

private:
    float currentSampleRate = 44100.0f;
    int lookaheadSamples = 1;
    std::vector<float> delayBuffer;
    int writeIndex = 0;
    float gain = 1.0f;
};
