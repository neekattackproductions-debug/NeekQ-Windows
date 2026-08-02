#pragma once
#include <vector>
#include <cmath>

class Flanger
{
public:
    // lfoPhaseOffset lets left/right start at different points in the drift
    // cycle, so the doubled voice decorrelates slightly between channels
    // instead of moving in lockstep.
    void setup (float sampleRate, float lfoPhaseOffset = 0.0f)
    {
        currentSampleRate = sampleRate;
        int maxDelaySamples = (int) (sampleRate * 0.04f) + 4;
        delayBuffer.assign ((size_t) maxDelaySamples, 0.0f);
        writeIndex = 0;
        lfoPhase = lfoPhaseOffset;
    }

    // intensity: 0.0 - 1.0. A short, near-static delay with only a hint of
    // pitch drift and no feedback — hugs the vocal like a doubler/chorus
    // rather than sweeping like a flanger.
    float processSample (float input, float intensity)
    {
        const float lfoRateHz   = 0.12f;
        const float baseDelayMs = 16.0f;
        const float depthMs     = intensity * 1.2f;
        const float wetAmount   = intensity * 0.55f;

        lfoPhase += lfoRateHz / currentSampleRate;
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;

        float lfo = 0.5f * (1.0f + std::sin (6.283185307f * lfoPhase));

        float delaySamples = ((baseDelayMs + depthMs * lfo) / 1000.0f) * currentSampleRate;

        int bufferSize = (int) delayBuffer.size();
        float readPos = (float) writeIndex - delaySamples;
        while (readPos < 0.0f)
            readPos += (float) bufferSize;

        int idx0 = (int) readPos % bufferSize;
        int idx1 = (idx0 + 1) % bufferSize;
        float frac = readPos - std::floor (readPos);

        float delayed = delayBuffer[(size_t) idx0] * (1.0f - frac) + delayBuffer[(size_t) idx1] * frac;

        delayBuffer[(size_t) writeIndex] = input;
        writeIndex = (writeIndex + 1) % bufferSize;

        return input * (1.0f - wetAmount) + delayed * wetAmount;
    }

private:
    float currentSampleRate = 44100.0f;
    std::vector<float> delayBuffer;
    int writeIndex = 0;
    float lfoPhase = 0.0f;
};
