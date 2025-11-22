#pragma once

#include "PluginProcessor.h"
#include "Globals.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    PluginProcessor &pref;
    juce::ComboBox sync;
    void setSyncMenu(juce::ComboBox& box, const juce::StringArray& sync_notes);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};