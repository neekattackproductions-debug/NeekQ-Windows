#pragma once
#include <JuceHeader.h>
#include "FaceStencilData.h"

// Renders the "mix" slider as the Neek face stencil instead of a normal fader —
// at full mix the whole face is visible; pulling the fader down erases the face
// from the top downward (like a draining level), so the remaining visible area
// itself communicates the mix amount.
class FaceMixLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FaceMixLookAndFeel()
    {
        faceImage = juce::ImageFileFormat::loadFrom (faceStencilPNGData, (size_t) faceStencilPNGDataSize);
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        return juce::Font (juce::FontOptions ("Helvetica Neue", 16.0f, juce::Font::bold));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        juce::Rectangle<float> bounds ((float) x, (float) y, (float) width, (float) height);

        if (! faceImage.isValid())
            return;

        g.saveState();
        juce::Rectangle<int> visibleClip ((int) bounds.getX(), (int) sliderPos,
                                          (int) bounds.getWidth(), (int) (bounds.getBottom() - sliderPos));
        g.reduceClipRegion (visibleClip);
        g.drawImage (faceImage, bounds, juce::RectanglePlacement::centred, false);
        g.restoreState();
    }

private:
    juce::Image faceImage;
};
