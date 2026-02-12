#pragma once

#ifndef TAPDANCER_HADAMARDFDN_H
#define TAPDANCER_HADAMARDFDN_H

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Interpolator.h"
#include <array>
#include <cmath>
#include <memory>

template <typename SampleType, int NumLines = 4>
class DiffusionEngine {
public:
    DiffusionEngine() = default;

    static_assert(NumLines == 4, "DiffusionEngine currently implements a fixed 4x4 Hadamard diffuser");

    using Vec = std::array<SampleType, 4>;

    [[nodiscard]] static Vec hadamard4 (const Vec& x) noexcept
    {
        // 4x4 Hadamard matrix, scaled by 1/sqrt(4) = 1/2 (orthonormal)
        // H4 = [+1 +1 +1 +1]
        //      [+1 -1 +1 -1]
        //      [+1 +1 -1 -1]
        //      [+1 -1 -1 +1]
        const auto s = static_cast<SampleType> (0.5);

        return {
            s * (x[0] + x[1] + x[2] + x[3]),
            s * (x[0] - x[1] + x[2] - x[3]),
            s * (x[0] + x[1] - x[2] - x[3]),
            s * (x[0] - x[1] - x[2] + x[3])
        };
    }

    // Diffusion blend: 0 -> identity (no mixing), 1 -> full Hadamard mixing.
    [[nodiscard]] static Vec applyDiffusion (const Vec& x, SampleType diffusionAmount) noexcept
    {
        const auto d = juce::jlimit (static_cast<SampleType> (0),
                                    static_cast<SampleType> (1),
                                    diffusionAmount);

        const auto h = hadamard4 (x);
        Vec y {};
        const auto oneMinusD = static_cast<SampleType> (1) - d;
        for (size_t i = 0; i < 4; ++i)
            y[i] = oneMinusD * x[i] + d * h[i];

        return y;
    }

    void prepare(juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;

        // Pick mutually-prime delay lengths (in ms) so they don't share resonances
        constexpr std::array<float, 4> delayTimesMs = { 13.7f, 17.3f, 23.1f, 31.7f };

        for (int i = 0; i < NumLines; ++i) {
            const int maxSmp = static_cast<int>(std::ceil(sampleRate * 0.05)); // 50 ms max
            const int len = maxSmp + 8; // padding for interpolation
            bufferLength[i] = len;
            buffers[i].reset(new SampleType[static_cast<size_t>(len)]);
            std::fill_n(buffers[i].get(), len, SampleType(0));
            writeIndex[i] = 0;

            delaySamples[i] = static_cast<SampleType>(sampleRate * delayTimesMs[i] * 0.001);
        }

        feedbackGain = static_cast<SampleType>(0.5);
        diffusion = static_cast<SampleType>(1);
        std::fill(lineOutputs.begin(), lineOutputs.end(), SampleType(0));
    }

    void reset() {
        for (int i = 0; i < NumLines; ++i) {
            if (buffers[i])
                std::fill_n(buffers[i].get(), bufferLength[i], SampleType(0));
            writeIndex[i] = 0;
        }
        std::fill(lineOutputs.begin(), lineOutputs.end(), SampleType(0));
    }

    // Process one stereo pair through the FDN
    // inputL/R go in, diffuse outputL/R come out
    std::pair<SampleType, SampleType> processSample(SampleType inputL, SampleType inputR) {
        // 1. Read from all delay lines
        for (int i = 0; i < NumLines; ++i) {
            const int d = static_cast<int>(delaySamples[i]);
            const int readIdx = (writeIndex[i] - d + bufferLength[i]) % bufferLength[i];
            lineOutputs[i] = buffers[i].get()[readIdx];
        }

        // 2. Apply diffusion (Hadamard mixing blended with identity)
        const Vec x { lineOutputs[0], lineOutputs[1], lineOutputs[2], lineOutputs[3] };
        const Vec mixed = applyDiffusion (x, diffusion);

        // 3. Feed input into lines 0,1 (L) and 2,3 (R), plus feedback
        std::array<SampleType, NumLines> writeValues;
        writeValues[0] = inputL + feedbackGain * mixed[0];
        writeValues[1] = inputL + feedbackGain * mixed[1];
        writeValues[2] = inputR + feedbackGain * mixed[2];
        writeValues[3] = inputR + feedbackGain * mixed[3];

        // 4. Write into each delay line
        for (int i = 0; i < NumLines; ++i) {
            buffers[i].get()[writeIndex[i]] = writeValues[i];
            writeIndex[i] = (writeIndex[i] + 1) % bufferLength[i];
        }

        // 5. Mix down to stereo: lines 0,1 → L, lines 2,3 → R
        SampleType outL = static_cast<SampleType>(0.5) * (mixed[0] + mixed[1]);
        SampleType outR = static_cast<SampleType>(0.5) * (mixed[2] + mixed[3]);

        return { outL, outR };
    }

    void setFeedback(SampleType fb) {
        feedbackGain = juce::jlimit(SampleType(0), SampleType(0.95), fb);
    }

    void setDiffusion (SampleType amount) noexcept
    {
        diffusion = juce::jlimit (static_cast<SampleType> (0),
                                 static_cast<SampleType> (1),
                                 amount);
    }

private:
    double sampleRate = 44100.0;

    std::array<std::unique_ptr<SampleType[]>, NumLines> buffers;
    std::array<int, NumLines> bufferLength {};
    std::array<int, NumLines> writeIndex {};
    std::array<SampleType, NumLines> delaySamples {};
    std::array<SampleType, NumLines> lineOutputs {};

    SampleType feedbackGain = 0.5f;
    SampleType diffusion = static_cast<SampleType> (1);
};

#endif