#pragma once

#include "ChronosProcessor.h"

//==============================================================================
class ChronosEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ChronosEditor (ChronosProcessor&);
    ~ChronosEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ChronosProcessor& pref;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChronosEditor)
};
