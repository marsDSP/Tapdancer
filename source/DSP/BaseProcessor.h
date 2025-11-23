#pragma once

#include <Includes.h>

namespace MarsDSP::DSP
{
    // abstract base class for dsp modules to inherit from
    class ProcessBase
    {
    public:
        ProcessBase() = default;

        virtual ~ProcessBase() = default;

        virtual void prepare(juce::dsp::ProcessSpec& spec)
        {
            const int steps = static_cast<int>(spec.sampleRate * 0.02);
            for (int i {}; i < s_in.size(); ++i)
            {
                s_in[i].reset(steps);
                s_out[i].reset(steps);
                s_mix[i].reset(steps);
            }
        }

        virtual void processBlock(const juce::dsp::AudioBlock<float>& block, const int num_samples)
        {
            for (size_t channel {}; channel < block.getNumChannels(); ++channel)
            {
                auto* data = block.getChannelPointer(channel);

                for (size_t sample {}; sample < num_samples; ++sample)
                {
                    const float xn_input = data[sample];
                    const float yn_output = processSample(xn_input);
                    data[sample] = yn_output;
                }
            }
        }

        virtual float processSample(float xn_input) = 0;

        std::array<juce::LinearSmoothedValue<float>, 2>& getInputs() { return s_in; }
        std::array<juce::LinearSmoothedValue<float>, 2>& getOutputs() { return s_out; }
        std::array<juce::LinearSmoothedValue<float>, 2>& getMix() { return s_mix; }

    private:

        std::array<juce::LinearSmoothedValue<float>, 2> s_in, s_out, s_mix;
    };
}