#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor &p) : AudioProcessorEditor(&p), pref(p)
{
    juce::ignoreUnused(pref);
    setSize (900, 450);
}

PluginEditor::~PluginEditor() = default;

void PluginEditor::paint(juce::Graphics &g)
{
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRect(getLocalBounds());
}

void PluginEditor::resized()
{
    const auto sync_padding = juce::roundToInt(getWidth() * 0.03f);
    const auto sync_width = juce::roundToInt(getWidth() * 0.07f);
    const auto sync_height = juce::roundToInt(getHeight() * 0.05f);
    const auto sync_x_pos = getWidth() - sync_width - sync_padding;
    const auto sync_y_pos = sync_padding;
    sync.setBounds(sync_x_pos, sync_y_pos, sync_width, sync_height);
}

void PluginEditor::setSyncMenu(juce::ComboBox& syncBox, const juce::StringArray& items)
{
    syncBox.addItemList(items, 1);
    syncBox.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(syncBox);
}