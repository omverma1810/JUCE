juce_add_plugin(VibeFX
    COMPANY_NAME "Espirit Tech"
    COMPANY_WEBSITE "https://example.com"
    BUNDLE_ID "com.espirittech.vibefx"
    PLUGIN_MANUFACTURER_CODE Esp1
    PLUGIN_CODE Vbfx
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Vibe FX"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD TRUE)

juce_generate_juce_header(VibeFX)

target_sources(VibeFX
    PRIVATE
        Source/Parameters.cpp
        Source/Presets.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(VibeFX
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(VibeFX
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
