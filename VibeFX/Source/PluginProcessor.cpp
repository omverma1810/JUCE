#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

VibeFXProcessor::VibeFXProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    bypassParameter = apvts.getParameter (IDs::bypass);
}

void VibeFXProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto numChannels = (juce::uint32) juce::jmax (1, getTotalNumInputChannels(), getTotalNumOutputChannels());
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, numChannels };

    pitch.prepare (sampleRate);
    lofi.prepare (spec);
    filter.prepare (spec);

    // Coefficients must exist before prepare() so the filter state is sized correctly
    *bass.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (sampleRate, 90.0f, 0.7f, 1.0f);
    bass.prepare (spec);
    lastBassGain = lastBassFreq = -1.0f;

    chorus.prepare (spec);
    echo.prepare (sampleRate);
    reverb.prepare (spec);
    pan8d.prepare (sampleRate);
    fades.prepare (sampleRate);

    limiter.prepare (spec);
    limiter.setThreshold (0.0f);   // safety ceiling at 0 dBFS; lower values also add make-up gain
    limiter.setRelease (80.0f);

    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (value (IDs::outputGain)));
    wetAmount.reset (sampleRate, 0.03);
    wetAmount.setCurrentAndTargetValue (isOn (IDs::bypass) ? 0.0f : value (IDs::outputMix));

    dryBuffer.setSize ((int) numChannels, samplesPerBlock);
    player.prepare (sampleRate);

    wasOn.fill (false);
    wasHostPlaying = false;
    wasOutro = false;
}

bool VibeFXProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

bool VibeFXProcessor::justEnabled (Module module, bool on)
{
    const bool edge = on && ! wasOn[(size_t) module];
    wasOn[(size_t) module] = on;
    return edge;
}

void VibeFXProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    const bool speedOn = isOn (IDs::speedOn);
    const auto speed = value (IDs::speed);

    // 1. Source: the built-in player (standalone demos) replaces the live input
    vibe::FilePlayer::RenderResult playerState;
    if (player.hasFile())
        playerState = player.render (buffer, speedOn ? speed : 1.0f);

    bool hostPlaying = false;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            hostPlaying = position->getIsPlaying();

    const bool transportStarted = (hostPlaying && ! wasHostPlaying) || playerState.startedFromTop;
    wasHostPlaying = hostPlaying;

    inputMeter.update (buffer);
    inputAnalyser.push (buffer);

    if (dryBuffer.getNumChannels() < numChannels || dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    // 2. Effect chain
    // The player already plays the file at the new speed, so only live input needs the pitch shifter
    const bool pitchActive = speedOn && ! playerState.active;
    if (justEnabled (speedModule, pitchActive)) pitch.reset();
    if (pitchActive)
        pitch.process (buffer, speed);

    const bool lofiOn = isOn (IDs::lofiOn);
    if (justEnabled (lofiModule, lofiOn)) lofi.reset();
    if (lofiOn)
        lofi.process (buffer, value (IDs::lofiBits), value (IDs::lofiRate), value (IDs::lofiWow),
                      value (IDs::lofiNoise), value (IDs::lofiTone), value (IDs::lofiMix));

    const bool filterOn = isOn (IDs::filterOn);
    if (justEnabled (filterModule, filterOn)) filter.reset();
    if (filterOn)
    {
        using Type = juce::dsp::StateVariableTPTFilterType;
        const auto type = juce::roundToInt (value (IDs::filterType));
        filter.setType (type == 0 ? Type::lowpass : type == 1 ? Type::highpass : Type::bandpass);
        filter.setCutoffFrequency (juce::jmin (value (IDs::filterCutoff), (float) getSampleRate() * 0.45f));
        filter.setResonance (value (IDs::filterRes));
        filter.process (context);
    }

    const bool bassOn = isOn (IDs::bassOn);
    if (justEnabled (bassModule, bassOn)) bass.reset();
    if (bassOn)
    {
        const auto gainDb = value (IDs::bassGain);
        const auto freq = value (IDs::bassFreq);

        if (! juce::exactlyEqual (gainDb, lastBassGain) || ! juce::exactlyEqual (freq, lastBassFreq))
        {
            *bass.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (
                getSampleRate(), freq, 0.7f, juce::Decibels::decibelsToGain (gainDb));
            lastBassGain = gainDb;
            lastBassFreq = freq;
        }

        bass.process (context);
    }

    const bool driveOn = isOn (IDs::driveOn);
    justEnabled (driveModule, driveOn);
    if (driveOn)
        vibe::applySaturation (buffer, value (IDs::driveAmount), value (IDs::driveMix));

    const bool chorusOn = isOn (IDs::chorusOn);
    if (justEnabled (chorusModule, chorusOn)) chorus.reset();
    if (chorusOn)
    {
        chorus.setRate (value (IDs::chorusRate));
        chorus.setDepth (value (IDs::chorusDepth));
        chorus.setCentreDelay (8.0f);
        chorus.setFeedback (0.1f);
        chorus.setMix (value (IDs::chorusMix));
        chorus.process (context);
    }

    const bool echoOn = isOn (IDs::echoOn);
    if (justEnabled (echoModule, echoOn)) echo.reset();
    if (echoOn)
        echo.process (buffer, value (IDs::echoTime), value (IDs::echoFeedback), value (IDs::echoTone),
                      value (IDs::echoMix), isOn (IDs::echoPingPong));

    const bool reverbOn = isOn (IDs::reverbOn);
    if (justEnabled (reverbModule, reverbOn)) reverb.reset();
    if (reverbOn)
    {
        const auto mix = value (IDs::reverbMix);
        juce::dsp::Reverb::Parameters p;
        p.roomSize = value (IDs::reverbSize);
        p.damping = value (IDs::reverbDamping);
        p.width = value (IDs::reverbWidth);
        p.wetLevel = mix * 0.7f;
        p.dryLevel = 1.0f - mix * 0.5f;
        reverb.setParameters (p);
        reverb.process (context);
    }

    const bool pan8dOn = isOn (IDs::pan8dOn);
    if (justEnabled (pan8dModule, pan8dOn)) pan8d.prepare (getSampleRate());
    if (pan8dOn)
        pan8d.process (buffer, value (IDs::pan8dRate), value (IDs::pan8dDepth));

    if (isOn (IDs::widthOn))
        vibe::applyStereoWidth (buffer, value (IDs::widthAmount));

    // Intro / outro: fade in on transport start, fade out on request or near the end of the demo file
    const bool outro = isOn (IDs::outro);

    if (isOn (IDs::fadesOn))
    {
        if (transportStarted)
            fades.startIntro (value (IDs::fadeIn));

        if (outro && ! wasOutro)
            fades.startOutro (value (IDs::fadeOut));
        else if (! outro && wasOutro)
            fades.cancelOutro();

        if (playerState.active && player.isPlaying() && fades.autoOutroArmed
            && playerState.secondsRemaining <= value (IDs::fadeOut))
        {
            fades.startOutro (value (IDs::fadeOut));
            fades.autoOutroArmed = false;
        }

        fades.process (buffer);
    }
    else
    {
        fades.setFullyOn();
    }

    wasOutro = outro;

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (value (IDs::outputGain)));
    outputGain.applyGain (buffer, numSamples);
    limiter.process (context);

    // 3. Blend with the untouched signal (output mix and bypass, both smoothed)
    wetAmount.setTargetValue (isOn (IDs::bypass) ? 0.0f : value (IDs::outputMix));

    if (wetAmount.isSmoothing() || wetAmount.getTargetValue() < 1.0f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto wet = wetAmount.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* out = buffer.getWritePointer (ch);
                out[i] = dryBuffer.getSample (ch, i) + wet * (out[i] - dryBuffer.getSample (ch, i));
            }
        }
    }

    outputMeter.update (buffer);
    outputAnalyser.push (buffer);

    if (muteOutput)
        buffer.clear();
}

int VibeFXProcessor::getNumPrograms()
{
    return (int) getFactoryPresets().size();
}

void VibeFXProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();

    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;

    currentProgram = index;
    applyPreset (apvts, presets[(size_t) index]);
}

const juce::String VibeFXProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? presets[(size_t) index].name : juce::String();
}

juce::AudioProcessorEditor* VibeFXProcessor::createEditor()
{
    return new VibeFXEditor (*this);
}

void VibeFXProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VibeFXProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("program", 0));
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibeFXProcessor();
}
