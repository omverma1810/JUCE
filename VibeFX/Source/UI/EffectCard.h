#pragma once

#include "LookAndFeel.h"

namespace vibe
{
    using APVTS = juce::AudioProcessorValueTreeState;

    // Rotary knob with its name above and value below
    class Knob : public juce::Component
    {
    public:
        Knob (APVTS& state, const juce::String& paramId, const juce::String& name, juce::Colour accent)
            : attachment (state, paramId, slider)
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
            slider.setColour (juce::Slider::rotarySliderFillColourId, accent);

            if (auto* parameter = state.getParameter (paramId))
                slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

            addAndMakeVisible (slider);

            label.setText (name, juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);
            label.setFont (font (11.0f, true));
            label.setColour (juce::Label::textColourId, colours::muted);
            label.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (label);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            label.setBounds (bounds.removeFromTop (14));
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, bounds.getWidth(), 16);
            slider.setBounds (bounds);
        }

    private:
        juce::Slider slider;
        juce::Label label;
        APVTS::SliderAttachment attachment;
    };

    //==============================================================================
    // One effect module: header with power switch, then knobs and optional extra controls
    class EffectCard : public juce::Component
    {
    public:
        EffectCard (APVTS& s, juce::String cardTitle, juce::String cardSubtitle, juce::Colour colour, const char* onParamId)
            : state (s), title (std::move (cardTitle)), subtitle (std::move (cardSubtitle)), accent (colour)
        {
            setOpaque (true);

            if (onParamId != nullptr)
            {
                onParameter = state.getParameter (onParamId);
                power.setColour (juce::ToggleButton::tickColourId, accent);
                power.setTooltip ("Turn " + title + " on or off");
                addAndMakeVisible (power);
                powerAttachment = std::make_unique<APVTS::ButtonAttachment> (state, onParamId, power);
            }
        }

        void addKnob (const char* paramId, const juce::String& name)
        {
            knobs.push_back (std::make_unique<Knob> (state, paramId, name, accent));
            addAndMakeVisible (*knobs.back());
        }

        void addToggle (const char* paramId, const juce::String& text)
        {
            auto toggle = std::make_unique<juce::ToggleButton> (text);
            toggle->setColour (juce::ToggleButton::tickColourId, accent);
            buttonAttachments.push_back (std::make_unique<APVTS::ButtonAttachment> (state, paramId, *toggle));
            addAndMakeVisible (*toggle);
            extras.push_back (std::move (toggle));
        }

        void addChoice (const char* paramId)
        {
            auto box = std::make_unique<juce::ComboBox>();

            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId)))
                box->addItemList (choice->choices, 1);

            comboAttachments.push_back (std::make_unique<APVTS::ComboBoxAttachment> (state, paramId, *box));
            addAndMakeVisible (*box);
            extras.push_back (std::move (box));
        }

        void refresh()
        {
            const bool on = onParameter == nullptr || onParameter->getValue() > 0.5f;

            if (on == lastOn)
                return;

            lastOn = on;

            for (auto& knob : knobs)
                knob->setAlpha (on ? 1.0f : 0.4f);

            for (auto& extra : extras)
                extra->setAlpha (on ? 1.0f : 0.4f);

            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (colours::background);
            auto bounds = getLocalBounds().toFloat();

            g.setColour (colours::panel);
            g.fillRoundedRectangle (bounds, 10.0f);

            if (lastOn && onParameter != nullptr)
            {
                g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.14f), 0.0f, 0.0f,
                                                         accent.withAlpha (0.0f), 0.0f, bounds.getHeight() * 0.6f, false));
                g.fillRoundedRectangle (bounds, 10.0f);
            }

            g.setColour (lastOn ? accent.withAlpha (0.6f) : colours::border);
            g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);

            g.setColour (accent.withAlpha (lastOn ? 1.0f : 0.35f));
            g.fillRoundedRectangle (bounds.removeFromTop (3.0f).reduced (14.0f, 0.0f), 1.5f);

            auto text = titleArea;
            g.setColour (lastOn ? colours::text : colours::muted);
            g.setFont (font (13.5f, true));
            g.drawText (title, text.removeFromTop (18.0f), juce::Justification::centredLeft);

            g.setColour (colours::muted);
            g.setFont (font (10.5f));
            g.drawText (subtitle, text, juce::Justification::centredLeft, true);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (12, 9);
            auto header = bounds.removeFromTop (36);

            if (onParameter != nullptr)
                power.setBounds (header.removeFromRight (40).withSizeKeepingCentre (40, 22));

            titleArea = header.toFloat().withTrimmedTop (2.0f);

            if (! extras.empty())
            {
                auto row = bounds.removeFromBottom (24);
                const auto width = row.getWidth() / (int) extras.size();

                for (auto& extra : extras)
                    extra->setBounds (row.removeFromLeft (width).reduced (2, 0));

                bounds.removeFromBottom (4);
            }

            const auto count = (int) knobs.size();

            if (count == 0)
                return;

            const auto columns = count <= 3 ? count : (count == 4 ? 2 : 3);
            const auto rows = (count + columns - 1) / columns;
            const auto cellWidth = bounds.getWidth() / columns;
            const auto cellHeight = bounds.getHeight() / rows;

            for (int i = 0; i < count; ++i)
                knobs[(size_t) i]->setBounds (bounds.getX() + (i % columns) * cellWidth,
                                              bounds.getY() + (i / columns) * cellHeight,
                                              cellWidth, cellHeight);
        }

    private:
        APVTS& state;
        juce::String title, subtitle;
        juce::Colour accent;
        juce::RangedAudioParameter* onParameter = nullptr;
        juce::Rectangle<float> titleArea;
        bool lastOn = true;

        juce::ToggleButton power;
        std::unique_ptr<APVTS::ButtonAttachment> powerAttachment;
        std::vector<std::unique_ptr<Knob>> knobs;
        std::vector<std::unique_ptr<juce::Component>> extras;
        std::vector<std::unique_ptr<APVTS::ButtonAttachment>> buttonAttachments;
        std::vector<std::unique_ptr<APVTS::ComboBoxAttachment>> comboAttachments;
    };
}
