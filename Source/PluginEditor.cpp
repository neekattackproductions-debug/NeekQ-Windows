#include "PluginProcessor.h"
#include "PluginEditor.h"

EQ5AudioProcessorEditor::EQ5AudioProcessorEditor (EQ5AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      mixAttachment            (p.apvts, "mix",            mixSlider),
      nailKnobAttachment       (p.apvts, "saturationDrive", nailKnob),
      lowCutAttachment         (p.apvts, "lowCut",         lowCutSlider),
      midFreqAttachment        (p.apvts, "midFreq",        midFreqSlider),
      midGainAttachment        (p.apvts, "midGain",        midGainSlider),
      hiCutAttachment          (p.apvts, "hiCut",          hiCutSlider),
      compressionAttachment    (p.apvts, "compression",    compressionSlider),
      fetCompressionAttachment (p.apvts, "fetCompression", fetCompressionSlider),
      makeupGainAttachment     (p.apvts, "makeupGain",     makeupGainSlider),
      pultecLimitAttachment    (p.apvts, "pultecLimit",   pultecLimitSlider),
      bypassAttachment         (p.apvts, "bypass",         bypassButton),
      twoXAttachment           (p.apvts, "doubleIntensity", twoXButton),
      roundBypassAttachment    (p.apvts, "roundBypass",    roundBypassButton),
      punchBypassAttachment    (p.apvts, "punchBypass",    punchBypassButton),
      juiceBypassAttachment    (p.apvts, "juiceBypass",    juiceBypassButton)
{
    setLookAndFeel (&hardwareLookAndFeel);

    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3a3c));
    setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xff4dd9e8));
    setColour (juce::Slider::backgroundColourId,          juce::Colour (0xff3a3a3c));
    setColour (juce::Slider::trackColourId,               juce::Colour (0xff4dd9e8));
    setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId,                 juce::Colour (0xffd8d8dc));

    // setTextBoxStyle() below snapshots textBoxOutlineColourId immediately (it builds the
    // value textbox right then) — at that point these sliders aren't parented to the editor
    // yet, so the transparent override above isn't visible via inheritance. Set it directly
    // on each slider so the snapshot picks it up regardless of ordering.
    for (auto* s : { &mixSlider, &nailKnobDisplay, &lowCutSlider, &midFreqSlider, &midGainSlider, &hiCutSlider,
                      &compressionSlider, &fetCompressionSlider, &makeupGainSlider, &pultecLimitSlider })
        s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    auto logoImage = juce::ImageFileFormat::loadFrom (logoPNGData, (size_t) logoPNGDataSize);
    logoImageComponent.setImage (logoImage);
    logoImageComponent.setImagePlacement (juce::RectanglePlacement::centred);
    addAndMakeVisible (logoImageComponent);

    eqSectionLabel.setText ("EQ", juce::dontSendNotification);
    eqSectionLabel.setJustificationType (juce::Justification::centredLeft);
    eqSectionLabel.setFont (juce::Font (juce::FontOptions ("Hack", 1.0f, juce::Font::bold)).withHeight (14.0f).withHorizontalScale (1.15f));
    eqSectionLabel.getProperties().set ("useCaptionFont", true);
    eqSectionLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb8a06a));
    addAndMakeVisible (eqSectionLabel);

    compSectionLabel.setText ("COMP", juce::dontSendNotification);
    compSectionLabel.setJustificationType (juce::Justification::centredLeft);
    compSectionLabel.setFont (juce::Font (juce::FontOptions ("Hack", 1.0f, juce::Font::bold)).withHeight (14.0f).withHorizontalScale (1.15f));
    compSectionLabel.getProperties().set ("useCaptionFont", true);
    compSectionLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb8a06a));
    addAndMakeVisible (compSectionLabel);

    auto usePercentDisplay = [] (juce::Slider& slider)
    {
        slider.textFromValueFunction = [] (double value)
        {
            return juce::String (juce::roundToInt (value * 100.0)) + "%";
        };
        slider.valueFromTextFunction = [] (const juce::String& text)
        {
            return text.retainCharacters ("0123456789.").getDoubleValue() / 100.0;
        };
        slider.updateText();
    };

    // Hidden hit-target sat on the logo's top-right screw — never moves,
    // never draws anything. Driving the actual parameter.
    nailKnob.setLookAndFeel (&invisibleLookAndFeel);
    nailKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    nailKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    nailKnob.setOpaque (false);
    addAndMakeVisible (nailKnob);

    // Visible "wheel" that pops up alongside it only while pressed/dragged.
    nailKnobDisplay.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    nailKnobDisplay.setLookAndFeel (&hardwareLookAndFeel);
    nailKnobDisplay.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 32, 12);
    nailKnobDisplay.setRange (0.0, 1.0);
    nailKnobDisplay.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffbb86fc));
    usePercentDisplay (nailKnobDisplay);
    nailKnobDisplay.setInterceptsMouseClicks (false, false);
    addChildComponent (nailKnobDisplay); // added hidden — addAndMakeVisible would force it shown

    nailKnob.onPress = [this]
    {
        nailKnobDisplay.setValue (nailKnob.getValue(), juce::dontSendNotification);
        nailKnobDisplay.setVisible (true);
        nailKnobDisplay.toFront (false);
    };
    nailKnob.onRelease = [this]
    {
        nailKnobDisplay.setVisible (false);
    };
    nailKnob.onValueChange = [this]
    {
        nailKnobDisplay.setValue (nailKnob.getValue(), juce::dontSendNotification);
    };

    mixSlider.setSliderStyle (juce::Slider::LinearVertical);
    mixSlider.setLookAndFeel (&faceMixLookAndFeel);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    usePercentDisplay (mixSlider);
    addAndMakeVisible (mixSlider);

    mixLabel.setText ("MIX", juce::dontSendNotification);
    mixLabel.setJustificationType (juce::Justification::centred);
    mixLabel.setFont (juce::Font (juce::FontOptions ("Hack", 1.0f, juce::Font::bold)).withHeight (14.0f).withHorizontalScale (1.15f));
    mixLabel.getProperties().set ("useCaptionFont", true);
    addAndMakeVisible (mixLabel);

    bypassButton.setButtonText ("ON");
    bypassButton.setClickingTogglesState (true);
    bypassButton.setWantsKeyboardFocus (false);
    bypassButton.setColour (juce::TextButton::buttonColourId,   juce::Colours::darkgreen);
    bypassButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::red);
    bypassButton.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
    bypassButton.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    bypassButton.onClick = [this]
    {
        bypassButton.setButtonText (bypassButton.getToggleState() ? "OFF" : "ON");
    };
    addAndMakeVisible (bypassButton);

    twoXButton.setButtonText ("2X");
    twoXButton.setClickingTogglesState (true);
    twoXButton.setWantsKeyboardFocus (false);
    twoXButton.setColour (juce::TextButton::buttonColourId,   juce::Colours::darkslategrey);
    twoXButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::orange);
    addAndMakeVisible (twoXButton);

    // Small per-stage mute toggles for Round/Punch/Juice — engaged (red) means
    // that stage is bypassed while the rest of the chain keeps processing.
    auto setupMiniBypass = [this] (juce::TextButton& button)
    {
        button.setButtonText ("");
        button.setClickingTogglesState (true);
        button.setWantsKeyboardFocus (false);
        button.setLookAndFeel (&circleBypassLookAndFeel);
        button.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff5a5a5e));
        button.setColour (juce::TextButton::buttonOnColourId, juce::Colours::red);
        addAndMakeVisible (button);
    };
    setupMiniBypass (roundBypassButton);
    setupMiniBypass (punchBypassButton);
    setupMiniBypass (juiceBypassButton);

    // Vintage-gear saturation flavors — mutually exclusive. Clicking the
    // active one again turns saturation off; clicking another switches to it.
    saturationParam = dynamic_cast<juce::AudioParameterChoice*> (audioProcessor.apvts.getParameter ("saturationType"));

    auto setupSatButton = [this] (juce::TextButton& button, const juce::String& text, int index, juce::Colour onColour)
    {
        button.setButtonText (text);
        button.setClickingTogglesState (false);
        button.setWantsKeyboardFocus (false);
        button.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff3a3a3c));
        button.setColour (juce::TextButton::buttonOnColourId, onColour);
        button.onClick = [this, index]
        {
            if (saturationParam == nullptr)
                return;
            int current = saturationParam->getIndex();
            *saturationParam = (current == index) ? 0 : index;
            updateSaturationButtonStates();
        };
        addAndMakeVisible (button);
    };
    setupSatButton (satTapeButton,      "TAPE",      1, juce::Colour (0xffd08a3c));
    setupSatButton (satTubeButton,      "TUBE",      2, juce::Colour (0xffff6a2a));
    setupSatButton (satConsoleButton,   "CONSOLE",   3, juce::Colours::orange);
    setupSatButton (satFuzzButton,      "FUZZ",      4, juce::Colour (0xffff2d95));
    setupSatButton (satGermaniumButton, "GERMANIUM", 5, juce::Colour (0xff2ad4c8));

    updateSaturationButtonStates();
    startTimerHz (15);


    lowCutSlider.setSliderStyle  (juce::Slider::RotaryVerticalDrag);
    midFreqSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    midGainSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    hiCutSlider.setSliderStyle   (juce::Slider::RotaryVerticalDrag);

    lowCutSlider.setTextBoxStyle  (juce::Slider::TextBoxBelow, false, 80, 20);
    midFreqSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    midGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    hiCutSlider.setTextBoxStyle   (juce::Slider::TextBoxBelow, false, 80, 20);

    lowCutSlider.setColour  (juce::Slider::rotarySliderFillColourId, juce::Colours::limegreen);
    midGainSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::limegreen);
    hiCutSlider.setColour   (juce::Slider::rotarySliderFillColourId, juce::Colours::limegreen);

    auto useDbDisplay = [] (juce::Slider& slider)
    {
        slider.textFromValueFunction = [] (double value)
        {
            return juce::String (value, 2) + " dB";
        };
        slider.valueFromTextFunction = [] (const juce::String& text)
        {
            return text.retainCharacters ("-0123456789.").getDoubleValue();
        };
        slider.updateText();
    };
    useDbDisplay (lowCutSlider);
    useDbDisplay (midGainSlider);
    useDbDisplay (hiCutSlider);
    midFreqSlider.getProperties().set ("hideArc", true);
    midFreqSlider.setNumDecimalPlacesToDisplay (0);
    midFreqSlider.textFromValueFunction = [] (double value)
    {
        if (value >= 1000.0)
            return juce::String (value / 1000.0, 2) + "kHz";
        return juce::String (juce::roundToInt (value)) + "Hz";
    };
    midFreqSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        bool isK = text.containsIgnoreCase ("k");
        double num = text.retainCharacters ("0123456789.").getDoubleValue();
        return isK ? num * 1000.0 : num;
    };
    midFreqSlider.updateText();

    lowCutLabel.setText  ("LO SHELF", juce::dontSendNotification);
    midFreqLabel.setText ("FREQUENCY", juce::dontSendNotification);
    midGainLabel.setText ("MID GAIN",  juce::dontSendNotification);
    hiCutLabel.setText   ("HI SHELF",  juce::dontSendNotification);

    lowCutLabel.setJustificationType  (juce::Justification::centred);
    midFreqLabel.setJustificationType (juce::Justification::centred);
    midGainLabel.setJustificationType (juce::Justification::centred);
    hiCutLabel.setJustificationType   (juce::Justification::centred);

    juce::Font boldLabelFont = juce::Font (juce::FontOptions ("Hack", 1.0f, juce::Font::bold)).withHeight (17.0f).withHorizontalScale (1.15f);
    juce::Font eqLabelFont   = boldLabelFont.withHeight (14.0f);
    lowCutLabel.setFont  (eqLabelFont);
    midFreqLabel.setFont (eqLabelFont);
    midGainLabel.setFont (eqLabelFont);
    hiCutLabel.setFont   (eqLabelFont);
    for (auto* l : { &lowCutLabel, &midFreqLabel, &midGainLabel, &hiCutLabel })
        l->getProperties().set ("useCaptionFont", true);

    addAndMakeVisible (lowCutSlider);
    addAndMakeVisible (midFreqSlider);
    addAndMakeVisible (midGainSlider);
    addAndMakeVisible (hiCutSlider);

    addAndMakeVisible (lowCutLabel);
    addAndMakeVisible (midFreqLabel);
    addAndMakeVisible (midGainLabel);
    addAndMakeVisible (hiCutLabel);

    compressionSlider.setSliderStyle    (juce::Slider::RotaryVerticalDrag);
    fetCompressionSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    makeupGainSlider.setSliderStyle     (juce::Slider::RotaryVerticalDrag);
    pultecLimitSlider.setSliderStyle    (juce::Slider::RotaryVerticalDrag);

    makeupGainSlider.getProperties().set ("useHackFont", true);
    // setTextBoxStyle() below creates the value textbox immediately via
    // getLookAndFeel() — if the slider isn't parented yet that resolves to
    // JUCE's default LookAndFeel instead of ours, and the "useHackFont" flag
    // never gets copied onto the label at all. Set explicitly to sidestep that.
    makeupGainSlider.setLookAndFeel (&hardwareLookAndFeel);

    compressionSlider.setTextBoxStyle    (juce::Slider::TextBoxBelow, false, 80, 20);
    fetCompressionSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    makeupGainSlider.setTextBoxStyle     (juce::Slider::TextBoxBelow, false, 130, 24);
    pultecLimitSlider.setTextBoxStyle    (juce::Slider::TextBoxBelow, false, 80, 20);

    compressionSlider.setColour    (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffeeff00));
    fetCompressionSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::dodgerblue);
    pultecLimitSlider.setColour    (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffc400ff));

    for (auto* s : { &compressionSlider, &fetCompressionSlider, &pultecLimitSlider })
        s->getProperties().set ("metalBody", true);

    usePercentDisplay (compressionSlider);
    usePercentDisplay (fetCompressionSlider);
    usePercentDisplay (pultecLimitSlider);

    // Output stays a fixed red, not part of the green "adjusted" highlight system
    makeupGainSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::red);

    compressionLabel.setText    ("ROUND",  juce::dontSendNotification);
    fetCompressionLabel.setText ("PUNCH",  juce::dontSendNotification);
    makeupGainLabel.setText     ("OUTPUT", juce::dontSendNotification);
    pultecLimitLabel.setText    ("JUICE",  juce::dontSendNotification);

    compressionLabel.setJustificationType    (juce::Justification::centred);
    fetCompressionLabel.setJustificationType (juce::Justification::centred);
    makeupGainLabel.setJustificationType     (juce::Justification::centred);
    pultecLimitLabel.setJustificationType    (juce::Justification::centred);

    compressionLabel.setFont    (eqLabelFont);
    fetCompressionLabel.setFont (eqLabelFont);
    makeupGainLabel.setFont     (eqLabelFont);
    pultecLimitLabel.setFont    (eqLabelFont);
    for (auto* l : { &compressionLabel, &fetCompressionLabel, &makeupGainLabel, &pultecLimitLabel })
        l->getProperties().set ("useCaptionFont", true);

    addAndMakeVisible (compressionSlider);
    addAndMakeVisible (fetCompressionSlider);
    addAndMakeVisible (makeupGainSlider);
    addAndMakeVisible (pultecLimitSlider);

    addAndMakeVisible (compressionLabel);
    addAndMakeVisible (fetCompressionLabel);
    addAndMakeVisible (pultecLimitLabel);
    // makeupGainLabel ("OUTPUT") stays hidden — its knob uses that space instead.
    makeupGainSlider.getProperties().set ("bigKnob", true);
    makeupGainSlider.getProperties().set ("metalBody", true);
    makeupGainSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 2) + " dB";
    };
    makeupGainSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.retainCharacters ("-0123456789.").getDoubleValue();
    };
    makeupGainSlider.updateText();

    // These sit inside their knob's own bounds now, so keep them frontmost —
    // otherwise the slider (added above) would paint over them.
    roundBypassButton.toFront (false);
    punchBypassButton.toFront (false);
    juiceBypassButton.toFront (false);

    const int knobColWidth      = 170;
    const int buttonColWidth    = 150;
    const int headerHeight      = 44;
    const int satRowHeight      = 34;
    const int sectionLabelHeight = 24;
    const int rowContentHeight  = 180;
    const int rowGap            = 10;

    setSize (knobColWidth * 4 + buttonColWidth,
             headerHeight + satRowHeight + (sectionLabelHeight + rowContentHeight) * 2 + rowGap + 10);
}

EQ5AudioProcessorEditor::~EQ5AudioProcessorEditor()
{
    stopTimer();
    mixSlider.setLookAndFeel (nullptr);
    nailKnob.setLookAndFeel (nullptr);
    nailKnobDisplay.setLookAndFeel (nullptr);
    roundBypassButton.setLookAndFeel (nullptr);
    punchBypassButton.setLookAndFeel (nullptr);
    juiceBypassButton.setLookAndFeel (nullptr);
    makeupGainSlider.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void EQ5AudioProcessorEditor::updateSaturationButtonStates()
{
    if (saturationParam == nullptr)
        return;

    int current = saturationParam->getIndex();
    satTapeButton.setToggleState      (current == 1, juce::dontSendNotification);
    satTubeButton.setToggleState      (current == 2, juce::dontSendNotification);
    satConsoleButton.setToggleState   (current == 3, juce::dontSendNotification);
    satFuzzButton.setToggleState      (current == 4, juce::dontSendNotification);
    satGermaniumButton.setToggleState (current == 5, juce::dontSendNotification);
}

void EQ5AudioProcessorEditor::timerCallback()
{
    updateSaturationButtonStates();
}

void EQ5AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141416));

    juce::ColourGradient vignette (juce::Colour (0xff000000).withAlpha (0.0f), (float) getWidth() * 0.5f, 0.0f,
                                   juce::Colour (0xff000000).withAlpha (0.35f), 0.0f, 0.0f, true);
    vignette.point2 = { (float) getWidth() * 0.5f, (float) getHeight() * 1.4f };
    g.setGradientFill (vignette);
    g.fillRect (getLocalBounds());

    const int headerHeight = 44;
    g.setColour (juce::Colour (0xff3a3a3c));
    g.drawHorizontalLine (headerHeight, 0.0f, (float) getWidth());

    auto drawSectionDivider = [&] (juce::Label& label)
    {
        auto b = label.getBounds();
        int y = b.getCentreY();
        g.setColour (juce::Colour (0xff3a3a3c));
        if (b.getX() > 10)
            g.drawHorizontalLine (y, 10.0f, (float) b.getX() - 6.0f);
        g.drawHorizontalLine (y, (float) b.getRight() + 6.0f, (float) getWidth() - 10.0f);
    };

    drawSectionDivider (eqSectionLabel);
    drawSectionDivider (compSectionLabel);

    const int knobColWidth = 170;
    for (int i = 1; i < 4; ++i)
    {
        int x = i * knobColWidth;
        g.setColour (juce::Colour (0xff3a3a3c));
        g.drawVerticalLine (x, (float) lowCutLabel.getBounds().getY() - 4.0f, (float) lowCutSlider.getBounds().getBottom() + 4.0f);
        g.drawVerticalLine (x, (float) compressionLabel.getBounds().getY() - 4.0f, (float) compressionSlider.getBounds().getBottom() + 4.0f);
    }

    int buttonColX = knobColWidth * 4;
    g.drawVerticalLine (buttonColX, (float) compressionLabel.getBounds().getY() - 4.0f, (float) compressionSlider.getBounds().getBottom() + 4.0f);
}

void EQ5AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    const int knobColWidth       = 170;
    const int buttonColWidth     = 150;
    const int headerHeight       = 44;
    const int satRowHeight       = 34;
    const int sectionLabelHeight = 24;
    const int rowContentHeight   = 180;
    const int rowGap             = 10;

        auto headerArea = area.removeFromTop (headerHeight);

        // centred above the Mid Gain knob (3rd of the 4 EQ columns)
        int midGainCentreX = knobColWidth * 2 + knobColWidth / 2;
        int logoWidth = 250;
        logoImageComponent.setBounds (midGainCentreX - logoWidth / 2, headerArea.getY() + 4, logoWidth, headerArea.getHeight() - 8);

        // Hidden saturation-drive knob, sat invisibly over the top-right screw of the
        // nameplate graphic — positioned from the image's own source aspect
        // ratio so it tracks the logo if its displayed size ever changes.
        {
            auto  logoBounds     = logoImageComponent.getBounds();
            int   logoCompHeight = logoBounds.getHeight();
            float scale = juce::jmin ((float) logoWidth / 2172.0f, (float) logoCompHeight / 724.0f);
            float dispW = 2172.0f * scale;
            float dispH = 724.0f * scale;
            float xOff  = ((float) logoWidth - dispW) * 0.5f;
            float yOff  = ((float) logoCompHeight - dispH) * 0.5f;

            const float screwFracX = 0.929f;
            const float screwFracY = 0.242f;
            float screwX = (float) logoBounds.getX() + xOff + screwFracX * dispW;
            float screwY = (float) logoBounds.getY() + yOff + screwFracY * dispH;

            int hitSize = 18; // dot itself renders smaller (~42%), close to the real screw size
            nailKnob.setBounds ((int) screwX - hitSize / 2, (int) screwY - hitSize / 2, hitSize, hitSize);

            int dispW2 = 35, dispH2 = 45;
            int dispX = juce::jlimit (0, getWidth() - dispW2, logoBounds.getRight() + 6);
            int dispY = juce::jlimit (0, getHeight() - dispH2, (int) screwY - dispH2 / 2);
            nailKnobDisplay.setBounds (dispX, dispY, dispW2, dispH2);
        }

        // Saturation row — 5 mutually-exclusive vintage-flavour buttons
        {
            auto satRowArea = area.removeFromTop (satRowHeight);
            juce::TextButton* satButtons[] = { &satTapeButton, &satTubeButton, &satConsoleButton, &satFuzzButton, &satGermaniumButton };
            int satButtonGap = 6;
            int satButtonWidth = (satRowArea.getWidth() - satButtonGap * 4) / 5;
            int satX = satRowArea.getX();
            for (auto* b : satButtons)
            {
                b->setBounds (satX, satRowArea.getY() + 2, satButtonWidth, satRowArea.getHeight() - 4);
                satX += satButtonWidth + satButtonGap;
            }
        }

        // Row 1: EQ
        auto eqSectionArea = area.removeFromTop (sectionLabelHeight);
        eqSectionLabel.setBounds (10, eqSectionArea.getY(), 60, sectionLabelHeight);

        auto eqRowArea = area.removeFromTop (rowContentHeight);
        auto lowCutArea  = eqRowArea.removeFromLeft (knobColWidth);
        auto midFreqArea = eqRowArea.removeFromLeft (knobColWidth);
        auto midGainArea = eqRowArea.removeFromLeft (knobColWidth);
        auto hiCutArea   = eqRowArea.removeFromLeft (knobColWidth);

        lowCutLabel.setBounds  (lowCutArea.removeFromTop  (20));
        midFreqLabel.setBounds (midFreqArea.removeFromTop (20));
        midGainLabel.setBounds (midGainArea.removeFromTop (20));
        hiCutLabel.setBounds   (hiCutArea.removeFromTop   (20));

        lowCutSlider.setBounds  (lowCutArea);
        midFreqSlider.setBounds (midFreqArea);
        midGainSlider.setBounds (midGainArea);
        hiCutSlider.setBounds   (hiCutArea);

        mixLabel.setBounds  (eqRowArea.removeFromTop (20));
        mixSlider.setBounds (eqRowArea);

        area.removeFromTop (rowGap);

        // Row 2: COMP
        auto compSectionArea = area.removeFromTop (sectionLabelHeight);
        compSectionLabel.setBounds (10, compSectionArea.getY(), 80, sectionLabelHeight);

        auto compRowArea = area.removeFromTop (rowContentHeight);
        auto compArea    = compRowArea.removeFromLeft (knobColWidth);
        auto fetCompArea = compRowArea.removeFromLeft (knobColWidth);
        auto pultecArea  = compRowArea.removeFromLeft (knobColWidth);
        auto makeupArea  = compRowArea.removeFromLeft (knobColWidth);
        auto buttonArea  = compRowArea;

        compressionLabel.setBounds    (compArea.removeFromTop (20));
        fetCompressionLabel.setBounds (fetCompArea.removeFromTop (20));
        pultecLimitLabel.setBounds    (pultecArea.removeFromTop (20));
        // Output's label is hidden and its knob gets the full column height instead —
        // this knob gets touched constantly (it's compensating for all the makeup gain
        // from Round/Punch/Juice), so it needs to be big and easy to grab.

        compressionSlider.setBounds    (compArea);
        fetCompressionSlider.setBounds (fetCompArea);
        pultecLimitSlider.setBounds    (pultecArea);

        int bypDotSize = 14;
        roundBypassButton.setBounds (compArea.getRight()    - bypDotSize - 4, compArea.getBottom()    - bypDotSize - 4, bypDotSize, bypDotSize);
        punchBypassButton.setBounds (fetCompArea.getRight() - bypDotSize - 4, fetCompArea.getBottom() - bypDotSize - 4, bypDotSize, bypDotSize);
        juiceBypassButton.setBounds (pultecArea.getRight()  - bypDotSize - 4, pultecArea.getBottom()  - bypDotSize - 4, bypDotSize, bypDotSize);
        makeupGainSlider.setBounds     (makeupArea);

        auto bypassArea = buttonArea.removeFromTop (buttonArea.getHeight() / 2);
        auto twoXArea   = buttonArea;

        int knobSize = juce::jmin (bypassArea.getWidth(), bypassArea.getHeight()) - 20;
        bypassButton.setBounds (bypassArea.withSizeKeepingCentre (knobSize, knobSize));

        int twoXSize = juce::jmin (twoXArea.getWidth(), twoXArea.getHeight()) - 30;
        twoXButton.setBounds (twoXArea.withSizeKeepingCentre (twoXSize, twoXSize));
    }
