#include "Parameters.h"

namespace
{
    using juce::String;

    struct Format
    {
        std::function<String (float, int)> toText;
        std::function<float (const String&)> fromText;
    };

    float parseNumber (const String& text)
    {
        const auto value = text.retainCharacters ("0123456789.-").getFloatValue();
        return text.containsIgnoreCase ("k") ? value * 1000.0f : value;
    }

    const Format percent { [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + " %"; },
                           [] (const String& t) { return parseNumber (t) / 100.0f; } };
    const Format hertz { [] (float v, int) { return v < 1000.0f ? String (juce::roundToInt (v)) + " Hz"
                                                                : String (v / 1000.0f, 1) + " kHz"; },
                         parseNumber };
    const Format lfoHertz { [] (float v, int) { return String (v, 2) + " Hz"; }, parseNumber };
    const Format millis { [] (float v, int) { return String (juce::roundToInt (v)) + " ms"; }, parseNumber };
    const Format seconds { [] (float v, int) { return String (v, 1) + " s"; }, parseNumber };
    const Format decibels { [] (float v, int) { return (v > 0.0f ? "+" : "") + String (v, 1) + " dB"; }, parseNumber };
    const Format multiplier { [] (float v, int) { return String (v, 2) + "x"; }, parseNumber };
    const Format bitDepth { [] (float v, int) { return String (v, 1) + " bit"; }, parseNumber };
    const Format plain { [] (float v, int) { return String (v, 2); }, parseNumber };

    // AudioParameterBool reports whatever in-between value a host writes (e.g. 0.62), which then
    // doesn't survive a save/restore. This one always reports exactly 0 or 1.
    class SwitchParameter final : public juce::AudioParameterBool
    {
    public:
        using AudioParameterBool::AudioParameterBool;

    private:
        float getValue() const override { return get() ? 1.0f : 0.0f; }
    };

    juce::NormalisableRange<float> range (float lo, float hi, float interval = 0.0f, float centre = 0.0f)
    {
        juce::NormalisableRange<float> r (lo, hi, interval);

        if (centre > 0.0f)
            r.setSkewForCentre (centre);

        return r;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&] (const char* id, const char* name, juce::NormalisableRange<float> r, float def, const Format& f)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, r, def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (f.toText)
                                                 .withValueFromStringFunction (f.fromText)));
    };

    auto addBool = [&] (const char* id, const char* name, bool def)
    {
        layout.add (std::make_unique<SwitchParameter> (juce::ParameterID { id, 1 }, name, def));
    };

    addBool (IDs::bypass, "Bypass", false);

    addBool (IDs::speedOn, "Speed On", false);
    addFloat (IDs::speed, "Speed", range (0.5f, 1.5f, 0.01f), 1.0f, multiplier);

    addBool (IDs::lofiOn, "Lo-Fi On", false);
    addFloat (IDs::lofiBits, "Lo-Fi Bits", range (4.0f, 16.0f, 0.1f), 10.0f, bitDepth);
    addFloat (IDs::lofiRate, "Lo-Fi Rate", range (2000.0f, 44100.0f, 1.0f, 11025.0f), 16000.0f, hertz);
    addFloat (IDs::lofiWow, "Lo-Fi Wow", range (0.0f, 1.0f), 0.3f, percent);
    addFloat (IDs::lofiNoise, "Lo-Fi Noise", range (0.0f, 1.0f), 0.2f, percent);
    addFloat (IDs::lofiTone, "Lo-Fi Tone", range (1000.0f, 20000.0f, 1.0f, 5000.0f), 7000.0f, hertz);
    addFloat (IDs::lofiMix, "Lo-Fi Mix", range (0.0f, 1.0f), 1.0f, percent);

    addBool (IDs::filterOn, "Filter On", false);
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { IDs::filterType, 1 }, "Filter Type",
                                                              juce::StringArray { "Low-pass", "High-pass", "Band-pass" }, 0));
    addFloat (IDs::filterCutoff, "Filter Cutoff", range (20.0f, 20000.0f, 1.0f, 1000.0f), 1000.0f, hertz);
    addFloat (IDs::filterRes, "Filter Resonance", range (0.5f, 8.0f, 0.01f, 1.5f), 0.71f, plain);

    addBool (IDs::bassOn, "Bass On", false);
    addFloat (IDs::bassGain, "Bass Boost", range (0.0f, 15.0f, 0.1f), 6.0f, decibels);
    addFloat (IDs::bassFreq, "Bass Frequency", range (40.0f, 250.0f, 1.0f), 90.0f, hertz);

    addBool (IDs::driveOn, "Drive On", false);
    addFloat (IDs::driveAmount, "Drive", range (0.0f, 1.0f), 0.3f, percent);
    addFloat (IDs::driveMix, "Drive Mix", range (0.0f, 1.0f), 1.0f, percent);

    addBool (IDs::chorusOn, "Chorus On", false);
    addFloat (IDs::chorusRate, "Chorus Rate", range (0.05f, 5.0f, 0.01f, 0.8f), 0.8f, lfoHertz);
    addFloat (IDs::chorusDepth, "Chorus Depth", range (0.0f, 1.0f), 0.4f, percent);
    addFloat (IDs::chorusMix, "Chorus Mix", range (0.0f, 1.0f), 0.5f, percent);

    addBool (IDs::echoOn, "Echo On", false);
    addFloat (IDs::echoTime, "Echo Time", range (20.0f, 1500.0f, 1.0f, 300.0f), 350.0f, millis);
    addFloat (IDs::echoFeedback, "Echo Feedback", range (0.0f, 0.95f), 0.4f, percent);
    addFloat (IDs::echoTone, "Echo Tone", range (1000.0f, 20000.0f, 1.0f, 5000.0f), 6000.0f, hertz);
    addFloat (IDs::echoMix, "Echo Mix", range (0.0f, 1.0f), 0.3f, percent);
    addBool (IDs::echoPingPong, "Echo Ping-Pong", true);

    addBool (IDs::reverbOn, "Reverb On", false);
    addFloat (IDs::reverbSize, "Reverb Size", range (0.0f, 1.0f), 0.7f, percent);
    addFloat (IDs::reverbDamping, "Reverb Damping", range (0.0f, 1.0f), 0.5f, percent);
    addFloat (IDs::reverbWidth, "Reverb Width", range (0.0f, 1.0f), 1.0f, percent);
    addFloat (IDs::reverbMix, "Reverb Mix", range (0.0f, 1.0f), 0.3f, percent);

    addBool (IDs::pan8dOn, "8D On", false);
    addFloat (IDs::pan8dRate, "8D Rate", range (0.02f, 1.0f, 0.01f, 0.2f), 0.15f, lfoHertz);
    addFloat (IDs::pan8dDepth, "8D Depth", range (0.0f, 1.0f), 1.0f, percent);

    addBool (IDs::widthOn, "Width On", false);
    addFloat (IDs::widthAmount, "Stereo Width", range (0.0f, 2.0f), 1.4f, percent);

    addBool (IDs::fadesOn, "Fades On", false);
    addFloat (IDs::fadeIn, "Intro Fade", range (0.0f, 10.0f, 0.1f), 3.0f, seconds);
    addFloat (IDs::fadeOut, "Outro Fade", range (0.0f, 10.0f, 0.1f), 4.0f, seconds);
    addBool (IDs::outro, "Fade Out Now", false);

    addFloat (IDs::outputGain, "Output Gain", range (-24.0f, 12.0f, 0.1f), 0.0f, decibels);
    addFloat (IDs::outputMix, "Output Mix", range (0.0f, 1.0f), 1.0f, percent);

    return layout;
}
