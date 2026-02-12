#pragma once

#ifndef TAPDANCER_PROCESSBLOCK_H
#define TAPDANCER_PROCESSBLOCK_H

#include <JuceHeader.h>
#include "BiquadEngine.h"
#include "DelayEngine.h"

class ProcessBlock {
public:
    ProcessBlock() = default;
    ~ProcessBlock() = default;

    void prepare (double smpRate, int smpPerBlock, int numCh, int oFactor) {
        dsp::ProcessSpec spec {};
        spec.sampleRate = smpRate;
        spec.maximumBlockSize = smpPerBlock;
        spec.numChannels = numCh;

        oSmp = std::make_unique<juce::dsp::Oversampling<float>>(spec.numChannels,
            oFactor, juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR, true);

        oSmp->InitProcessing(spec.maximumBlockSize);
        biquad.prepare(spec);
        delay.prepare(spec);
    }

    void process (const AudioBuffer<float>& buffer, int numCh) {

    }

    void update (/* call parameter object virtual address here */) {

    }

private:

    BiquadEngine<float> biquad;
    DelayEngine<float> delay;
    std::unique_ptr<dsp::Oversampling<float>> oSmp;
};

#endif