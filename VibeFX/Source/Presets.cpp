#include "Presets.h"
#include "Parameters.h"

const std::vector<Preset>& getFactoryPresets()
{
    using namespace IDs;

    static const std::vector<Preset> presets {
        { "Clean", "Everything off: the original sound.", {} },

        { "Slowed + Reverb", "The classic TikTok edit: slower, deeper and washed in reverb.",
          { { speedOn, 1 }, { speed, 0.82f },
            { reverbOn, 1 }, { reverbSize, 0.88f }, { reverbDamping, 0.45f }, { reverbWidth, 1.0f }, { reverbMix, 0.38f },
            { bassOn, 1 }, { bassGain, 3.0f }, { bassFreq, 90.0f },
            { filterOn, 1 }, { filterType, 0 }, { filterCutoff, 9000.0f },
            { fadesOn, 1 }, { fadeIn, 2.0f }, { fadeOut, 4.0f } } },

        { "Nightcore", "Faster and higher, with a bright sparkle.",
          { { speedOn, 1 }, { speed, 1.3f },
            { bassOn, 1 }, { bassGain, 2.0f },
            { reverbOn, 1 }, { reverbSize, 0.4f }, { reverbMix, 0.12f } } },

        { "Sped Up", "Hype-edit energy: 25% faster.",
          { { speedOn, 1 }, { speed, 1.25f } } },

        { "Lo-Fi Chill", "Dusty cassette: crunchy, warm and wobbly, with vinyl crackle.",
          { { speedOn, 1 }, { speed, 0.95f },
            { lofiOn, 1 }, { lofiBits, 9.5f }, { lofiRate, 14000.0f }, { lofiWow, 0.45f }, { lofiNoise, 0.35f },
            { lofiTone, 5500.0f }, { lofiMix, 1.0f },
            { chorusOn, 1 }, { chorusRate, 0.4f }, { chorusDepth, 0.25f }, { chorusMix, 0.3f },
            { reverbOn, 1 }, { reverbSize, 0.5f }, { reverbMix, 0.18f },
            { bassOn, 1 }, { bassGain, 3.0f } } },

        { "Vaporwave", "Slowed way down, with dreamy chorus and a huge reverb.",
          { { speedOn, 1 }, { speed, 0.78f },
            { chorusOn, 1 }, { chorusRate, 0.3f }, { chorusDepth, 0.6f }, { chorusMix, 0.5f },
            { reverbOn, 1 }, { reverbSize, 0.92f }, { reverbDamping, 0.6f }, { reverbMix, 0.42f },
            { lofiOn, 1 }, { lofiBits, 12.0f }, { lofiRate, 22050.0f }, { lofiWow, 0.25f }, { lofiNoise, 0.08f },
            { lofiTone, 9000.0f }, { lofiMix, 0.7f },
            { filterOn, 1 }, { filterType, 0 }, { filterCutoff, 7500.0f } } },

        { "8D Audio", "The sound circles around your head. Use headphones!",
          { { pan8dOn, 1 }, { pan8dRate, 0.12f }, { pan8dDepth, 1.0f },
            { reverbOn, 1 }, { reverbSize, 0.6f }, { reverbMix, 0.2f } } },

        { "Telephone", "Thin and narrow, like a phone call.",
          { { filterOn, 1 }, { filterType, 2 }, { filterCutoff, 1400.0f }, { filterRes, 1.2f },
            { driveOn, 1 }, { driveAmount, 0.35f },
            { widthOn, 1 }, { widthAmount, 0.0f } } },

        { "Old Radio", "1940s AM radio: band-limited, noisy and mono.",
          { { filterOn, 1 }, { filterType, 2 }, { filterCutoff, 1800.0f }, { filterRes, 0.9f },
            { lofiOn, 1 }, { lofiBits, 8.0f }, { lofiRate, 11025.0f }, { lofiWow, 0.15f }, { lofiNoise, 0.45f },
            { lofiTone, 4500.0f },
            { driveOn, 1 }, { driveAmount, 0.45f },
            { widthOn, 1 }, { widthAmount, 0.0f } } },

        { "Underwater", "Muffled and wobbly, as if heard from below the surface.",
          { { filterOn, 1 }, { filterType, 0 }, { filterCutoff, 450.0f }, { filterRes, 1.1f },
            { chorusOn, 1 }, { chorusRate, 0.25f }, { chorusDepth, 0.8f }, { chorusMix, 0.6f },
            { reverbOn, 1 }, { reverbSize, 0.7f }, { reverbDamping, 0.8f }, { reverbMix, 0.3f },
            { bassOn, 1 }, { bassGain, 4.0f } } },

        { "Echo Canyon", "Ping-pong echoes bouncing between left and right.",
          { { echoOn, 1 }, { echoTime, 380.0f }, { echoFeedback, 0.5f }, { echoTone, 5000.0f }, { echoMix, 0.38f },
            { echoPingPong, 1 },
            { reverbOn, 1 }, { reverbSize, 0.6f }, { reverbMix, 0.18f } } },

        { "Concert Hall", "A big, natural room around the track.",
          { { reverbOn, 1 }, { reverbSize, 0.93f }, { reverbDamping, 0.35f }, { reverbWidth, 1.0f }, { reverbMix, 0.33f },
            { widthOn, 1 }, { widthAmount, 1.2f } } },

        { "Bass Boosted", "Heavy low end with a touch of saturation.",
          { { bassOn, 1 }, { bassGain, 11.0f }, { bassFreq, 70.0f },
            { driveOn, 1 }, { driveAmount, 0.25f }, { driveMix, 0.6f },
            { outputGain, -3.0f } } },

        { "Dreamy", "Wide, shimmering chorus, echo and reverb.",
          { { chorusOn, 1 }, { chorusRate, 0.5f }, { chorusDepth, 0.5f }, { chorusMix, 0.45f },
            { echoOn, 1 }, { echoTime, 450.0f }, { echoFeedback, 0.35f }, { echoTone, 4000.0f }, { echoMix, 0.22f },
            { reverbOn, 1 }, { reverbSize, 0.85f }, { reverbDamping, 0.4f }, { reverbMix, 0.32f },
            { widthOn, 1 }, { widthAmount, 1.6f } } },

        { "Intro / Outro Fades", "Fades in when playback starts and out at the end.",
          { { fadesOn, 1 }, { fadeIn, 4.0f }, { fadeOut, 5.0f } } },
    };

    return presets;
}

void applyPreset (juce::AudioProcessorValueTreeState& state, const Preset& preset)
{
    for (auto* parameter : state.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
            if (ranged->getParameterID() != IDs::bypass)
                ranged->setValueNotifyingHost (ranged->getDefaultValue());

    for (const auto& [id, value] : preset.values)
        if (auto* parameter = state.getParameter (id))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}
