#pragma once

#include <Includes.h>
#include "Parameters.h"

namespace MarsDSP
{
    // class designed to provide smooth interpolation between parameter changes..
    class Smoothers
    {
    public:
        explicit Smoothers(const Parameters& p) : params(p) {} // reading parameter values, never writing them

        void prepare(juce::dsp::ProcessSpec& spec) noexcept
        {
            constexpr double duration = 0.02;
            const int steps = static_cast<int>(spec.sampleRate * duration);
            auto resetAll = [steps](auto& smootherArray)
            {
                for (auto& smoother : smootherArray)
                    smoother.reset(steps);
            };

            resetAll(gainSmoother);
            resetAll(mixSmoother);
            resetAll(delayTimeSmoother);
            resetAll(feedbackSmoother);
            resetAll(wideSmoother);
            resetAll(hpfSmoother);
            resetAll(lpfSmoother);

            // After 200 ms (0.2f), the 1-pole filter will have approached the target value to within 63.2%.
            // The formula below describes the charge time of an analog capacitor.
            // It is common to use these representations in DSP code, as a great deal of DSP is based on EE.
            // Using a 1-pole here for an exponential curve to avoid zipper noise since it fits better in context.

            //coeff = 1.0f - std::exp(-1.0f / (0.2f * static_cast<float>(spec.sampleRate)));
        }

        void reset() noexcept
        {
            // drive = 0.0f;
            // for (auto& smoother : driveSmoother)
            //     smoother.setCurrentAndTargetValue(params.consoleDriveParam->get() * 0.01f); // percent -> 0..1

            gain = 0.0f;
            const auto gainVal = juce::Decibels::decibelsToGain(params.gainInDb->get());
            for (auto& smoother : gainSmoother)
                smoother.setCurrentAndTargetValue(gainVal);

            delayTime = 0.0f;
            for (auto& smoother : mixSmoother)
                smoother.setCurrentAndTargetValue(params.mixPercent->get()); // milliseconds

            mix = 1.0f;
            for (auto& smoother : delayTimeSmoother)
                smoother.setCurrentAndTargetValue(params.delayTimeMs->get() * 0.01f);

            feedback = 0.0f;
            for (auto& smoother : feedbackSmoother)
                smoother.setCurrentAndTargetValue(params.feedbackHz->get() * 0.01f);

            // values for stereo param range. stereo is not a panning parameter.
            // stereo widens the sound, but the knob itself functions _like_ a panner.
            panL = 0.0f;
            panR = 1.0f;

            wide = 0.0f;
            for (auto& smoother : wideSmoother)
                smoother.setCurrentAndTargetValue(params.widePercent->get() * 0.01f); // -1..1

            highcut = 20000.0f;
            for (auto& smoother : hpfSmoother)
                smoother.setCurrentAndTargetValue(params.hpfHz->get()); // Hz

            lowcut = 0.0f;
            for (auto& smoother : lpfSmoother)
                smoother.setCurrentAndTargetValue(params.lpfHz->get()); // Hz
        }

        // Update the smoother's target when the parameter changes; the actual
        // gain value will be read via smoothen() per-sample to avoid zipper noise.
        void update() noexcept
        {
            //const float consoleDrive = params.consoleDriveParam->get();
            //const float newConsoleDrive = consoleDrive * 0.01f; // percent -> 0..1
            //for (auto& smoother : driveSmoother)
            //    smoother.setTargetValue(newConsoleDrive);

            const float gainDB = params.gainInDb->get();
            const float newGainDB = juce::Decibels::decibelsToGain(gainDB);
            for (auto& smoother : gainSmoother)
                smoother.setTargetValue(newGainDB);

            const float newMixPercent = params.mixPercent->get() * 0.01f; // 0..1
            for (auto& smoother : mixSmoother)
                smoother.setTargetValue(newMixPercent);

            const float newDelayMs = params.delayTimeMs->get();
            for (auto& smoother : delayTimeSmoother)
                smoother.setTargetValue(newDelayMs);

            const float newFeedback = params.feedbackHz->get() * 0.01f; // -1..1
            for (auto& smoother : feedbackSmoother)
                smoother.setTargetValue(newFeedback);

            const float newStereoWidth = params.widePercent->get() * 0.01f; // -1..1
            for (auto& smoother : wideSmoother)
                smoother.setTargetValue(newStereoWidth);

            const float newHPFHz = params.hpfHz->get(); // Hz
            for (auto& smoother : hpfSmoother)
                smoother.setTargetValue(newHPFHz);

            const float newLPFHz = params.lpfHz->get(); // Hz
            for (auto& smoother : lpfSmoother)
                smoother.setTargetValue(newLPFHz);

            bypassed = params.bypass->get();
            oversample = params.oversamplingChoice->getIndex();
            delayNote = params.noteInterval->getIndex();
            tempoSync = params.sync->get();

            targetDelayTime = params.delayTimeMs->get();

            if (delayTime == 0.0f)
            {
                delayTime = targetDelayTime;
            }
        }

        static void EqualPowerPan(const float panLR, float& L, float& R)
        {
            const float x = 0.7853981633974483f * (panLR + 1.0f);
            L = std::cos(x);
            R = std::sin(x);
        }

        void smoothen() noexcept
        {
            auto smoothen = [](auto& smootherArray)
            {
                for (auto& smoother : smootherArray)
                    smoother.getNextValue();
            };

            smoothen(gainSmoother);
            smoothen(mixSmoother);
            smoothen(delayTimeSmoother);
            smoothen(feedbackSmoother);
            smoothen(wideSmoother);
            smoothen(hpfSmoother);
            smoothen(lpfSmoother);

            // Compute equal-power pan from the smoothed stereo parameter (0..1 -> -1..1).
            // We only need channel[0] for pan, so we'll call .getNextValue() directly.
            // We'll save a few cycles this way, since panning is inherently mono.

            const float pan = wideSmoother[0].getNextValue();
            EqualPowerPan(pan, panL, panR);

            delayTime = targetDelayTime;
        }

        std::vector<std::array<juce::LinearSmoothedValue<float>, 2>*> getSmoothers() noexcept
        {
            return { &gainSmoother,
                     &mixSmoother,
                     &feedbackSmoother,
                     &wideSmoother,
                     &hpfSmoother,
                     &lpfSmoother,
                     &delayTimeSmoother, };
        }

        enum class SmootherUpdateMode
        {
            initialize,
            liveInRealTime
        };

        void setSmoothers(int numSamplesToSkip, SmootherUpdateMode init) noexcept
        {
            juce::ignoreUnused(init);

            auto skipArray = [numSamplesToSkip](auto& smootherArray)
            {
                for (auto& s : smootherArray)
                    s.skip(numSamplesToSkip);
            };

            skipArray(gainSmoother);
            skipArray(mixSmoother);
            skipArray(delayTimeSmoother);
            skipArray(feedbackSmoother);
            skipArray(wideSmoother);
            skipArray(hpfSmoother);
            skipArray(lpfSmoother);
;
        }

        // Lightweight getters for consumers that need per-sample smoothed values
        // without calling smoothen() for the whole set at once.
        // These return the next smoothed value for the requested channel (0 = L, 1 = R).
        float getNextDelayTimeMs(size_t channel = 0) noexcept { return delayTimeSmoother[channel].getNextValue(); }
        float getNextMix(size_t channel = 0) noexcept         { return mixSmoother[channel].getNextValue(); }
        float getNextFeedback(size_t channel = 0) noexcept    { return feedbackSmoother[channel].getNextValue(); }

    private:

        // we don't need to copy, just reference
        const Parameters& params;

        // float drive     { 0.0f };
        float gain      { 0.0f };
        float mix       { 0.0f };
        float delayTime { 0.0f };
        float panL      { 0.0f };
        float panR      { 1.0f };
        float feedback  { 0.0f };
        float wide      { 0.0f };
        float highcut   { 20000.0f };
        float lowcut    { 0.0f };

        float targetDelayTime { 0.0f };
        float coeff { 0.0f };

        int delayNote { 0 };
        int oversample { 0 };

        bool tempoSync { false };
        bool bypassed { false };

        std::array<juce::LinearSmoothedValue<float>, 2>

        gainSmoother,
        mixSmoother,
        delayTimeSmoother,
        feedbackSmoother,
        wideSmoother,
        hpfSmoother,
        lpfSmoother;
    };
}
