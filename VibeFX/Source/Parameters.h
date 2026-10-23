#pragma once

#include <JuceHeader.h>

namespace IDs
{
    inline constexpr auto bypass = "bypass";

    inline constexpr auto speedOn = "speed_on";
    inline constexpr auto speed = "speed";

    inline constexpr auto lofiOn = "lofi_on";
    inline constexpr auto lofiBits = "lofi_bits";
    inline constexpr auto lofiRate = "lofi_rate";
    inline constexpr auto lofiWow = "lofi_wow";
    inline constexpr auto lofiNoise = "lofi_noise";
    inline constexpr auto lofiTone = "lofi_tone";
    inline constexpr auto lofiMix = "lofi_mix";

    inline constexpr auto filterOn = "filter_on";
    inline constexpr auto filterType = "filter_type";
    inline constexpr auto filterCutoff = "filter_cutoff";
    inline constexpr auto filterRes = "filter_res";

    inline constexpr auto bassOn = "bass_on";
    inline constexpr auto bassGain = "bass_gain";
    inline constexpr auto bassFreq = "bass_freq";

    inline constexpr auto driveOn = "drive_on";
    inline constexpr auto driveAmount = "drive_amount";
    inline constexpr auto driveMix = "drive_mix";

    inline constexpr auto chorusOn = "chorus_on";
    inline constexpr auto chorusRate = "chorus_rate";
    inline constexpr auto chorusDepth = "chorus_depth";
    inline constexpr auto chorusMix = "chorus_mix";

    inline constexpr auto echoOn = "echo_on";
    inline constexpr auto echoTime = "echo_time";
    inline constexpr auto echoFeedback = "echo_feedback";
    inline constexpr auto echoTone = "echo_tone";
    inline constexpr auto echoMix = "echo_mix";
    inline constexpr auto echoPingPong = "echo_pingpong";

    inline constexpr auto reverbOn = "reverb_on";
    inline constexpr auto reverbSize = "reverb_size";
    inline constexpr auto reverbDamping = "reverb_damping";
    inline constexpr auto reverbWidth = "reverb_width";
    inline constexpr auto reverbMix = "reverb_mix";

    inline constexpr auto pan8dOn = "pan8d_on";
    inline constexpr auto pan8dRate = "pan8d_rate";
    inline constexpr auto pan8dDepth = "pan8d_depth";

    inline constexpr auto widthOn = "width_on";
    inline constexpr auto widthAmount = "width_amount";

    inline constexpr auto fadesOn = "fades_on";
    inline constexpr auto fadeIn = "fade_in";
    inline constexpr auto fadeOut = "fade_out";
    inline constexpr auto outro = "outro";

    inline constexpr auto outputGain = "output_gain";
    inline constexpr auto outputMix = "output_mix";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
