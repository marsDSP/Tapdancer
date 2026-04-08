#include "ChronosProcessor.h"
#include "ChronosEditor.h"

//==============================================================================
ChronosEditor::ChronosEditor (ChronosProcessor& p)
    : AudioProcessorEditor (&p), pref (p)
{
    juce::ignoreUnused (pref);
    setSize (400, 300);
}

ChronosEditor::~ChronosEditor()
{
}

//==============================================================================
void ChronosEditor::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void ChronosEditor::resized()
{
}
