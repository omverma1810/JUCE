#pragma once

#include <JuceHeader.h>

namespace vibe
{
    // Reads a power-of-two ring buffer at a fractional (possibly negative) position
    inline float readLinear (const float* ring, int mask, double pos)
    {
        const auto i0 = (int) std::floor (pos);
        const auto frac = (float) (pos - i0);
        const auto a = ring[i0 & mask];
        const auto b = ring[(i0 + 1) & mask];
        return a + frac * (b - a);
    }

    //==============================================================================
    // Tape-style pitch shifter: two crossfaded read heads sweeping a short delay line.
    // ratio < 1 lowers pitch ("slowed"), ratio > 1 raises it ("sped up" / nightcore).
    class TapePitchShifter
    {
    public:
        void prepare (double sampleRate)
        {
            window = sampleRate * 0.05;
            const auto size = juce::nextPowerOfTwo ((int) (window + minDelay) + 4);
            mask = size - 1;
            ring.setSize (2, size);
            reset();
        }

        void reset()
        {
            ring.clear();
            writePos = 0;
            phase = 0.0;
        }

        void process (juce::AudioBuffer<float>& buffer, float ratio)
        {
            const auto numChannels = juce::jmin (2, buffer.getNumChannels());
            const auto numSamples = buffer.getNumSamples();
            const bool unity = std::abs (ratio - 1.0f) < 0.001f;
            const auto increment = (1.0 - ratio) / window;

            for (int i = 0; i < numSamples; ++i)
            {
                const auto phase2 = phase >= 0.5 ? phase - 0.5 : phase + 0.5;
                const auto delay1 = minDelay + phase * window;
                const auto delay2 = minDelay + phase2 * window;
                const auto s = (float) std::sin (juce::MathConstants<double>::pi * phase);
                const auto gain1 = s * s;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    auto* data = ring.getWritePointer (ch);
                    data[writePos] = buffer.getSample (ch, i);

                    // At exactly 1.0x keep the signal dry to avoid comb filtering between the heads
                    if (! unity)
                        buffer.setSample (ch, i, readLinear (data, mask, writePos - delay1) * gain1
                                               + readLinear (data, mask, writePos - delay2) * (1.0f - gain1));
                }

                writePos = (writePos + 1) & mask;

                if (! unity)
                {
                    phase += increment;
                    if (phase >= 1.0) phase -= 1.0;
                    else if (phase < 0.0) phase += 1.0;
                }
            }
        }

    private:
        static constexpr double minDelay = 2.0;
        juce::AudioBuffer<float> ring;
        double window = 2400.0, phase = 0.0;
        int writePos = 0, mask = 0;
    };

    //==============================================================================
    // Cassette / sampler character: wow & flutter, sample-rate reduction,
    // bit crushing, a darkening low-pass, tape hiss and vinyl crackle.
    class LoFi
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec)
        {
            sampleRate = spec.sampleRate;
            const auto size = juce::nextPowerOfTwo ((int) (sampleRate * 0.03));
            mask = size - 1;
            wowRing.setSize (2, size);
            tone.prepare (spec);
            tone.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            reset();
        }

        void reset()
        {
            wowRing.clear();
            tone.reset();
            writePos = 0;
            wowPhase = flutterPhase = holdAccumulator = 0.0;
            held = { 0.0f, 0.0f };
            crackle = 0.0f;
        }

        void process (juce::AudioBuffer<float>& buffer, float bits, float rateHz, float wow,
                      float noise, float toneHz, float mix)
        {
            const auto numChannels = juce::jmin (2, buffer.getNumChannels());
            const auto numSamples = buffer.getNumSamples();
            const auto levels = std::pow (2.0f, bits - 1.0f);
            const auto holdStep = juce::jmin (1.0, (double) rateHz / sampleRate);
            const auto twoPi = juce::MathConstants<double>::twoPi;

            tone.setCutoffFrequency (juce::jmin (toneHz, (float) sampleRate * 0.45f));

            for (int i = 0; i < numSamples; ++i)
            {
                wowPhase = std::fmod (wowPhase + 0.55 / sampleRate, 1.0);
                flutterPhase = std::fmod (flutterPhase + 6.0 / sampleRate, 1.0);
                const auto modMs = 3.0 + wow * (2.0 * std::sin (twoPi * wowPhase) + 0.35 * std::sin (twoPi * flutterPhase));
                const auto delay = modMs * 0.001 * sampleRate;

                holdAccumulator += holdStep;
                const bool takeNewSample = holdAccumulator >= 1.0;
                if (takeNewSample)
                    holdAccumulator -= 1.0;

                const auto hiss = (random.nextFloat() * 2.0f - 1.0f) * 0.012f * noise;

                if (random.nextFloat() < noise * 0.0005f)
                    crackle = (random.nextFloat() * 0.5f + 0.2f) * noise * (random.nextBool() ? 1.0f : -1.0f);

                const auto pop = crackle;
                crackle *= 0.55f;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const auto dry = buffer.getSample (ch, i);
                    auto* ring = wowRing.getWritePointer (ch);
                    ring[writePos] = dry;

                    auto y = readLinear (ring, mask, writePos - delay);

                    if (takeNewSample)
                        held[(size_t) ch] = y;

                    y = std::round (held[(size_t) ch] * levels) / levels;
                    y = tone.processSample (ch, y) + hiss + pop;

                    buffer.setSample (ch, i, dry + mix * (y - dry));
                }

                writePos = (writePos + 1) & mask;
            }
        }

    private:
        juce::AudioBuffer<float> wowRing;
        juce::dsp::StateVariableTPTFilter<float> tone;
        juce::Random random;
        std::array<float, 2> held {};
        double sampleRate = 44100.0, wowPhase = 0.0, flutterPhase = 0.0, holdAccumulator = 0.0;
        float crackle = 0.0f;
        int writePos = 0, mask = 0;
    };

    //==============================================================================
    // Stereo delay with darkening feedback and optional ping-pong
    class StereoEcho
    {
    public:
        void prepare (double sr)
        {
            sampleRate = sr;
            const auto size = juce::nextPowerOfTwo ((int) (sampleRate * 1.6) + 4);
            mask = size - 1;
            ring.setSize (2, size);
            delaySamples.reset (sampleRate, 0.15);
            reset();
        }

        void reset()
        {
            ring.clear();
            writePos = 0;
            lowpass = { 0.0f, 0.0f };
        }

        void process (juce::AudioBuffer<float>& buffer, float timeMs, float feedback, float toneHz, float mix, bool pingPong)
        {
            const auto numSamples = buffer.getNumSamples();
            const bool stereo = buffer.getNumChannels() >= 2;
            const auto coeff = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * toneHz / sampleRate));
            auto* ringL = ring.getWritePointer (0);
            auto* ringR = ring.getWritePointer (1);

            delaySamples.setTargetValue (timeMs * 0.001 * sampleRate);

            for (int i = 0; i < numSamples; ++i)
            {
                const auto delay = delaySamples.getNextValue();
                lowpass[0] += coeff * (readLinear (ringL, mask, writePos - delay) - lowpass[0]);
                lowpass[1] += coeff * (readLinear (ringR, mask, writePos - delay) - lowpass[1]);

                const auto inL = buffer.getSample (0, i);
                const auto inR = stereo ? buffer.getSample (1, i) : inL;

                if (pingPong && stereo)
                {
                    ringL[writePos] = (inL + inR) * 0.5f + lowpass[1] * feedback;
                    ringR[writePos] = lowpass[0] * feedback;
                }
                else
                {
                    ringL[writePos] = inL + lowpass[0] * feedback;
                    ringR[writePos] = inR + lowpass[1] * feedback;
                }

                buffer.setSample (0, i, inL + lowpass[0] * mix);
                if (stereo)
                    buffer.setSample (1, i, inR + lowpass[1] * mix);

                writePos = (writePos + 1) & mask;
            }
        }

    private:
        juce::AudioBuffer<float> ring;
        juce::SmoothedValue<double> delaySamples;
        std::array<float, 2> lowpass {};
        double sampleRate = 44100.0;
        int writePos = 0, mask = 0;
    };

    //==============================================================================
    // "8D audio": the mix is folded to mono and slowly circled around the listener
    class AutoPan8D
    {
    public:
        void prepare (double sr) { sampleRate = sr; phase = 0.0; }

        void process (juce::AudioBuffer<float>& buffer, float rateHz, float depth)
        {
            if (buffer.getNumChannels() < 2)
                return;

            auto* left = buffer.getWritePointer (0);
            auto* right = buffer.getWritePointer (1);
            const auto increment = rateHz / sampleRate;
            const auto twoPi = juce::MathConstants<double>::twoPi;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto mono = (left[i] + right[i]) * 0.5f;
                const auto pan = (float) std::sin (twoPi * phase) * depth;
                const auto angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;

                // Slightly quieter while the source passes "behind" the listener
                const auto behind = 0.5f * (1.0f - (float) std::cos (twoPi * phase));
                const auto level = juce::MathConstants<float>::sqrt2 * (1.0f - 0.25f * depth * behind);

                left[i] = mono * std::cos (angle) * level;
                right[i] = mono * std::sin (angle) * level;

                phase = std::fmod (phase + increment, 1.0);
            }
        }

    private:
        double sampleRate = 44100.0, phase = 0.0;
    };

    //==============================================================================
    inline void applyStereoWidth (juce::AudioBuffer<float>& buffer, float width)
    {
        if (buffer.getNumChannels() < 2)
            return;

        auto* left = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto mid = (left[i] + right[i]) * 0.5f;
            const auto side = (left[i] - right[i]) * 0.5f * width;
            left[i] = mid + side;
            right[i] = mid - side;
        }
    }

    inline void applySaturation (juce::AudioBuffer<float>& buffer, float amount, float mix)
    {
        const auto drive = 1.0f + amount * 20.0f;
        const auto normalise = 1.0f / std::tanh (drive);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto wet = std::tanh (data[i] * drive) * normalise;
                data[i] += mix * (wet - data[i]);
            }
        }
    }

    //==============================================================================
    // Intro (fade-in) and outro (fade-out) gain ramp
    class FadeEnvelope
    {
    public:
        void prepare (double sr)
        {
            sampleRate = sr;
            setFullyOn();
        }

        void setFullyOn()
        {
            gain = target = 1.0;
            step = 0.0;
            autoOutroArmed = true;
        }

        void startIntro (float seconds)
        {
            autoOutroArmed = true;

            if (seconds < 0.01f)
                return setFullyOn();

            gain = 0.0;
            target = 1.0;
            step = 1.0 / (seconds * sampleRate);
        }

        void startOutro (float seconds)
        {
            target = 0.0;
            step = seconds < 0.01f ? 1.0 : 1.0 / (seconds * sampleRate);
        }

        void cancelOutro()
        {
            target = 1.0;
            step = 1.0 / (0.05 * sampleRate);
        }

        void process (juce::AudioBuffer<float>& buffer)
        {
            if (gain == 1.0 && target == 1.0)
                return;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                gain = gain < target ? juce::jmin (target, gain + step) : juce::jmax (target, gain - step);

                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    buffer.getWritePointer (ch)[i] *= (float) gain;
            }
        }

        bool autoOutroArmed = true;

    private:
        double sampleRate = 44100.0, gain = 1.0, target = 1.0, step = 0.0;
    };
}
