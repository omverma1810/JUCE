#pragma once

#include "LookAndFeel.h"
#include "../DSP/Visualiser.h"

namespace vibe
{
    // Signal-flow strip: one box per module, click to toggle, animated dots while audio flows
    class ChainView : public juce::Component, private juce::Timer
    {
    public:
        struct Node
        {
            juce::String name;
            juce::Colour colour;
            juce::RangedAudioParameter* parameter;
        };

        ChainView (std::vector<Node> chainNodes, std::function<float()> outputLevel)
            : nodes (std::move (chainNodes)), getLevel (std::move (outputLevel)), states (nodes.size(), false)
        {
            setOpaque (true);
            startTimerHz (30);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (colours::background);
            const auto boxes = getBoxes();
            const bool flowing = level > 0.001f;

            for (size_t i = 0; i + 1 < boxes.size(); ++i)
            {
                const auto from = boxes[i].getRight();
                const auto to = boxes[i + 1].getX();
                const auto y = boxes[i].getCentreY();

                g.setColour (colours::border);
                g.drawLine (from, y, to, y, 1.5f);

                if (flowing)
                {
                    const auto t = std::fmod (phase + (float) i * 0.13f, 1.0f);
                    g.setColour (colours::accent2.withAlpha (juce::jlimit (0.3f, 1.0f, level * 3.0f)));
                    g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre ({ from + (to - from) * t, y }));
                }
            }

            for (size_t i = 0; i < boxes.size(); ++i)
            {
                const bool isEndpoint = i == 0 || i == boxes.size() - 1;
                const auto box = boxes[i];

                if (isEndpoint)
                {
                    g.setColour (colours::panelRaised);
                    g.fillRoundedRectangle (box, 8.0f);
                    g.setColour (colours::border);
                    g.drawRoundedRectangle (box, 8.0f, 1.0f);
                    g.setColour (colours::text);
                    g.setFont (font (12.0f, true));
                    g.drawText (i == 0 ? "INPUT" : "OUTPUT", box, juce::Justification::centred);
                    continue;
                }

                const auto& node = nodes[i - 1];
                const bool on = node.parameter != nullptr && node.parameter->getValue() > 0.5f;

                g.setColour (on ? node.colour.withAlpha (0.22f) : colours::panel);
                g.fillRoundedRectangle (box, 8.0f);
                g.setColour (on ? node.colour : colours::border);
                g.drawRoundedRectangle (box.reduced (0.5f), 8.0f, on ? 1.5f : 1.0f);

                g.setColour (on ? node.colour : colours::track);
                g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ box.getX() + 10.0f, box.getCentreY() }));

                g.setColour (on ? colours::text : colours::muted);
                g.setFont (font (11.5f, on));
                g.drawFittedText (node.name, box.withTrimmedLeft (14.0f).toNearestInt(), juce::Justification::centred, 1);
            }
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            const auto boxes = getBoxes();

            for (size_t i = 1; i + 1 < boxes.size(); ++i)
            {
                if (auto* parameter = nodes[i - 1].parameter; parameter != nullptr && boxes[i].contains (e.position))
                {
                    parameter->beginChangeGesture();
                    parameter->setValueNotifyingHost (parameter->getValue() > 0.5f ? 0.0f : 1.0f);
                    parameter->endChangeGesture();
                    repaint();
                }
            }
        }

    private:
        std::vector<juce::Rectangle<float>> getBoxes() const
        {
            std::vector<juce::Rectangle<float>> boxes;
            const auto count = (int) nodes.size() + 2;
            const auto gap = 12.0f;
            const auto width = ((float) getWidth() - gap * (float) (count - 1)) / (float) count;

            for (int i = 0; i < count; ++i)
                boxes.emplace_back ((float) i * (width + gap), 4.0f, width, (float) getHeight() - 8.0f);

            return boxes;
        }

        // Only redraws while audio is flowing or a module was switched
        void timerCallback() override
        {
            level = juce::jmax (getLevel(), level * 0.9f);
            const bool flowing = level > 0.001f;
            bool changed = flowing != wasFlowing;

            for (size_t i = 0; i < nodes.size(); ++i)
            {
                const bool on = nodes[i].parameter != nullptr && nodes[i].parameter->getValue() > 0.5f;

                if (on != states[i])
                {
                    states[i] = on;
                    changed = true;
                }
            }

            if (flowing)
                phase = std::fmod (phase + 0.035f, 1.0f);

            if (flowing || changed)
                repaint();

            wasFlowing = flowing;
        }

        std::vector<Node> nodes;
        std::function<float()> getLevel;
        std::vector<bool> states;
        float level = 0.0f, phase = 0.0f;
        bool wasFlowing = false;
    };

    //==============================================================================
    // Real-time spectrum (original vs processed) and an oscilloscope of the output
    class VisualiserPanel : public juce::Component, private juce::Timer
    {
    public:
        VisualiserPanel (AnalyserFifo& input, AnalyserFifo& output, std::function<double()> sampleRateSource)
            : inputFifo (input), outputFifo (output), getSampleRate (std::move (sampleRateSource)),
              inputHistory ((size_t) fftSize, 0.0f), outputHistory ((size_t) fftSize, 0.0f),
              scratch ((size_t) fftSize * 2, 0.0f), fftData ((size_t) fftSize * 2, 0.0f)
        {
            inputSpectrum.fill (-120.0f);
            outputSpectrum.fill (-120.0f);
            setOpaque (true);
            startTimerHz (30);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (colours::background);
            auto bounds = getLocalBounds().toFloat();
            g.setColour (colours::panel);
            g.fillRoundedRectangle (bounds, 10.0f);
            g.setColour (colours::border);
            g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);

            auto content = bounds.reduced (14.0f, 10.0f);
            auto titles = content.removeFromTop (18.0f);
            auto scopeArea = content.removeFromRight (content.getWidth() * 0.28f);
            content.removeFromRight (14.0f);
            auto scopeTitle = titles.removeFromRight (scopeArea.getWidth());

            g.setFont (font (11.0f, true));
            g.setColour (colours::muted);
            g.drawText ("SPECTRUM", titles, juce::Justification::centredLeft);
            g.drawText ("WAVEFORM", scopeTitle, juce::Justification::centredLeft);

            // Legend
            auto legend = titles.removeFromRight (230.0f);
            g.setColour (colours::muted.withAlpha (0.7f));
            g.fillRect (legend.removeFromLeft (14.0f).withSizeKeepingCentre (14.0f, 3.0f));
            g.drawText ("  original", legend.removeFromLeft (80.0f), juce::Justification::centredLeft);
            g.setGradientFill (juce::ColourGradient (colours::accent, legend.getX(), 0.0f, colours::accent2, legend.getX() + 14.0f, 0.0f, false));
            g.fillRect (legend.removeFromLeft (14.0f).withSizeKeepingCentre (14.0f, 3.0f));
            g.setColour (colours::muted);
            g.drawText ("  with effects", legend, juce::Justification::centredLeft);

            content.removeFromTop (6.0f);
            scopeArea.removeFromTop (6.0f);
            drawSpectrum (g, content);
            drawScope (g, scopeArea);
        }

    private:
        static constexpr int fftOrder = 11;
        static constexpr int fftSize = 1 << fftOrder;
        static constexpr float minDb = -84.0f, maxDb = 6.0f;
        using Spectrum = std::array<float, fftSize / 2>;

        // Skips the FFT and the redraw entirely once playback has stopped and the display has decayed
        void timerCallback() override
        {
            const bool inputAudible = pullInto (inputFifo, inputHistory);
            const bool outputAudible = pullInto (outputFifo, outputHistory);
            const bool gotAudio = inputAudible || outputAudible;
            framesWithoutAudio = gotAudio ? 0 : framesWithoutAudio + 1;

            if (framesWithoutAudio == 10)   // ~1/3 s of nothing: playback stopped, let the display fall
            {
                std::fill (inputHistory.begin(), inputHistory.end(), 0.0f);
                std::fill (outputHistory.begin(), outputHistory.end(), 0.0f);
            }

            if (! gotAudio && isSilent)
                return;

            computeSpectrum (inputHistory, inputSpectrum);
            computeSpectrum (outputHistory, outputSpectrum);

            isSilent = framesWithoutAudio >= 10
                       && *std::max_element (inputSpectrum.begin(), inputSpectrum.end()) <= -119.0f
                       && *std::max_element (outputSpectrum.begin(), outputSpectrum.end()) <= -119.0f;
            repaint();
        }

        // Returns true if the new samples contain actual sound (the host keeps sending silence while paused)
        bool pullInto (AnalyserFifo& fifo, std::vector<float>& history)
        {
            int count;
            bool audible = false;

            while ((count = fifo.pull (scratch.data(), (int) scratch.size())) > 0)
            {
                const auto range = juce::FloatVectorOperations::findMinAndMax (scratch.data(), count);
                audible = audible || range.getStart() < -1.0e-5f || range.getEnd() > 1.0e-5f;

                if (count >= fftSize)
                {
                    std::copy (scratch.begin() + (count - fftSize), scratch.begin() + count, history.begin());
                }
                else
                {
                    std::move (history.begin() + count, history.end(), history.begin());
                    std::copy (scratch.begin(), scratch.begin() + count, history.end() - count);
                }
            }

            return audible;
        }

        void computeSpectrum (const std::vector<float>& history, Spectrum& spectrum)
        {
            std::fill (fftData.begin(), fftData.end(), 0.0f);
            std::copy (history.begin(), history.end(), fftData.begin());
            window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
            fft.performFrequencyOnlyForwardTransform (fftData.data());

            const auto scale = 4.0f / (float) fftSize;   // Hann window: full-scale sine reads 0 dB

            for (size_t i = 0; i < spectrum.size(); ++i)
            {
                const auto db = juce::Decibels::gainToDecibels (fftData[i] * scale, -120.0f);
                spectrum[i] = db > spectrum[i] ? db : spectrum[i] - 1.5f;   // fast attack, slow fall
            }
        }

        static float xForFrequency (float hz, juce::Rectangle<float> area)
        {
            return area.getX() + area.getWidth() * std::log (hz / 20.0f) / std::log (1000.0f);
        }

        static float yForDb (float db, juce::Rectangle<float> area)
        {
            return juce::jmap (juce::jlimit (minDb, maxDb, db), minDb, maxDb, area.getBottom(), area.getY());
        }

        juce::Path makeSpectrumPath (const Spectrum& spectrum, juce::Rectangle<float> area) const
        {
            juce::Path path;
            const auto binWidth = getSampleRate() / fftSize;

            for (float x = 0.0f; x <= area.getWidth(); x += 2.0f)
            {
                const auto hz = 20.0f * std::pow (1000.0f, x / area.getWidth());
                const auto bin = juce::jlimit (0.0, (double) spectrum.size() - 2.0, hz / binWidth);
                const auto index = (size_t) bin;
                const auto frac = (float) (bin - (double) index);
                auto db = spectrum[index] + frac * (spectrum[index + 1] - spectrum[index]);
                db += 3.0f * std::log2 (hz / 1000.0f);   // +3 dB/octave tilt so music looks balanced

                const auto point = juce::Point<float> (area.getX() + x, yForDb (db, area));
                if (x == 0.0f) path.startNewSubPath (point);
                else           path.lineTo (point);
            }

            return path;
        }

        void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> area) const
        {
            g.setColour (colours::background.withAlpha (0.6f));
            g.fillRoundedRectangle (area, 6.0f);

            g.setFont (font (10.0f));

            for (auto hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
            {
                const auto x = xForFrequency (hz, area);
                g.setColour (colours::border.withAlpha (0.6f));
                g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
                g.setColour (colours::muted.withAlpha (0.8f));
                g.drawText (hz >= 1000.0f ? juce::String (hz / 1000.0f, 0) + "k" : juce::String ((int) hz),
                            juce::Rectangle<float> (x + 3.0f, area.getBottom() - 14.0f, 30.0f, 12.0f), juce::Justification::left);
            }

            for (auto db : { -72.0f, -48.0f, -24.0f, 0.0f })
            {
                const auto y = yForDb (db, area);
                g.setColour (colours::border.withAlpha (0.6f));
                g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
                g.setColour (colours::muted.withAlpha (0.8f));
                g.drawText (juce::String ((int) db) + " dB", juce::Rectangle<float> (area.getX() + 4.0f, y + 1.0f, 50.0f, 12.0f), juce::Justification::left);
            }

            auto closed = [&] (juce::Path p)
            {
                p.lineTo (area.getRight(), area.getBottom());
                p.lineTo (area.getX(), area.getBottom());
                p.closeSubPath();
                return p;
            };

            const auto inputPath = makeSpectrumPath (inputSpectrum, area);
            g.setColour (colours::muted.withAlpha (0.12f));
            g.fillPath (closed (inputPath));
            g.setColour (colours::muted.withAlpha (0.55f));
            g.strokePath (inputPath, juce::PathStrokeType (1.0f));

            const auto outputPath = makeSpectrumPath (outputSpectrum, area);
            g.setGradientFill (juce::ColourGradient (colours::accent.withAlpha (0.35f), area.getX(), 0.0f,
                                                     colours::accent2.withAlpha (0.35f), area.getRight(), 0.0f, false));
            g.fillPath (closed (outputPath));
            g.setGradientFill (juce::ColourGradient (colours::accent, area.getX(), 0.0f, colours::accent2, area.getRight(), 0.0f, false));
            g.strokePath (outputPath, juce::PathStrokeType (1.8f));
        }

        void drawScope (juce::Graphics& g, juce::Rectangle<float> area) const
        {
            g.setColour (colours::background.withAlpha (0.6f));
            g.fillRoundedRectangle (area, 6.0f);
            g.setColour (colours::border.withAlpha (0.6f));
            g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());

            constexpr int shown = 1024;
            const auto size = (int) outputHistory.size();

            // Start at a rising zero crossing so the waveform holds still
            auto start = size - shown - 1;
            for (int i = size - shown - 1; i > size - shown - 512 && i > 0; --i)
            {
                if (outputHistory[(size_t) i - 1] < 0.0f && outputHistory[(size_t) i] >= 0.0f)
                {
                    start = i;
                    break;
                }
            }

            // Auto-scale so quiet passages are still readable
            float peak = 0.0f;
            for (int i = 0; i < shown; ++i)
                peak = juce::jmax (peak, std::abs (outputHistory[(size_t) (start + i)]));

            const auto scale = area.getHeight() * 0.45f / juce::jmax (0.2f, peak);

            juce::Path path;
            for (int i = 0; i < shown; ++i)
            {
                const auto sample = juce::jlimit (-1.0f, 1.0f, outputHistory[(size_t) (start + i)]);
                const juce::Point<float> point (area.getX() + area.getWidth() * (float) i / (float) (shown - 1),
                                                area.getCentreY() - sample * scale);
                if (i == 0) path.startNewSubPath (point);
                else        path.lineTo (point);
            }

            g.setColour (colours::accent2);
            g.strokePath (path, juce::PathStrokeType (1.5f));
        }

        AnalyserFifo& inputFifo;
        AnalyserFifo& outputFifo;
        std::function<double()> getSampleRate;
        std::vector<float> inputHistory, outputHistory, scratch, fftData;
        Spectrum inputSpectrum {}, outputSpectrum {};
        int framesWithoutAudio = 0;
        bool isSilent = false;
        juce::dsp::FFT fft { fftOrder };
        juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    };

    //==============================================================================
    // Input and output peak meters with peak hold
    class MeterBridge : public juce::Component, private juce::Timer
    {
    public:
        MeterBridge (LevelMeterSource& input, LevelMeterSource& output) : sources { &input, &output }
        {
            setOpaque (true);
            startTimerHz (30);
        }

        float getOutputLevel() const { return juce::jmax (levels[2], levels[3]); }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (colours::background);
            auto bounds = getLocalBounds().toFloat();
            g.setColour (colours::panel);
            g.fillRoundedRectangle (bounds, 10.0f);
            g.setColour (colours::border);
            g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);

            auto content = bounds.reduced (14.0f, 10.0f);
            g.setFont (font (11.0f, true));
            g.setColour (colours::muted);
            g.drawText ("LEVELS", content.removeFromTop (18.0f), juce::Justification::centredLeft);

            auto readout = content.removeFromBottom (32.0f);
            auto labels = content.removeFromBottom (16.0f);
            content.removeFromTop (6.0f);

            const auto groupWidth = content.getWidth() / 2.0f;
            const auto barWidth = juce::jmin (16.0f, groupWidth / 3.0f);

            for (int group = 0; group < 2; ++group)
            {
                auto area = content.withX (content.getX() + groupWidth * (float) group).withWidth (groupWidth);
                auto labelArea = labels.withX (area.getX()).withWidth (groupWidth);

                g.setColour (colours::muted);
                g.setFont (font (10.5f, true));
                g.drawText (group == 0 ? "IN" : "OUT", labelArea, juce::Justification::centred);

                for (int ch = 0; ch < 2; ++ch)
                {
                    const auto index = group * 2 + ch;
                    const auto x = area.getCentreX() + (ch == 0 ? -barWidth - 2.0f : 2.0f);
                    const juce::Rectangle<float> bar (x, area.getY(), barWidth, area.getHeight());

                    g.setColour (colours::background);
                    g.fillRoundedRectangle (bar, 3.0f);

                    const auto top = yForLevel (levels[(size_t) index], bar);
                    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34d399), 0.0f, bar.getBottom(),
                                                             juce::Colour (0xffff5c7a), 0.0f, bar.getY(), false));
                    g.fillRoundedRectangle (bar.withTop (top), 3.0f);

                    g.setColour (colours::text);
                    g.fillRect (bar.getX(), yForLevel (holds[(size_t) index], bar), barWidth, 2.0f);
                }
            }

            const auto outDb = juce::Decibels::gainToDecibels (getOutputLevel(), -60.0f);
            g.setColour (colours::text);
            g.setFont (font (15.0f, true));
            g.drawText (outDb <= -60.0f ? "-inf dB" : juce::String (outDb, 1) + " dB", readout, juce::Justification::centred);
        }

    private:
        static float yForLevel (float gain, juce::Rectangle<float> bar)
        {
            const auto db = juce::Decibels::gainToDecibels (gain, -60.0f);
            return juce::jmap (juce::jlimit (-60.0f, 6.0f, db), -60.0f, 6.0f, bar.getBottom(), bar.getY());
        }

        void timerCallback() override
        {
            for (int i = 0; i < 4; ++i)
            {
                const auto peak = sources[(size_t) (i / 2)]->collect (i % 2);
                levels[(size_t) i] = juce::jmax (peak, levels[(size_t) i] * 0.86f);

                if (levels[(size_t) i] >= holds[(size_t) i])
                {
                    holds[(size_t) i] = levels[(size_t) i];
                    holdFrames[(size_t) i] = 45;
                }
                else if (--holdFrames[(size_t) i] <= 0)
                {
                    holds[(size_t) i] *= 0.92f;
                }
            }

            // Stop redrawing once everything has fallen below -60 dB (nothing visible changes)
            const auto loudest = juce::jmax (*std::max_element (levels.begin(), levels.end()),
                                             *std::max_element (holds.begin(), holds.end()));
            const bool visible = loudest > 0.001f;

            if (visible || wasVisible)
                repaint();

            wasVisible = visible;
        }

        std::array<LevelMeterSource*, 2> sources;
        std::array<float, 4> levels {}, holds {};
        std::array<int, 4> holdFrames {};
        bool wasVisible = true;
    };
}
