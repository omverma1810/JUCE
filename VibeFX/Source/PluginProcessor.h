#pragma once

#include <JuceHeader.h>
#include "Parameters.h"
#include "DSP/Effects.h"
#include "DSP/FilePlayer.h"
#include "DSP/Visualiser.h"

class VibeFXProcessor final : public juce::AudioProcessor
{
public:
    VibeFXProcessor();
    ~VibeFXProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParameter; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Shared with the editor
    vibe::FilePlayer player;
    vibe::AnalyserFifo inputAnalyser, outputAnalyser;
    vibe::LevelMeterSource inputMeter, outputMeter;
    std::atomic<bool> muteOutput { false };   // keeps the analysers running but silences the speakers

private:
    enum Module { speedModule, lofiModule, filterModule, bassModule, driveModule, chorusModule,
                  echoModule, reverbModule, pan8dModule, numModules };

    bool justEnabled (Module module, bool isOn);
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool isOn (const char* id) const { return value (id) > 0.5f; }

    juce::AudioProcessorParameter* bypassParameter = nullptr;
    int currentProgram = 0;

    vibe::TapePitchShifter pitch;
    vibe::LoFi lofi;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> bass;
    juce::dsp::Chorus<float> chorus;
    vibe::StereoEcho echo;
    juce::dsp::Reverb reverb;
    vibe::AutoPan8D pan8d;
    vibe::FadeEnvelope fades;
    juce::dsp::Limiter<float> limiter;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
    juce::SmoothedValue<float> wetAmount;
    juce::AudioBuffer<float> dryBuffer;

    std::array<bool, numModules> wasOn {};
    float lastBassGain = -1.0f, lastBassFreq = -1.0f;
    bool wasHostPlaying = false, wasOutro = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeFXProcessor)
};
