cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel). Must be set before project().
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(GainPlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libs/JUCE)

juce_add_plugin(GainPlugin
    COMPANY_NAME "Espirit Tech"
    COMPANY_WEBSITE "https://example.com"
    BUNDLE_ID "com.espirittech.gainplugin"
    PLUGIN_MANUFACTURER_CODE Esp1      # 4 chars, at least one uppercase
    PLUGIN_CODE Gn01                   # 4 chars, unique per plugin
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Gain PoC"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Tools
    COPY_PLUGIN_AFTER_BUILD TRUE)

juce_generate_juce_header(GainPlugin)

target_sources(GainPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(GainPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(GainPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Multi-effect showcase plugin with dashboard UI
add_subdirectory(VibeFX)
