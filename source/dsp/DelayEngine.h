#pragma once

#ifndef TAPDANCER_DELAYENGINE_H
#define TAPDANCER_DELAYENGINE_H

template <typename SampleType>
class DelayEngine {
public:
    DelayEngine() = default;

    void setMaximumDelayInSamples (int maxLengthInSamples) noexcept {
        jassert (maxLengthInSamples > 0.0);
        int paddedLength = maxLengthInSamples + 8;
        if (bufferLength < paddedLength) {
            bufferLength = paddedLength;
            bufferL.reset(new sampleType[static_cast<size_t>(bufferLength)]);
            bufferR.reset(new sampleType[static_cast<size_t>(bufferLength)]);
        }
    }
private:
};

#endif