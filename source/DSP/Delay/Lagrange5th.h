/*
==============================================================================

   ClassID:       Lagrange5th
   Type:          Delay line circular buffer
   Vendor:        MarsDSP
   Version:       1.0.0
   Namespace:     MarsDSP::DSP
   Description:   Uses Lagrange 5th Interpolation
   Dependencies:  DelayInterpolation.h, ProcessBlock.h, Smoothers.h
   License:       MIT License

==============================================================================
*/

#pragma once

#include <Includes.h>
#include <Globals.h>
#include "Interpolator.h"

namespace MarsDSP::DSP
{
    // Delay class using Lagrange 5th Interpolation.
    // This is an incredible sounding delay.
    template <typename SampleType>
    class Lagrange5th
    {
    public:

        Lagrange5th() = default;

        void setMaximumDelayInSamples(int maxLengthInSamples) noexcept

        {
            jassert(maxLengthInSamples > 0);

            // Need at least 6 taps beyond the integer part for 5th-order Lagrange
            int paddedLength = maxLengthInSamples + 8;

            if (bufferLength < paddedLength)
            {
                bufferLength = paddedLength;
                bufferL.reset(new SampleType[static_cast<size_t>(bufferLength)]);
                bufferR.reset(new SampleType[static_cast<size_t>(bufferLength)]);
            }
        }

        void prepare (juce::dsp::ProcessSpec& spec)
        {
            sampleRate = spec.sampleRate;

            const double numSamples = maxDelayTime / 1000.0 * sampleRate;
            const int maxDelayInSamples = static_cast<int>(std::ceil(numSamples));

            // Reserve internal buffer storage for custom interpolated delay
            setMaximumDelayInSamples(maxDelayInSamples);
            reset();

            // init
            feedbackL = 0.0f;
            feedbackR = 0.0f;

            delayInSamples = 0.0f;
            targetDelay = 0.0f;
            xfade = 0.0f;
            xfadeInc = static_cast<float>(1.0 / (0.05 * sampleRate)); // 50 ms

            fade = 1.0f;
            fadeTarget = 1.0f;

            coeff = 1.0f - std::exp(-1.0f / (0.05f * static_cast<float> (sampleRate)));

            wait = 0.0f;
            waitInc = 1.0f / (0.3f * static_cast<float> (sampleRate)); // 300 ms

            // If an external smoother is attached, prepare/reset it here
            if (smoothersPtr != nullptr)
            {
                smoothersPtr->prepare(spec);
                smoothersPtr->reset();
            }
        }

        // process
        void processBlock(juce::dsp::AudioBlock<SampleType>& block, const int num_samples)
        {
            const size_t numCh = block.getNumChannels();

            auto* ch0 = numCh > 0 ? block.getChannelPointer(0) : nullptr;
            auto* ch1 = numCh > 1 ? block.getChannelPointer(1) : nullptr;

            // If we have external smoothers attached, assume caller updated their targets this block
            // and pull smoothed values per-sample; otherwise cache current parameters once per block.
            const bool useSmoothers = (smoothersPtr != nullptr);

            const float dMsParam = juce::jlimit(minDelayTime, maxDelayTime, delayTime);
            const SampleType dSampParam = static_cast<SampleType>(sampleRate * (dMsParam * 0.001f));
            const SampleType mixParamBlock = static_cast<SampleType>(juce::jlimit(0.0f, 1.0f, mix));
            const SampleType fbLParamBlock = static_cast<SampleType>(juce::jlimit(0.0f, 0.99f, feedbackL));
            const SampleType fbRParamBlock = static_cast<SampleType>(juce::jlimit(0.0f, 0.99f, feedbackR));

            if (monoMode)
            {
                for (size_t n = 0; n < static_cast<size_t>(num_samples); ++n)
                {
                    SampleType dSamp = dSampParam;
                    SampleType mixParam = mixParamBlock;
                    SampleType fbParam = fbLParamBlock;

                    if (useSmoothers)
                    {
                        // mono uses left channel (0)
                        const float dMs = smoothersPtr->getNextDelayTimeMs(0);
                        dSamp = static_cast<SampleType>(sampleRate * (dMs * 0.001f));
                        mixParam = static_cast<SampleType>(juce::jlimit(0.0f, 1.0f, smoothersPtr->getNextMix(0)));
                        fbParam = static_cast<SampleType>(juce::jlimit(0.0f, 0.99f, smoothersPtr->getNextFeedback(0)));
                    }
                    SampleType x = ch0 != nullptr ? ch0[n] : static_cast<SampleType>(0);
                    if (ch1 != nullptr)
                        x = static_cast<SampleType>(0.5) * (x + ch1[n]); // sum to mono

                    // read delayed sample BEFORE writing for proper feedback
                    const int lastIdxL = (writeIndexL - 1 + bufferLength) % bufferLength;
                    const SampleType y = readInterpolated(bufferL.get(), lastIdxL, dSamp, lagrange5thL);

                    // write input + feedback
                    const SampleType writeVal = x + fbParam * y;
                    writeSample(bufferL.get(), writeIndexL, writeVal);

                    const SampleType out = y * mixParam
                                          + x * (static_cast<SampleType>(1) - mixParam);
                    if (ch0 != nullptr) ch0[n] = out;
                    if (ch1 != nullptr) ch1[n] = out;
                }
            }
            else // stereo
            {
                for (size_t n = 0; n < static_cast<size_t>(num_samples); ++n)
                {
                    SampleType dSampL = dSampParam;
                    SampleType dSampR = dSampParam;
                    SampleType mixParam = mixParamBlock;
                    SampleType fbLParam = fbLParamBlock;
                    SampleType fbRParam = fbRParamBlock;

                    if (useSmoothers)
                    {
                        const float dMsL = smoothersPtr->getNextDelayTimeMs(0);
                        const float dMsR = smoothersPtr->getNextDelayTimeMs(1);
                        dSampL = static_cast<SampleType>(sampleRate * (dMsL * 0.001f));
                        dSampR = static_cast<SampleType>(sampleRate * (dMsR * 0.001f));
                        mixParam = static_cast<SampleType>(juce::jlimit(0.0f, 1.0f, smoothersPtr->getNextMix(0)));
                        fbLParam = static_cast<SampleType>(juce::jlimit(0.0f, 0.99f, smoothersPtr->getNextFeedback(0)));
                        fbRParam = static_cast<SampleType>(juce::jlimit(0.0f, 0.99f, smoothersPtr->getNextFeedback(1)));
                    }

                    if (ch0 != nullptr)
                    {
                        const SampleType xL = ch0[n];
                        const int lastIdxL = (writeIndexL - 1 + bufferLength) % bufferLength;
                        const SampleType yL = readInterpolated(bufferL.get(), lastIdxL, dSampL, lagrange5thL);
                        const SampleType writeValL = xL + fbLParam * yL;
                        writeSample(bufferL.get(), writeIndexL, writeValL);
                        ch0[n] = yL * mixParam
                               + xL * (static_cast<SampleType>(1) - mixParam);
                    }

                    if (ch1 != nullptr)
                    {
                        const SampleType xR = ch1[n];
                        const int lastIdxR = (writeIndexR - 1 + bufferLength) % bufferLength;
                        const SampleType yR = readInterpolated(bufferR.get(), lastIdxR, dSampR, lagrange5thR);
                        const SampleType writeValR = xR + fbRParam * yR;
                        writeSample(bufferR.get(), writeIndexR, writeValR);
                        ch1[n] = yR * mixParam
                               + xR * (static_cast<SampleType>(1) - mixParam);
                    }
                }
            }
        }

        void reset() noexcept
        {
            writeIndexL = 0;
            writeIndexR = 0;

            if (bufferL)
            {
                for (size_t i = 0; i < static_cast<size_t>(bufferLength); ++i)
                    bufferL[i] = static_cast<SampleType>(0);
            }
            if (bufferR)
            {
                for (size_t i = 0; i < static_cast<size_t>(bufferLength); ++i)
                    bufferR[i] = static_cast<SampleType>(0);
            }
        }

        // Helper: write a single sample into a ring buffer and advance the write index
        void writeSample(SampleType* buf, int& idx, SampleType input) noexcept
        {
            jassert(bufferLength > 0);
            // write at current index, then advance
            buf[static_cast<size_t>(idx)] = input;
            idx += 1;
            if (idx >= bufferLength)
                idx = 0;
        }

        // Helper: read an interpolated delayed sample using 5th-order Lagrange
        SampleType readInterpolated(const SampleType* buf, int idx, SampleType delaySamples,
                                    DelayLineInterpolationTypes::Lagrange5th& interp) const noexcept
        {
            if (bufferLength <= 8)
                return static_cast<SampleType>(0);

            // Clamp delay to valid range
            const SampleType maxDelay = static_cast<SampleType>(bufferLength - 8);
            SampleType d = juce::jlimit(static_cast<SampleType>(0), maxDelay, delaySamples);

            int dInt = static_cast<int>(std::floor(static_cast<double>(d)));
            SampleType dFrac = d - static_cast<SampleType>(dInt);

            // Adjust for 5th-order lagrange internal alignment
            int offset = dInt;
            interp.updateInternalVariables(offset, dFrac);

            // Starting index for 6-sample window (with wrap-around)
            int start = idx - offset;
            while (start < 0) start += bufferLength;
            if (start >= bufferLength) start %= bufferLength;

            // Copy 6 consecutive samples with wrap-around into a small local buffer
            SampleType window[6];
            for (int k = 0; k < 6; ++k)
            {
                int pos = start + k;
                if (pos >= bufferLength) pos -= bufferLength;
                window[k] = buf[static_cast<size_t>(pos)];
            }

            // Call interpolator with local window; delayInt is zero because window starts at the base index
            return interp.call<SampleType, SampleType, SampleType>(window, 0, dFrac);
        }

        void setMono(const bool shouldBeMono) noexcept
        {
            monoMode = shouldBeMono;
        }

        [[nodiscard]] bool isMono() const noexcept
        {
            return monoMode;
        }

        // Parameter setters (used when no external smoothing is attached)
        void setDelayTime(float milliseconds) noexcept
        {
            delayTime = milliseconds;
        }

        void setMix(float value) noexcept
        {
            // Accept 0..1 or 0..100 input; normalise to 0..1
            mix = (value > 1.0f ? value * 0.01f : value);
        }

        void getFeedbackValues(float value) noexcept
        {
            // Clamp to a safe range to avoid instability
            const float fb = juce::jlimit(0.0f, 0.99f, value);
            feedbackL = fb;
            feedbackR = fb;
        }

        // Attach external smoothers that will provide per-sample smoothed parameter values
        void setSmoothers(Smoothers* s) noexcept
        {
            smoothersPtr = s;
        }

    private:

        double sampleRate = 44100.0;

        float delayTime = 50.0f; // milliseconds
        float fade = 0.0f;
        float fadeTarget = 0.0f;
        float coeff = 0.0f;
        float wait = 0.0f;
        float waitInc = 0.0f;

        float delayInSamples = 0.0f;
        float targetDelay = 0.0f;
        float xfade = 0.0f;
        float xfadeInc = 0.0f;

        float feedbackL = 0.0f;
        float feedbackR = 0.0f;

        float mix { 1.0f }; // 0..1 wet level

        Smoothers* smoothersPtr { nullptr };

        // Custom 5th-order Lagrange interpolators (from DelayInterpolation.h)
        // to be used for left/right channels by the delay processor
        DelayLineInterpolationTypes::Lagrange5th lagrange5thL, lagrange5thR;
        std::unique_ptr<SampleType[]> bufferL, bufferR;

        int bufferLength = 0;
        int writeIndexL = 0; // where the most recent value was written (left)
        int writeIndexR = 0; // where the most recent value was written (right)

        bool monoMode = false; // if true, force mono processing
    };
}
