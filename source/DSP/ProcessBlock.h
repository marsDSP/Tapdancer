#pragma once

#include "Smoother.h"
#include "Delay/Lagrange5th.h"

namespace MarsDSP::DSP {

        class ProcessBlock
    {
    public:

        ProcessBlock() = default;
        ~ProcessBlock() = default;

        void prepare (double sample_Rate, int samples_Per_Block, int num_Channels, int factor)
        {
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate = sample_Rate;
            spec.maximumBlockSize = samples_Per_Block;
            spec.numChannels = num_Channels;

            m_spec = spec; // keep a copy for smoothers/rehydration

            m_oversample = std::make_unique<juce::dsp::Oversampling<float>>(spec.numChannels,
                factor, juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR, true);

            m_oversample->initProcessing(spec.maximumBlockSize);

            lagrange.prepare(spec);

            // Only prepare/reset smoothers here if they were already created.
            // Normally, smoothers are constructed lazily in updateParams().
            if (smoothers)
            {
                smoothers->prepare(spec);
                smoothers->reset();

                lagrange.setSmoothers(smoothers.get());
            }

        }

        void process(juce::AudioBuffer<float>& buffer, const int num_samples)
        {
            const int num_oversampled_samples = num_samples * static_cast<int>(m_oversample->getOversamplingFactor());
            juce::dsp::AudioBlock<float> block (buffer);
            auto up_sampled_block = m_oversample->processSamplesUp(block);
            lagrange.processBlock(up_sampled_block, num_oversampled_samples);
            m_oversample->processSamplesDown(block);
        }

        void updateParams(const Parameters& parameters)
        {
            // Lazily construct and attach smoothers to the delay processor
            if (!smoothers)
            {
                smoothers = std::make_unique<Smoothers>(parameters);
                if (m_spec.sampleRate > 0.0)
                {
                    smoothers->prepare(m_spec);
                    smoothers->reset();
                }
                lagrange.setSmoothers(smoothers.get());
            }

            // Update smoother targets from the latest parameters
            smoothers->update();
        }

    private:

        Lagrange5th<float> lagrange;

        std::unique_ptr<juce::dsp::Oversampling<float>> m_oversample;
        juce::dsp::ProcessSpec m_spec {};
        std::unique_ptr<Smoothers> smoothers;
    };
}
