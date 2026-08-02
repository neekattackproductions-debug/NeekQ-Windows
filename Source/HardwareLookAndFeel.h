#pragma once
#include <JuceHeader.h>
#include "AkiraExpandedFont.h"

// Renders controls to look like a physical hardware channel strip: small dark
// matte knob bodies with a glowing accent-coloured arc marking the value
// (plus a soft bloom behind it) and a pointer line inside the body. The arc's
// colour comes from rotarySliderFillColourId, so it still turns lime green
// when a control is moved away from zero, per the existing behaviour.
class HardwareLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Governs each slider's numeric readout (the knob captions themselves are
    // set directly via Label::setFont() in PluginEditor.cpp, not through here).
    // Kept as a clean system sans so the numbers stay easy to read, separate
    // from the display face used for the knob captions — except sliders marked
    // with "useHackFont" (see createSliderTextBox below), which read in Hack.
    juce::Font getLabelFont (juce::Label& label) override
    {
        if ((bool) label.getProperties().getWithDefault ("useHackFont", false))
            return juce::Font (juce::FontOptions ("Hack", 17.0f, juce::Font::bold));

        return juce::Font (juce::FontOptions ("Helvetica Neue", 16.0f, juce::Font::bold));
    }

    // The slider's value-readout Label is created internally by JUCE, so the only
    // way to tag it per-slider is here, where the owning Slider is available —
    // copy its "useHackFont" property onto the Label createSliderTextBox() builds.
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox (slider);
        l->getProperties().set ("useHackFont", slider.getProperties().getWithDefault ("useHackFont", false));
        return l;
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return juce::Font (juce::FontOptions (getAkiraExpandedTypeface())).withHeight ((float) juce::jmin (16, buttonHeight - 4));
    }

    // JUCE's default drawLabel() always paints with getLabelFont(label), completely
    // ignoring whatever was passed to Label::setFont() — which silently defeated the
    // caption labels' explicit Akira Expanded font. Labels marked with the
    // "useCaptionFont" property (set on the knob/section captions in PluginEditor.cpp)
    // paint with their own explicitly-set font instead; everything else (e.g. each
    // slider's own internal value-readout label) keeps using getLabelFont() as before.
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll (label.findColour (juce::Label::backgroundColourId));

        if (! label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            auto font = (bool) label.getProperties().getWithDefault ("useCaptionFont", false)
                          ? label.getFont()
                          : getLabelFont (label);

            g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
            g.setFont (font);

            auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
            g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                              juce::jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())),
                              label.getMinimumHorizontalScale());
        }

        g.setColour (label.findColour (juce::Label::outlineColourId).withMultipliedAlpha (label.isEnabled() ? 1.0f : 0.5f));
        g.drawRect (label.getLocalBounds());
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        bool bigKnob = (bool) slider.getProperties().getWithDefault ("bigKnob", false);
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (bigKnob ? 3.0f : 10.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        bool greenWhenPositive = (bool) slider.getProperties().getWithDefault ("greenWhenPositive", false);
        auto accentColour = (greenWhenPositive && slider.getValue() > 0.0)
                               ? juce::Colours::limegreen
                               : slider.findColour (juce::Slider::rotarySliderFillColourId, true);

        // decorative dim tick dots ringing the knob
        for (int i = 0; i < 20; ++i)
        {
            float a = rotaryStartAngle + (i / 19.0f) * (rotaryEndAngle - rotaryStartAngle);
            auto p = centre.getPointOnCircumference (radius * 1.18f, a);
            g.setColour (juce::Colour (0xff3a3a3e));
            g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre (p));
        }

        // knob body — chrome (Round/Punch/Juice) and red (Output) get the glossy
        // hardware-dome treatment (bezel ring, radial sheen, specular highlight,
        // dark rim); every other knob stays the original plain flat matte body.
        auto bodyRadius = radius * (bigKnob ? 0.76f : 0.7f);
        auto bodyRect = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);
        bool metalBody = (bool) slider.getProperties().getWithDefault ("metalBody", false);
        bool redBody   = (bool) slider.getProperties().getWithDefault ("redBody", false);

        if (redBody)
        {
            // flat solid red — no gradient, no gloss
            g.setColour (juce::Colour (0xff0a0a0c));
            g.fillEllipse (bodyRect.expanded (bodyRadius * 0.12f));
            g.setColour (juce::Colour (0xffcc1a1a));
            g.fillEllipse (bodyRect);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.drawEllipse (bodyRect.reduced (0.5f), bodyRadius * 0.045f);
        }
        else if (metalBody)
        {
            // dark bezel collar behind the dome
            g.setColour (juce::Colour (0xff0a0a0c));
            g.fillEllipse (bodyRect.expanded (bodyRadius * 0.12f));

            auto highlightPos = centre.translated (-bodyRadius * 0.28f, -bodyRadius * 0.38f);

            juce::Colour domeHighlight = juce::Colours::white;
            juce::Colour domeMid       = juce::Colour (0xffbfbfc4);
            juce::Colour domeShadow    = juce::Colour (0xff6a6a70);

            juce::ColourGradient domeGradient (domeHighlight, highlightPos.x, highlightPos.y,
                                               domeShadow, centre.x, centre.y + bodyRadius, false);
            domeGradient.addColour (0.4, domeMid);
            g.setGradientFill (domeGradient);
            g.fillEllipse (bodyRect);

            // specular highlight blob, offset toward the upper-left like a glossy sphere
            auto highlightBounds = juce::Rectangle<float> (bodyRadius * 0.85f, bodyRadius * 0.5f).withCentre (highlightPos);
            juce::ColourGradient specGradient (juce::Colours::white.withAlpha (0.75f),
                                               highlightBounds.getCentreX(), highlightBounds.getCentreY(),
                                               juce::Colours::white.withAlpha (0.0f),
                                               highlightBounds.getRight(), highlightBounds.getBottom(), true);
            g.setGradientFill (specGradient);
            g.fillEllipse (highlightBounds);

            // dark rim for depth
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.drawEllipse (bodyRect.reduced (0.5f), bodyRadius * 0.045f);
        }
        else
        {
            juce::ColourGradient bodyGradient (juce::Colour (0xff3c3c40), centre.x, centre.y,
                                               juce::Colour (0xff18181a), centre.x, centre.y + bodyRadius, false);
            g.setGradientFill (bodyGradient);
            g.fillEllipse (bodyRect);
        }

        // glowing value arc, with a soft bloom layered behind the crisp line —
        // some sliders (e.g. Frequency) opt out entirely via the "hideArc" property
        if (! (bool) slider.getProperties().getWithDefault ("hideArc", false))
        {
            auto arcRadius = radius * 0.92f;
            float lineW = juce::jmax (2.5f, radius * 0.08f);

            juce::Path backgroundArc;
            backgroundArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (juce::Colour (0xff2a2a2e));
            g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // For bipolar ranges (e.g. -18dB to +18dB) the arc should fill from the zero/rest
            // position, not from the range minimum — otherwise a small negative value looks
            // like a huge change instead of a small one.
            float fillFromAngle = rotaryStartAngle;
            double rangeMin = slider.getMinimum();
            double rangeMax = slider.getMaximum();
            if (rangeMin < 0.0 && rangeMax > 0.0)
            {
                float zeroFraction = (float) ((0.0 - rangeMin) / (rangeMax - rangeMin));
                fillFromAngle = rotaryStartAngle + zeroFraction * (rotaryEndAngle - rotaryStartAngle);
            }

            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, fillFromAngle, toAngle, true);

            for (int i = 3; i >= 1; --i)
            {
                g.setColour (accentColour.withAlpha (0.10f * i));
                g.strokePath (valueArc, juce::PathStrokeType (lineW + i * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setColour (accentColour);
            g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // pointer line, inside the knob body
        juce::Path pointer;
        auto pointerStart = centre.getPointOnCircumference (bodyRadius * 0.50f, toAngle);
        auto pointerEnd    = centre.getPointOnCircumference (bodyRadius * 0.91f, toAngle);
        pointer.startNewSubPath (pointerStart);
        pointer.lineTo (pointerEnd);
        g.setColour (metalBody ? juce::Colour (0xff2a2a2e) : juce::Colour (0xffe8e8ec));
        g.strokePath (pointer, juce::PathStrokeType (juce::jmax (1.5f, radius * 0.06f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (width * 0.42f, 4.0f);

        g.setColour (juce::Colour (0xff141416));
        g.fillRect (bounds);

        auto fillBounds = bounds.withTop (sliderPos);

        if (fillBounds.getHeight() > 0.0f)
        {
            g.setColour (juce::Colours::limegreen);
            g.fillRect (fillBounds);
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);

        g.setColour (juce::Colour (0xff141416));
        g.fillRoundedRectangle (bounds, 4.0f);

        auto litColour = button.getToggleState() ? button.findColour (juce::TextButton::buttonOnColourId)
                                                  : button.findColour (juce::TextButton::buttonColourId);

        // Scaled off the shorter side so wide/short buttons (like the saturation
        // row) don't collapse to a sliver — reducing by a fraction of width alone
        // pushed the inner rect's height negative on anything wider than it's tall.
        auto pad = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.16f;
        auto innerBounds = bounds.reduced (pad);
        g.setColour (litColour.withAlpha (0.30f));
        g.fillRoundedRectangle (innerBounds.expanded (3.0f), 4.0f);
        g.setColour (litColour);
        g.fillRoundedRectangle (innerBounds, 3.0f);

        g.setColour (juce::Colour (0xff5a5a5e));
        g.drawRoundedRectangle (bounds, 4.0f, 1.2f);
    }
};
