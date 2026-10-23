#pragma once

#include <JuceHeader.h>

struct Preset
{
    juce::String name;
    juce::String description;
    std::vector<std::pair<const char*, float>> values;   // parameter ID -> real (unnormalised) value
};

const std::vector<Preset>& getFactoryPresets();

// Resets every parameter except bypass to its default, then applies the preset's values
void applyPreset (juce::AudioProcessorValueTreeState& state, const Preset& preset);
