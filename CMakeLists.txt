cmake_minimum_required(VERSION 3.22)

project(AutoVoxTune VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# JUCE se descarga automaticamente (misma version que el proyecto FET 1176).
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AutoVoxTune
    COMPANY_NAME "OpenCodeDSP"
    PLUGIN_MANUFACTURER_CODE Ocds
    PLUGIN_CODE Avtn
    FORMATS VST3
    PRODUCT_NAME "AutoVox Tune"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(AutoVoxTune
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PitchDetector.cpp
        Source/PitchShifter.cpp
        Source/FormantFilter.cpp
        Source/MusicalScale.cpp
)

target_include_directories(AutoVoxTune PRIVATE Source)

target_compile_definitions(AutoVoxTune
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AutoVoxTune
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
