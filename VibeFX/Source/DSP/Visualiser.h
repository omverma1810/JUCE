#pragma once

#include <JuceHeader.h>

namespace vibe
{
    // Lock-free single-producer (audio thread) / single-consumer (UI timer) sample feed
    class AnalyserFifo
    {
    public:
        AnalyserFifo() : buffer ((size_t) capacity, 0.0f) {}

        void push (const juce::AudioBuffer<float>& source)
        {
            const auto numSamples = source.getNumSamples();
            const auto numChannels = juce::jmin (2, source.getNumChannels());

            if (numChannels == 0)
                return;

            int start1, size1, start2, size2;
            fifo.prepareToWrite (numSamples, start1, size1, start2, size2);

            auto mono = [&] (int i)
            {
                float sum = 0.0f;
                for (int ch = 0; ch < numChannels; ++ch)
                    sum += source.getSample (ch, i);
                return sum / (float) numChannels;
            };

            for (int i = 0; i < size1; ++i) buffer[(size_t) (start1 + i)] = mono (i);
            for (int i = 0; i < size2; ++i) buffer[(size_t) (start2 + i)] = mono (size1 + i);

            fifo.finishedWrite (size1 + size2);
        }

        int pull (float* dest, int maxSamples)
        {
            int start1, size1, start2, size2;
            fifo.prepareToRead (maxSamples, start1, size1, start2, size2);

            std::copy_n (buffer.begin() + start1, size1, dest);
            std::copy_n (buffer.begin() + start2, size2, dest + size1);

            fifo.finishedRead (size1 + size2);
            return size1 + size2;
        }

    private:
        static constexpr int capacity = 32768;
        juce::AbstractFifo fifo { capacity };
        std::vector<float> buffer;
    };

    //==============================================================================
    // Peak levels written by the audio thread, collected (and reset) by the UI
    struct LevelMeterSource
    {
        void update (const juce::AudioBuffer<float>& buffer)
        {
            const auto numChannels = buffer.getNumChannels();

            for (int ch = 0; ch < 2; ++ch)
            {
                if (numChannels == 0)
                    break;

                const auto level = buffer.getMagnitude (juce::jmin (ch, numChannels - 1), 0, buffer.getNumSamples());

                if (level > peak[ch].load())
                    peak[ch].store (level);
            }
        }

        float collect (int channel) { return peak[channel].exchange (0.0f); }

        std::atomic<float> peak[2] { 0.0f, 0.0f };
    };
}
