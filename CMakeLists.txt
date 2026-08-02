cmake_minimum_required(VERSION 3.22)

project(NeekQ VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# AU is macOS/iOS-only — JUCE would reject the format on Windows/Linux anyway,
# but keeping it out of the FORMATS list entirely avoids the AU-specific
# configure step running on platforms that can't build it.
if (APPLE)
    set(NEEKQ_FORMATS VST3 AU Standalone)
else()
    set(NEEKQ_FORMATS VST3 Standalone)
endif()

juce_add_plugin(NeekQ
    COMPANY_NAME "Neek Audio"
    PRODUCT_NAME "NeekQ"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Luhk
    BUNDLE_ID "com.yourcompany.EQ5"
    FORMATS ${NEEKQ_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(NeekQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/FourKnobEQ.h
    Source/ThreeBandEQ.h
    Source/OptoCompressor.h
    Source/FETCompressor.h
    Source/PultecEQ.h
    Source/Limiter.h
    Source/Saturation.h
    Source/HardwareLookAndFeel.h
    Source/FaceMixLookAndFeel.h
    Source/AkiraExpandedFont.h
    Source/AkiraExpandedFontData.h
    Source/LogoData.h
    Source/FaceStencilData.h
)

target_compile_definitions(NeekQ PUBLIC
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(NeekQ PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce_recommended_config_flags
    juce_recommended_lto_flags
    juce_recommended_warning_flags
)
