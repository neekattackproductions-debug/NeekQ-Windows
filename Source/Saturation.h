#pragma once
#include <cmath>
#include <algorithm>

// Five distinct waveshaping "flavors", each a nod to a different era/style of
// classic analog gear. Only one is active at a time. Each also takes a hidden
// "drive" amount (0-1, from the secret nail knob) that pushes its internal
// gain staging harder — the normalisation reference stays fixed to the mode's
// base drive, so turning drive up rides progressively further into the curve's
// clipped region instead of just getting louder.
class Saturation
{
public:
    enum Type
    {
        Off = 0,
        Tape,
        Tube,
        Console,
        Fuzz,
        Germanium
    };

    void setup (float sampleRate)
    {
        currentSampleRate = sampleRate;
        tapeLpfState = 0.0f;
    }

    float processSample (float input, int typeIndex, float driveAmount)
    {
        switch ((Type) typeIndex)
        {
            case Tape:      return processTape (input, driveAmount);
            case Tube:      return processTube (input, driveAmount);
            case Console:   return processConsole (input, driveAmount);
            case Fuzz:      return processFuzz (input, driveAmount);
            case Germanium: return processGermanium (input, driveAmount);
            case Off:
            default:        return input;
        }
    }

private:
    // Reel-to-reel style: warm, symmetric soft saturation with a gentle
    // high-frequency rolloff, like tape's natural self-EQ under drive.
    float processTape (float input, float driveAmount)
    {
        const float baseDrive = 2.2f;
        float drive = baseDrive * (1.0f + driveAmount * 4.0f);
        float shaped = std::tanh (input * drive) / std::tanh (baseDrive);

        const float lpfCoeff = 0.45f;
        tapeLpfState += lpfCoeff * (shaped - tapeLpfState);
        return tapeLpfState;
    }

    // Vintage tube preamp: asymmetric soft clipping — the positive and negative
    // halves saturate differently, generating the even-order harmonics tubes
    // are known for, with a touch of extra output on the softer side.
    float processTube (float input, float driveAmount)
    {
        const float baseDrive = 3.0f;
        float drive = baseDrive * (1.0f + driveAmount * 3.0f);
        float driven = input * drive;
        float shaped = driven >= 0.0f
                          ? std::tanh (driven)
                          : std::tanh (driven * 0.7f) * 1.15f;
        return shaped / 1.25f;
    }

    // Classic console summing bus: mostly transparent at low drive, a cubic
    // soft-knee that only really engages on hotter peaks — but pushed hard it
    // can get surprisingly aggressive for a "clean" console stage.
    float processConsole (float input, float driveAmount)
    {
        const float baseDrive = 1.4f;
        float drive = baseDrive * (1.0f + driveAmount * 5.0f);
        float driven = input * drive;
        float shaped = driven - (driven * driven * driven) / 6.0f;
        shaped = std::max (-1.5f, std::min (1.5f, shaped));
        return shaped / 1.3f;
    }

    // Vintage fuzz pedal: hard, aggressive clipping that "breaks up" readily,
    // rich in odd harmonics for a gritty, torn edge.
    float processFuzz (float input, float driveAmount)
    {
        const float baseDrive = 6.0f;
        float drive = baseDrive * (1.0f + driveAmount * 3.0f);
        float driven = input * drive;
        return driven / (1.0f + std::fabs (driven));
    }

    // Germanium transistor/diode-style clipper: an asymmetric hard-ish knee with
    // a distinct crunchy, slightly uneven character — grittier than tube, less
    // smooth than tape.
    float processGermanium (float input, float driveAmount)
    {
        const float baseDrive = 4.0f;
        float drive = baseDrive * (1.0f + driveAmount * 3.5f);
        float driven = input * drive;
        float shaped;
        if (driven >= 0.0f)
            shaped = 1.0f - std::exp (-driven);
        else
            shaped = -1.0f + std::exp (driven * 1.3f);
        return shaped * 0.9f;
    }

    float currentSampleRate = 44100.0f;
    float tapeLpfState = 0.0f;
};
