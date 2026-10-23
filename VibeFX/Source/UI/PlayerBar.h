#pragma once

#include "LookAndFeel.h"
#include "../DSP/FilePlayer.h"

namespace vibe
{
    // Transport for the built-in demo player (standalone app only)
    class PlayerBar : public juce::Component, private juce::Timer
    {
    public:
        explicit PlayerBar (FilePlayer& p) : player (p)
        {
            setOpaque (true);

            for (auto* button : { &loadButton, &playButton, &stopButton })
                addAndMakeVisible (button);

            loadButton.onClick = [this] { chooseFile(); };
            playButton.onClick = [this] { player.isPlaying() ? player.pause() : player.play(); };
            stopButton.onClick = [this] { player.stop(); };
            playButton.setColour (juce::TextButton::buttonColourId, colours::accent);

            loopToggle.setToggleState (player.looping, juce::dontSendNotification);
            loopToggle.onClick = [this] { player.looping = loopToggle.getToggleState(); };
            addAndMakeVisible (loopToggle);

            position.setSliderStyle (juce::Slider::LinearHorizontal);
            position.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            position.setRange (0.0, 1.0);
            position.setColour (juce::Slider::trackColourId, colours::accent2);
            position.onDragEnd = [this] { player.seek (position.getValue() * player.getLength()); };
            addAndMakeVisible (position);

            fileLabel.setFont (font (13.0f, true));
            timeLabel.setFont (font (12.5f));
            timeLabel.setJustificationType (juce::Justification::centredRight);
            timeLabel.setColour (juce::Label::textColourId, colours::muted);
            addAndMakeVisible (fileLabel);
            addAndMakeVisible (timeLabel);

            startTimerHz (15);
            timerCallback();
        }

        void loadFile (const juce::File& file)
        {
            juce::String error;

            if (! player.load (file, error))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't open file", error);

            timerCallback();
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (colours::background);
            g.setColour (colours::panel);
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
            g.setColour (colours::border);
            g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f, 1.0f);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (10, 8);
            loadButton.setBounds (bounds.removeFromLeft (130));
            bounds.removeFromLeft (8);
            playButton.setBounds (bounds.removeFromLeft (84));
            bounds.removeFromLeft (6);
            stopButton.setBounds (bounds.removeFromLeft (70));
            bounds.removeFromLeft (10);
            loopToggle.setBounds (bounds.removeFromLeft (76));
            bounds.removeFromLeft (8);
            fileLabel.setBounds (bounds.removeFromLeft (bounds.getWidth() / 4));
            timeLabel.setBounds (bounds.removeFromRight (110));
            position.setBounds (bounds.reduced (8, 0));
        }

    private:
        static juce::String formatTime (double seconds)
        {
            const auto total = (int) seconds;
            return juce::String (total / 60) + ":" + juce::String (total % 60).paddedLeft ('0', 2);
        }

        void chooseFile()
        {
            chooser = std::make_unique<juce::FileChooser> ("Choose a song or audio file",
                                                           juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                           "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.flac;*.ogg");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      if (const auto file = fc.getResult(); file.existsAsFile())
                                          loadFile (file);
                                  });
        }

        void timerCallback() override
        {
            const bool hasFile = player.hasFile();
            playButton.setEnabled (hasFile);
            stopButton.setEnabled (hasFile);
            position.setEnabled (hasFile);
            playButton.setButtonText (player.isPlaying() ? "Pause" : "Play");

            fileLabel.setText (hasFile ? player.getFileName() : "No file loaded: drop a song here or click Load",
                               juce::dontSendNotification);
            fileLabel.setColour (juce::Label::textColourId, hasFile ? colours::text : colours::muted);

            const auto length = player.getLength();
            timeLabel.setText (hasFile ? formatTime (player.getPosition()) + " / " + formatTime (length) : juce::String(),
                               juce::dontSendNotification);

            if (! position.isMouseButtonDown() && length > 0.0)
                position.setValue (player.getPosition() / length, juce::dontSendNotification);
        }

        FilePlayer& player;
        juce::TextButton loadButton { "Load audio..." }, playButton { "Play" }, stopButton { "Stop" };
        juce::ToggleButton loopToggle { "Loop" };
        juce::Slider position;
        juce::Label fileLabel, timeLabel;
        std::unique_ptr<juce::FileChooser> chooser;
    };
}
