#pragma once

#include <JuceHeader.h>

namespace vibe::colours
{
    const juce::Colour background { 0xff0e1016 };
    const juce::Colour panel { 0xff161a23 };
    const juce::Colour panelRaised { 0xff1e2331 };
    const juce::Colour border { 0xff282e3d };
    const juce::Colour track { 0xff2b3142 };
    const juce::Colour text { 0xffe7e9ef };
    const juce::Colour muted { 0xff8a91a5 };
    const juce::Colour accent { 0xff7c5cff };
    const juce::Colour accent2 { 0xff38d6e0 };
    const juce::Colour danger { 0xffff5c7a };
}

namespace vibe
{
    inline juce::Font font (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel()
        {
            using namespace colours;
            setColour (juce::ResizableWindow::backgroundColourId, background);
            setColour (juce::Slider::textBoxTextColourId, text);
            setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.4f));
            setColour (juce::Slider::backgroundColourId, track);
            setColour (juce::Slider::trackColourId, accent);
            setColour (juce::Slider::thumbColourId, text);
            setColour (juce::Label::textColourId, text);
            setColour (juce::ComboBox::backgroundColourId, panelRaised);
            setColour (juce::ComboBox::outlineColourId, border);
            setColour (juce::ComboBox::textColourId, text);
            setColour (juce::ComboBox::arrowColourId, muted);
            setColour (juce::PopupMenu::backgroundColourId, panelRaised);
            setColour (juce::PopupMenu::textColourId, text);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.35f));
            setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
            setColour (juce::TextButton::buttonColourId, panelRaised);
            setColour (juce::TextButton::buttonOnColourId, accent);
            setColour (juce::TextButton::textColourOffId, text);
            setColour (juce::TextButton::textColourOnId, juce::Colours::white);
            setColour (juce::ToggleButton::textColourId, text);
            setColour (juce::ToggleButton::tickColourId, accent);
            setColour (juce::TextEditor::backgroundColourId, panelRaised);
            setColour (juce::TextEditor::textColourId, text);
            setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.4f));
            setColour (juce::CaretComponent::caretColourId, text);
            setColour (juce::AlertWindow::backgroundColourId, panelRaised);
            setColour (juce::AlertWindow::textColourId, text);
        }

        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float position,
                               float startAngle, float endAngle, juce::Slider& slider) override
        {
            const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
            const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
            const auto centre = bounds.getCentre();
            const auto lineWidth = juce::jmax (2.5f, radius * 0.15f);
            const auto arcRadius = radius - lineWidth * 0.5f;
            const auto angle = startAngle + position * (endAngle - startAngle);
            const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
            const juce::PathStrokeType stroke (lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

            juce::Path background;
            background.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
            g.setColour (colours::track);
            g.strokePath (background, stroke);

            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);
            g.setColour (accent);
            g.strokePath (value, stroke);

            const auto knobRadius = arcRadius - lineWidth * 1.3f;
            g.setColour (colours::panelRaised);
            g.fillEllipse (juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre));
            g.setColour (colours::border);
            g.drawEllipse (juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre), 1.0f);

            g.setColour (colours::text);
            g.drawLine ({ centre.getPointOnCircumference (knobRadius * 0.3f, angle),
                          centre.getPointOnCircumference (knobRadius * 0.85f, angle) }, 2.0f);
        }

        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                               float, float, juce::Slider::SliderStyle, juce::Slider& slider) override
        {
            const auto bar = juce::Rectangle<float> ((float) x, (float) y + (float) height * 0.5f - 3.0f, (float) width, 6.0f);
            g.setColour (colours::track);
            g.fillRoundedRectangle (bar, 3.0f);
            g.setColour (slider.findColour (juce::Slider::trackColourId));
            g.fillRoundedRectangle (bar.withRight (sliderPos), 3.0f);
            g.setColour (colours::text);
            g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ sliderPos, bar.getCentreY() }));
        }

        void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool) override
        {
            const auto bounds = button.getLocalBounds().toFloat();
            const auto height = juce::jmin (18.0f, bounds.getHeight() - 2.0f);
            const auto pillWidth = height * 1.8f;
            const bool hasText = button.getButtonText().isNotEmpty();
            const auto pill = hasText ? juce::Rectangle<float> (bounds.getX() + 1.0f, bounds.getCentreY() - height * 0.5f, pillWidth, height)
                                      : juce::Rectangle<float> (pillWidth, height).withCentre (bounds.getCentre());
            const bool on = button.getToggleState();
            const auto accent = button.findColour (juce::ToggleButton::tickColourId);

            g.setColour (on ? accent : (highlighted ? colours::track.brighter (0.15f) : colours::track));
            g.fillRoundedRectangle (pill, height * 0.5f);

            const auto thumb = juce::Rectangle<float> (height - 5.0f, height - 5.0f)
                                   .withCentre ({ on ? pill.getRight() - height * 0.5f : pill.getX() + height * 0.5f, pill.getCentreY() });
            g.setColour (on ? juce::Colours::white : colours::muted);
            g.fillEllipse (thumb);

            if (hasText)
            {
                g.setColour (colours::text);
                g.setFont (font (12.5f));
                g.drawText (button.getButtonText(), bounds.withLeft (pill.getRight() + 7.0f), juce::Justification::centredLeft);
            }
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& background,
                                   bool highlighted, bool down) override
        {
            const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
            const bool on = button.getToggleState();
            auto fill = on ? button.findColour (juce::TextButton::buttonOnColourId) : background;

            if (highlighted) fill = fill.brighter (0.08f);
            if (down) fill = fill.brighter (0.15f);

            g.setColour (fill);
            g.fillRoundedRectangle (bounds, 7.0f);
            g.setColour (on ? fill.brighter (0.3f) : colours::border);
            g.drawRoundedRectangle (bounds, 7.0f, 1.0f);
        }

        juce::Font getTextButtonFont (juce::TextButton&, int height) override
        {
            return font (juce::jmin (13.5f, (float) height * 0.45f), true);
        }

        juce::Font getComboBoxFont (juce::ComboBox&) override { return font (14.0f, true); }
        juce::Font getPopupMenuFont() override { return font (14.0f); }

        juce::Label* createSliderTextBox (juce::Slider& slider) override
        {
            auto* label = LookAndFeel_V4::createSliderTextBox (slider);
            label->setFont (font (11.5f));
            return label;
        }
    };
}
