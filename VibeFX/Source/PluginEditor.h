#pragma once

#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/EffectCard.h"
#include "UI/Visualisers.h"
#include "UI/PlayerBar.h"

class VibeFXEditor final : public juce::AudioProcessorEditor,
                           public juce::FileDragAndDropTarget,
                           private juce::Timer
{
public:
    explicit VibeFXEditor (VibeFXProcessor&);
    ~VibeFXEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void timerCallback() override;
    void selectPreset (int index);
    void handleCommandLine();
    bool isStandalone() const;

    VibeFXProcessor& audioProcessor;
    vibe::LookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };

    juce::ComboBox presetBox;
    juce::TextButton previousButton { "<" }, nextButton { ">" }, bypassButton { "BYPASS" };
    juce::Label presetDescription;
    std::unique_ptr<vibe::APVTS::ButtonAttachment> bypassAttachment;

    vibe::MeterBridge meters;
    vibe::VisualiserPanel visualiser;
    std::unique_ptr<vibe::ChainView> chain;
    std::unique_ptr<vibe::PlayerBar> playerBar;
    std::vector<std::unique_ptr<vibe::EffectCard>> cards;

    juce::Rectangle<int> headerArea;
    int shownProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeFXEditor)
};
