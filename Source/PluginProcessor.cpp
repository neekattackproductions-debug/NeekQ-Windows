/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQ5AudioProcessor::EQ5AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}
EQ5AudioProcessor::~EQ5AudioProcessor()
{
}
juce::AudioProcessorValueTreeState::ParameterLayout EQ5AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> ("lowCut",  "Low Shelf", -18.0f, 18.0f, -3.0f));

    // Skewed so 2500Hz sits exactly at the knob's centre (12 o'clock), which is also the default
    float midFreqSkew = std::log (0.5f) / std::log ((2500.0f - 200.0f) / (5000.0f - 200.0f));
    juce::NormalisableRange<float> midFreqRange (200.0f, 5000.0f, 1.0f, midFreqSkew);
    layout.add (std::make_unique<juce::AudioParameterFloat> ("midFreq", "Frequency", midFreqRange, 200.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> ("midGain", "Mid Gain",  -18.0f, 18.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("hiCut",   "Hi Shelf",  -18.0f, 18.0f, 3.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> ("compression", "Round", 0.0f, 1.0f, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("makeupGain", "Output", -24.0f, 24.0f, -12.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("fetCompression", "Punch", 0.0f, 1.0f, 0.4f));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("bypass", "Bypass", false));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("doubleIntensity", "2X", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("pultecLimit", "Juice", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", 0.0f, 1.0f, 1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> ("saturationDrive", "Saturation Drive", 0.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> ("roundBypass", "Round Bypass", false));
    layout.add (std::make_unique<juce::AudioParameterBool> ("punchBypass", "Punch Bypass", false));
    layout.add (std::make_unique<juce::AudioParameterBool> ("juiceBypass", "Juice Bypass", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> ("saturationType", "Saturation",
        juce::StringArray { "Off", "Tape", "Tube", "Console", "Fuzz", "Germanium" }, 0));

    return layout;
}

//==============================================================================
const juce::String EQ5AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EQ5AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQ5AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQ5AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQ5AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQ5AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EQ5AudioProcessor::getCurrentProgram()
{
    return 0;
}

void EQ5AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EQ5AudioProcessor::getProgramName (int index)
{
    return {};
}

void EQ5AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EQ5AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    eqLeft.setup((float) sampleRate);
        eqRight.setup((float) sampleRate);
    compLeft.setup((float) sampleRate);
            compRight.setup((float) sampleRate);
    fetCompLeft.setup((float) sampleRate);
            fetCompRight.setup((float) sampleRate);
    pultecLeft.setup((float) sampleRate);
            pultecRight.setup((float) sampleRate);
    limiterLeft.setup((float) sampleRate);
            limiterRight.setup((float) sampleRate);
    saturationLeft.setup((float) sampleRate);
            saturationRight.setup((float) sampleRate);

    int dryDelaySamples = limiterLeft.getLookaheadSamples();
    dryDelayLeft.assign ((size_t) dryDelaySamples, 0.0f);
    dryDelayRight.assign ((size_t) dryDelaySamples, 0.0f);
    dryDelayWriteIndex = 0;
}

void EQ5AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQ5AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EQ5AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (*apvts.getRawParameterValue ("bypass") > 0.5f)
        return;

        float lowCut  = *apvts.getRawParameterValue ("lowCut");
        float midFreq = *apvts.getRawParameterValue ("midFreq");
        float midGain = *apvts.getRawParameterValue ("midGain");
        float hiCut   = *apvts.getRawParameterValue ("hiCut");

    float compression = *apvts.getRawParameterValue ("compression");
    float fetCompression = *apvts.getRawParameterValue ("fetCompression");
    bool doubleFx = *apvts.getRawParameterValue ("doubleIntensity") > 0.5f;

    float makeupGainDB = *apvts.getRawParameterValue ("makeupGain");
    float makeupGain = std::pow (10.0f, makeupGainDB / 20.0f);
    float pultecLimit = *apvts.getRawParameterValue ("pultecLimit");
    float mix = *apvts.getRawParameterValue ("mix");

    bool roundBypass = *apvts.getRawParameterValue ("roundBypass") > 0.5f;
    bool punchBypass = *apvts.getRawParameterValue ("punchBypass") > 0.5f;
    bool juiceBypass = *apvts.getRawParameterValue ("juiceBypass") > 0.5f;

    int saturationType = (int) *apvts.getRawParameterValue ("saturationType");
    float saturationDrive = *apvts.getRawParameterValue ("saturationDrive");

    // Auto gain: compensate for the average level the Round/Punch compressors pull down,
    // so turning them up doesn't quietly drop the overall volume. Skipped for any stage
    // that's bypassed, since that stage isn't pulling the level down at all.
    float optoAutoGainDB = 0.0f;
    if (! roundBypass && compression > 0.0f)
    {
        float optoThresholdDB = -30.0f * compression;
        float optoRatio       = 1.0f + 3.0f * compression;
        optoAutoGainDB = -optoThresholdDB * (1.0f - 1.0f / optoRatio) * 0.5f;
    }

    float fetAutoGainDB = 0.0f;
    if (! punchBypass && fetCompression > 0.0f)
    {
        float fetThresholdDB = -24.0f * fetCompression;
        float fetRatio       = 2.0f + 10.0f * fetCompression;
        fetAutoGainDB = -fetThresholdDB * (1.0f - 1.0f / fetRatio) * 0.5f;
    }

    float autoGain = std::pow (10.0f, (optoAutoGainDB + fetAutoGainDB) / 20.0f);
    makeupGain *= autoGain;

        eqLeft.updateParameters  (lowCut, midFreq, midGain, hiCut);
        eqRight.updateParameters (lowCut, midFreq, midGain, hiCut);

        pultecLeft.updateParameters  (pultecLimit);
        pultecRight.updateParameters (pultecLimit);

        auto* leftChannel  = buffer.getWritePointer (0);
        auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    int dryDelaySize = (int) dryDelayLeft.size();

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                float leftDry = dryDelayLeft[(size_t) dryDelayWriteIndex];
                dryDelayLeft[(size_t) dryDelayWriteIndex] = leftChannel[sample];

                float leftEQ = eqLeft.processSample (leftChannel[sample]);
                float leftSaturated = saturationLeft.processSample (leftEQ, saturationType, saturationDrive);

                float leftComp = leftSaturated;
                if (! roundBypass)
                {
                    leftComp = compLeft.processSample (leftSaturated, compression);
                    if (doubleFx)
                        leftComp = compLeft.processSample (leftComp, compression);
                }

                float leftFet = leftComp;
                if (! punchBypass)
                {
                    leftFet = fetCompLeft.processSample (leftComp, fetCompression);
                    if (doubleFx)
                        leftFet = fetCompLeft.processSample (leftFet, fetCompression);
                }

                float leftMakeup = leftFet * makeupGain;

                float leftLimited = leftMakeup;
                if (! juiceBypass)
                {
                    float leftPultec = pultecLeft.processSample (leftMakeup);
                    if (doubleFx)
                        leftPultec = pultecLeft.processSample (leftPultec);

                    leftLimited = limiterLeft.processSample (leftPultec, pultecLimit);
                }

                leftChannel[sample] = leftDry * (1.0f - mix) + leftLimited * mix;

                if (rightChannel != nullptr)
                {
                    float rightDry = dryDelayRight[(size_t) dryDelayWriteIndex];
                    dryDelayRight[(size_t) dryDelayWriteIndex] = rightChannel[sample];

                    float rightEQ = eqRight.processSample (rightChannel[sample]);
                    float rightSaturated = saturationRight.processSample (rightEQ, saturationType, saturationDrive);

                    float rightComp = rightSaturated;
                    if (! roundBypass)
                    {
                        rightComp = compRight.processSample (rightSaturated, compression);
                        if (doubleFx)
                            rightComp = compRight.processSample (rightComp, compression);
                    }

                    float rightFet = rightComp;
                    if (! punchBypass)
                    {
                        rightFet = fetCompRight.processSample (rightComp, fetCompression);
                        if (doubleFx)
                            rightFet = fetCompRight.processSample (rightFet, fetCompression);
                    }

                    float rightMakeup = rightFet * makeupGain;

                    float rightLimited = rightMakeup;
                    if (! juiceBypass)
                    {
                        float rightPultec = pultecRight.processSample (rightMakeup);
                        if (doubleFx)
                            rightPultec = pultecRight.processSample (rightPultec);

                        rightLimited = limiterRight.processSample (rightPultec, pultecLimit);
                    }

                    rightChannel[sample] = rightDry * (1.0f - mix) + rightLimited * mix;
                }

                dryDelayWriteIndex = (dryDelayWriteIndex + 1) % dryDelaySize;
            }
    }

//==============================================================================
bool EQ5AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EQ5AudioProcessor::createEditor()
{
    return new EQ5AudioProcessorEditor (*this);
}

//==============================================================================
void EQ5AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void EQ5AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQ5AudioProcessor();
}
