#pragma once
#include <cmath>

class OptoCompressor
{
public:
    float sampleRate = 44100.0f;
    float envelope   = 0.0f;

    void setup(float sr)
    {
        sampleRate = sr;
        envelope   = 0.0f;
    }

    float processSample(float input, float intensity)
    {
        // intensity is 0.0 to 1.0 (from knob)

        // No compression at zero
        if (intensity <= 0.0f)
            return input;

        // Opto-style: threshold drops as intensity increases
        // At intensity 0 -> threshold = 0dB (no compression)
        // At intensity 1 -> threshold = -30dB (heavy compression)
        float thresholdDB  = -30.0f * intensity;
        float thresholdLin = std::pow(10.0f, thresholdDB / 20.0f);

        // Ratio increases gently with intensity (1:1 to 4:1)
        float ratio = 1.0f + (3.0f * intensity);

        // Opto attack/release — slow and program dependent
        float attackTime  = 0.030f;  // 30ms
        float releaseTime = 0.150f + (0.350f * (1.0f - intensity)); // 150-500ms

        float attackCoef  = std::exp(-1.0f / (sampleRate * attackTime));
        float releaseCoef = std::exp(-1.0f / (sampleRate * releaseTime));

        // Envelope follower
        float inputAbs = std::fabs(input);
        if (inputAbs > envelope)
            envelope = attackCoef  * envelope + (1.0f - attackCoef)  * inputAbs;
        else
            envelope = releaseCoef * envelope + (1.0f - releaseCoef) * inputAbs;

        // Gain computation
        float gainDB = 0.0f;
        if (envelope > thresholdLin)
        {
            float envelopeDB  = 20.0f * std::log10(envelope + 1e-6f);
            float overDB      = envelopeDB - thresholdDB;
            gainDB            = -(overDB * (1.0f - 1.0f / ratio));
        }

        float gain = std::pow(10.0f, gainDB / 20.0f);
        return input * gain;
    }
};
