#pragma once
#include <JuceHeader.h>
#include "FourKnobEQ.h"
#include "OptoCompressor.h"
#include "FETCompressor.h"
#include "PultecEQ.h"
#include "Limiter.h"
#include "Saturation.h"
class EQ5AudioProcessor : public juce::AudioProcessor
{
public:
    EQ5AudioProcessor();
    ~EQ5AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    FourKnobEQ eqLeft;
    FourKnobEQ eqRight;

        OptoCompressor compLeft;
        OptoCompressor compRight;

        FETCompressor fetCompLeft;
        FETCompressor fetCompRight;

        PultecEQ pultecLeft;
        PultecEQ pultecRight;

        Limiter limiterLeft;
        Limiter limiterRight;

        Saturation saturationLeft;
        Saturation saturationRight;

        // Matches the limiter's lookahead delay so the dry signal used for the
        // Mix blend stays time-aligned with the wet path — otherwise the two
        // are offset by ~5ms and comb-filter (phase) against each other.
        std::vector<float> dryDelayLeft, dryDelayRight;
        int dryDelayWriteIndex = 0;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQ5AudioProcessor)
};
