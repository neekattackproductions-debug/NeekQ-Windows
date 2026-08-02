#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "HardwareLookAndFeel.h"
#include "FaceMixLookAndFeel.h"
#include "LogoData.h"

// Paints nothing at all — used for the hidden drive knob so it stays
// invisible, found only by feel over the logo's top-right screw.
class InvisibleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override {}
};

// A plain empty circle, filled with buttonOnColourId when toggled on —
// used for the per-stage bypass dots so they stay out of the way visually.
class CircleBypassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        bool on = button.getToggleState();

        auto litColour = button.findColour (juce::TextButton::buttonOnColourId);
        auto outlineColour = button.findColour (juce::TextButton::buttonColourId);

        if (on)
        {
            g.setColour (litColour);
            g.fillEllipse (bounds);
        }

        g.setColour (on ? litColour : outlineColour);
        g.drawEllipse (bounds, 1.4f);
    }
};

// The actual hidden hit-target sat over the logo's screw. Stays invisible
// and never moves — fires callbacks on press/release so a separate visible
// knob can be shown alongside it without disturbing this one's drag tracking.
class NailKnob : public juce::Slider
{
public:
    std::function<void()> onPress;
    std::function<void()> onRelease;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (onPress != nullptr)
            onPress();
        juce::Slider::mouseDown (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        juce::Slider::mouseUp (e);
        if (onRelease != nullptr)
            onRelease();
    }

    // Drawn to look like a 5th nameplate screw — camouflage, not a real knob.
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        float dotSize = juce::jmin (b.getWidth(), b.getHeight()) * 0.42f;
        auto dot = juce::Rectangle<float> (dotSize, dotSize).withCentre (b.getCentre());

        juce::ColourGradient grad (juce::Colour (0xff6a6a6e), dot.getTopLeft(),
                                    juce::Colour (0xff1a1a1c), dot.getBottomRight(), true);
        g.setGradientFill (grad);
        g.fillEllipse (dot);

        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (dot, 0.6f);
    }
};

class EQ5AudioProcessorEditor : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    EQ5AudioProcessorEditor (EQ5AudioProcessor&);
    ~EQ5AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateSaturationButtonStates();
    EQ5AudioProcessor& audioProcessor;

    HardwareLookAndFeel hardwareLookAndFeel;
    FaceMixLookAndFeel faceMixLookAndFeel;
    InvisibleLookAndFeel invisibleLookAndFeel;
    CircleBypassLookAndFeel circleBypassLookAndFeel;

    juce::ImageComponent logoImageComponent;
    NailKnob nailKnob;
    juce::Slider nailKnobDisplay;
    juce::Label eqSectionLabel;
    juce::Label compSectionLabel;

    juce::Slider mixSlider;
    juce::Label mixLabel;

    juce::Slider lowCutSlider;
    juce::Slider midFreqSlider;
    juce::Slider midGainSlider;
    juce::Slider hiCutSlider;

    juce::Slider compressionSlider;
    juce::Slider fetCompressionSlider;
    juce::Slider makeupGainSlider;
    juce::Slider pultecLimitSlider;

    juce::TextButton bypassButton;
    juce::TextButton twoXButton;

    juce::TextButton roundBypassButton;
    juce::TextButton punchBypassButton;
    juce::TextButton juiceBypassButton;

    // Vintage-gear-flavoured saturation types — mutually exclusive, driven
    // directly off the "saturationType" choice parameter rather than via a
    // ButtonAttachment (JUCE has no built-in binding for a button group to a
    // single choice param). timerCallback() keeps their toggle states synced
    // to the parameter, so preset recall/automation still reflects correctly.
    juce::TextButton satTapeButton;
    juce::TextButton satTubeButton;
    juce::TextButton satConsoleButton;
    juce::TextButton satFuzzButton;
    juce::TextButton satGermaniumButton;
    juce::AudioParameterChoice* saturationParam = nullptr;

    juce::Label lowCutLabel;
    juce::Label midFreqLabel;
    juce::Label midGainLabel;
    juce::Label hiCutLabel;

    juce::Label compressionLabel;
    juce::Label fetCompressionLabel;
    juce::Label makeupGainLabel;
    juce::Label pultecLimitLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment nailKnobAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment lowCutAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment midFreqAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment midGainAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment hiCutAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment compressionAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment fetCompressionAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment makeupGainAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment pultecLimitAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment twoXAttachment;

    juce::AudioProcessorValueTreeState::ButtonAttachment roundBypassAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment punchBypassAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment juiceBypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQ5AudioProcessorEditor)
};
