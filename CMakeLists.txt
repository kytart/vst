cmake_minimum_required(VERSION 3.22)

project(OrbitDelay VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Apple Silicon only. Universal binaries are a distribution concern, not a local one.
set(CMAKE_OSX_ARCHITECTURES arm64)

add_subdirectory(libs/JUCE)

juce_add_plugin(OrbitDelay
    COMPANY_NAME "Palm Muting"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE       # installs to ~/Library/Audio/Plug-Ins/VST3 on every build
    BUNDLE_ID com.palmmuting.orbitdelay
    PLUGIN_MANUFACTURER_CODE Palm
    PLUGIN_CODE Dly1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Orbit Delay")

target_sources(OrbitDelay PRIVATE
    src/PluginProcessor.cpp)

target_compile_definitions(OrbitDelay PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(OrbitDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
