#pragma once

#ifndef TAPDANCER_DELAYENGINE_H
#define TAPDANCER_DELAYENGINE_H

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Interpolator.h"

template <typename SampleType>
class DelayEngine {
public:
    DelayEngine() = default;

    void setMaximumDelayInSamples (int maxLengthInSamples) noexcept {
        jassert (maxLengthInSamples > 0.0);
        int paddedLength = maxLengthInSamples + 8;
        if (bufferLength < paddedLength) {
            bufferLength = paddedLength;
            bufferL.reset(new SampleType[static_cast<size_t>(bufferLength)]);
            bufferR.reset(new SampleType[static_cast<size_t>(bufferLength)]);
        }
    }

    void prepareDelay (juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;
        const double numSamples = maxDelayTime / 1000.0 * sampleRate;
        const int maxDelayInSamples = static_cast<int>(std::ceil(numSamples));

        // reserves internal buffer storage for custom interpolated delay
        setMaximumDelayInSamples(maxDelayInSamples);
        reset();

        // smoothed parameters — 50 ms ramp for mix/feedback, 50 ms for delay
        const double rampSeconds = 0.05;
        smoothedDelay.reset(sampleRate, rampSeconds);
        smoothedMix.reset(sampleRate, rampSeconds);
        smoothedFbL.reset(sampleRate, rampSeconds);
        smoothedFbR.reset(sampleRate, rampSeconds);

        smoothedDelay.setCurrentAndTargetValue(static_cast<SampleType>(0));
        smoothedMix.setCurrentAndTargetValue(static_cast<SampleType>(1));
        smoothedFbL.setCurrentAndTargetValue(static_cast<SampleType>(0));
        smoothedFbR.setCurrentAndTargetValue(static_cast<SampleType>(0));

        // modulation ducking: reduces wet/feedback level when delay time moves quickly
        prevDelaySamples = smoothedDelay.getCurrentValue();
        duckGain = static_cast<SampleType>(1);
        duckAttackCoeff  = calcOnePoleCoeffSeconds(0.005);
        duckReleaseCoeff = calcOnePoleCoeffSeconds(0.050);
    }

    void processDelay (juce::dsp::AudioBlock<SampleType>& block, const int numSmp) {
        if (bypassed) return;

        const size_t numCh = block.getNumChannels();
        auto *ch0 = numCh > 0 ? block.getChannelPointer(0) : nullptr;
        auto *ch1 = numCh > 1 ? block.getChannelPointer(1) : nullptr;

        // set smoothed targets for this block
        const float dMsParam = juce::jlimit(minDelayTime, maxDelayTime, delayTime);
        smoothedDelay.setTargetValue(static_cast<SampleType>(sampleRate * (dMsParam * 0.001f)));
        smoothedMix.setTargetValue(static_cast<SampleType>(juce::jlimit(0.0f, 1.0f, mix)));
        smoothedFbL.setTargetValue(static_cast<SampleType>(juce::jlimit(0.0f, 0.99f, feedbackL)));
        smoothedFbR.setTargetValue(static_cast<SampleType>(juce::jlimit(0.0f, 0.99f, feedbackR)));

        if (monoMode) {
            for (auto n {0uz}; n < static_cast<size_t>(numSmp); ++n) {
                const SampleType dSmp    = smoothedDelay.getNextValue();
                const SampleType mixParam = smoothedMix.getNextValue();
                const SampleType fbParam  = smoothedFbL.getNextValue();

                const SampleType modSpeed = std::abs(dSmp - prevDelaySamples);
                prevDelaySamples = dSmp;
                updateDuckGain(modSpeed);

                SampleType x = ch0 != nullptr ? ch0[n] : static_cast<SampleType>(0);
                if (ch1 != nullptr)
                    x = static_cast<SampleType>(0.5) * (x + ch1[n]);

                // read before write
                const int lastIndexL = (writeIndexL - 1 + bufferLength) % bufferLength;
                const SampleType y = readInterpolated(bufferL.get(), lastIndexL, dSmp, lagrange5thL);

                const SampleType yDucked = y * duckGain;

                // write (input + fb), soft-clip to tame loud sweeps
                const SampleType writeValue = softClip(x + fbParam * yDucked);
                writeSample(bufferL.get(), writeIndexL, writeValue);

                const SampleType out = softClip(yDucked * mixParam + x * (static_cast<SampleType>(1) - mixParam));
                if (ch0 != nullptr) ch0[n] = out;
                if (ch1 != nullptr) ch1[n] = out;
            }
        }
        else {
            for (auto n {0uz}; n < static_cast<size_t>(numSmp); ++n) {
                const SampleType dSmp    = smoothedDelay.getNextValue();
                const SampleType mixParam = smoothedMix.getNextValue();
                const SampleType fbLParam = smoothedFbL.getNextValue();
                const SampleType fbRParam = smoothedFbR.getNextValue();

                const SampleType modSpeed = std::abs(dSmp - prevDelaySamples);
                prevDelaySamples = dSmp;
                updateDuckGain(modSpeed);

                if (ch0 != nullptr) {
                    const SampleType xL = ch0[n];
                    const int lastIndexL = (writeIndexL - 1 + bufferLength) % bufferLength;
                    const SampleType yL = readInterpolated(bufferL.get(), lastIndexL, dSmp, lagrange5thL);
                    const SampleType yLDucked = yL * duckGain;
                    const SampleType writeValueL = softClip(xL + fbLParam * yLDucked);
                    writeSample(bufferL.get(), writeIndexL, writeValueL);
                    ch0[n] = softClip(yLDucked * mixParam + xL * (static_cast<SampleType>(1) - mixParam));
                }

                if (ch1 != nullptr) {
                    const SampleType xR = ch1[n];
                    const int lastIndexR = (writeIndexR - 1 + bufferLength) % bufferLength;
                    const SampleType yR = readInterpolated(bufferR.get(), lastIndexR, dSmp, lagrange5thR);
                    const SampleType yRDucked = yR * duckGain;
                    const SampleType writeValueR = softClip(xR + fbRParam * yRDucked);
                    writeSample(bufferR.get(), writeIndexR, writeValueR);
                    ch1[n] = softClip(yRDucked * mixParam
                           + xR * (static_cast<SampleType>(1) - mixParam));
                }
            }
        }
    }

    SampleType calcOnePoleCoeffSeconds(double seconds) const noexcept
    {
        if (seconds <= 0.0)
            return static_cast<SampleType>(1);

        const double denom = seconds * sampleRate;
        if (denom <= 0.0)
            return static_cast<SampleType>(1);

        return static_cast<SampleType>(1.0 - std::exp(-1.0 / denom));
    }

    void updateDuckGain(SampleType modSpeedSamplesPerSample) noexcept
    {
        // modulation ducking:
        // lower floor = deeper attenuation,
        // higher sensitivity = sooner ducking
        constexpr SampleType minDuckLocal = static_cast<SampleType>(0.08);
        constexpr SampleType sensitivityLocal = static_cast<SampleType>(0.20);

        const SampleType desired = juce::jlimit(minDuckLocal,
                                               static_cast<SampleType>(1),
                                               static_cast<SampleType>(1)
                                                   / (static_cast<SampleType>(1)
                                                      + sensitivityLocal * modSpeedSamplesPerSample));

        const SampleType coeff = (desired < duckGain ? duckAttackCoeff : duckReleaseCoeff);
        duckGain += coeff * (desired - duckGain);
    }

    void reset() noexcept {
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

    static SampleType softClip(SampleType x) noexcept
    {
        // tanh-style soft clipper: tames peaks while preserving character
        return std::tanh(x);
    }

    void writeSample(SampleType* buf, int& idx, SampleType input) noexcept
    {
        jassert(bufferLength > 0);
        // write at current index, then advance
        buf[static_cast<size_t>(idx)] = input;
        idx += 1;
        if (idx >= bufferLength)
            idx = 0;
    }

    SampleType readInterpolated(const SampleType* buf, int idx, SampleType delaySamples,
                            Lagrange5th& interp) const noexcept
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

    void setBypassed(const bool shouldBypass) noexcept
    {
        bypassed = shouldBypass;
    }

    [[nodiscard]] bool isBypassed() const noexcept
    {
        return bypassed;
    }

    // Parameter setters — values are smoothed per-sample inside processDelay
    void setDelayTime(float milliseconds) noexcept
    {
        delayTime = milliseconds;
    }

    void setMix(float value) noexcept
    {
        // Accept 0..1 or 0..100 input; normalise to 0..1
        mix = (value > 1.0f ? value * 0.01f : value);
    }

    void setFeedbackValues(float value) noexcept
    {
        // Clamp to a safe range to avoid instability
        const float fb = juce::jlimit(0.0f, 0.99f, value);
        feedbackL = fb;
        feedbackR = fb;
    }

private:

    static constexpr float minDelayTime = 5.0f;
    static constexpr float maxDelayTime = 5000.0f;

    double sampleRate = 44100.0;
    float delayTime = 50.0f;
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
    float mix { 1.0f };

    // per-sample smoothing
    juce::SmoothedValue<SampleType> smoothedDelay;
    juce::SmoothedValue<SampleType> smoothedMix;
    juce::SmoothedValue<SampleType> smoothedFbL;
    juce::SmoothedValue<SampleType> smoothedFbR;

    // modulation ducking state
    SampleType prevDelaySamples = static_cast<SampleType>(0);
    SampleType duckGain = static_cast<SampleType>(1);
    SampleType duckAttackCoeff = static_cast<SampleType>(1);
    SampleType duckReleaseCoeff = static_cast<SampleType>(1);

    Lagrange5th lagrange5thL, lagrange5thR;
    std::unique_ptr<SampleType[]> bufferL, bufferR;

    int bufferLength = 0;
    int writeIndexL = 0;
    int writeIndexR = 0;

    bool monoMode = false;
    bool bypassed = false;
};

#endif