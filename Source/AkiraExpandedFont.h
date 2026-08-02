#pragma once
#include <JuceHeader.h>
#include "AkiraExpandedFontData.h"

// Embedded directly (rather than looked up by family name) because this
// font's style metadata ("Super Bold", no "Regular") doesn't resolve
// reliably through the system font matcher — and this way the plugin
// doesn't depend on the font being installed on whatever machine runs it.
inline juce::Typeface::Ptr getAkiraExpandedTypeface()
{
    static juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor (AkiraExpandedFontData, (size_t) AkiraExpandedFontDataSize);
    return typeface;
}
