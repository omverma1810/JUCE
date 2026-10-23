#pragma once

#include <JuceHeader.h>

namespace vibe
{
    // Built-in audio file player used by the standalone app for demos.
    // Playing at a different speed changes tempo and pitch together, like real "slowed" edits.
    class FilePlayer
    {
    public:
        FilePlayer() { formats.registerBasicFormats(); }

        struct RenderResult
        {
            bool active = false;            // a file is loaded, so the player replaces the input
            bool startedFromTop = false;    // playback (re)started at the beginning this block
            double secondsRemaining = 1.0e9;
        };

        // Message thread
        bool load (const juce::File& file, juce::String& error)
        {
            std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

            if (reader == nullptr)
            {
                error = "This file type isn't supported.";
                return false;
            }

            const auto maxSamples = (juce::int64) (reader->sampleRate * 60.0 * 15.0);
            const auto numSamples = (int) juce::jmin (reader->lengthInSamples, maxSamples);
            juce::AudioBuffer<float> newData ((int) juce::jmin (2u, reader->numChannels), numSamples);
            reader->read (&newData, 0, numSamples, 0, true, true);

            playing = false;

            {
                const juce::SpinLock::ScopedLockType sl (lock);
                std::swap (data, newData);
                fileRate = reader->sampleRate;
                readPos = 0.0;
            }

            lengthSeconds = numSamples / reader->sampleRate;
            positionSeconds = 0.0;
            fileName = file.getFileName();
            loaded = true;
            return true;
        }

        void play()
        {
            if (! loaded)
                return;

            if (positionSeconds.load() <= 0.0)
                seekRequest = 0.0;

            playing = true;
        }

        void pause() { playing = false; }
        void stop() { playing = false; seekRequest = 0.0; positionSeconds = 0.0; }
        void seek (double seconds) { seekRequest = juce::jlimit (0.0, lengthSeconds.load(), seconds); }

        bool hasFile() const { return loaded; }
        bool isPlaying() const { return playing; }
        double getPosition() const { return positionSeconds; }
        double getLength() const { return lengthSeconds; }
        juce::String getFileName() const { return fileName; }

        std::atomic<bool> looping { true };

        // Audio thread
        void prepare (double sampleRate) { deviceRate = sampleRate; }

        RenderResult render (juce::AudioBuffer<float>& out, double speed)
        {
            RenderResult result;
            const juce::SpinLock::ScopedTryLockType sl (lock);

            if (! sl.isLocked() || ! loaded)
                return result;

            result.active = true;
            out.clear();

            if (auto seekTo = seekRequest.exchange (-1.0); seekTo >= 0.0)
            {
                readPos = seekTo * fileRate;
                result.startedFromTop = seekTo == 0.0;
            }

            const auto length = data.getNumSamples();

            if (! playing || length < 4)
                return result;

            const auto increment = fileRate / deviceRate * speed;
            const auto numFileChannels = data.getNumChannels();

            for (int i = 0; i < out.getNumSamples(); ++i)
            {
                if (readPos >= length - 1)
                {
                    if (! looping)
                    {
                        playing = false;
                        readPos = 0.0;
                        break;
                    }

                    readPos = 0.0;
                    result.startedFromTop = true;
                }

                const auto index = (int) readPos;
                const auto t = (float) (readPos - index);

                for (int ch = 0; ch < out.getNumChannels(); ++ch)
                {
                    const auto* src = data.getReadPointer (juce::jmin (ch, numFileChannels - 1));
                    auto at = [&] (int k) { return src[juce::jlimit (0, length - 1, k)]; };
                    out.setSample (ch, i, hermite (at (index - 1), at (index), at (index + 1), at (index + 2), t));
                }

                readPos += increment;
            }

            positionSeconds = readPos / fileRate;
            result.secondsRemaining = (length - readPos) / increment / deviceRate;
            return result;
        }

    private:
        static float hermite (float xm1, float x0, float x1, float x2, float t)
        {
            const auto c = (x1 - xm1) * 0.5f;
            const auto v = x0 - x1;
            const auto w = c + v;
            const auto a = w + v + (x2 - x0) * 0.5f;
            const auto b = w + a;
            return ((a * t - b) * t + c) * t + x0;
        }

        juce::AudioFormatManager formats;
        juce::SpinLock lock;
        juce::AudioBuffer<float> data;
        double fileRate = 44100.0, deviceRate = 44100.0, readPos = 0.0;
        std::atomic<bool> loaded { false }, playing { false };
        std::atomic<double> seekRequest { -1.0 }, positionSeconds { 0.0 }, lengthSeconds { 0.0 };
        juce::String fileName;
    };
}
