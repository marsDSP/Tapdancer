#include "Parameters.h"
#include "Converters.h"

namespace MarsDSP {

    Parameters::Parameters(juce::AudioProcessorValueTreeState& vts)
    {
        castParameter(vts, oversamplingChoiceID, oversamplingChoice);
        castParameter(vts, gainParamID, gainInDb);
        castParameter(vts, mixParamID, mixPercent);
        castParameter(vts, feedbackParamID, feedbackHz);
        castParameter(vts, wideParamID, widePercent);
        castParameter(vts, hpfParamID, hpfHz);
        castParameter(vts, lpfParamID, lpfHz);
        castParameter(vts, delayTimeParamID, delayTimeMs);
        castParameter(vts, noteIntervalParamID, noteInterval);
        castParameter(vts, syncParamID, sync);
        castParameter(vts, bypassParamID, bypass);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout Parameters::createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // oversample choice array
        layout.add(std::make_unique<juce::AudioParameterChoice>
            (oversamplingChoiceID, oversamplingChoiceName, oversample_items, 0));

        // gain.. out? need two gain stages for sure
        layout.add(std::make_unique<juce::AudioParameterFloat>
            (gainParamID, gainParamIDName, juce::NormalisableRange<float>
                {-12.0f, 12.0f}, 0.0f,
            juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(Converter::stringFromDecibels)));

        // mix percent
        layout.add(std::make_unique<juce::AudioParameterFloat>
            (mixParamID, mixParamIDName, juce::NormalisableRange<float>
                {0.0f, 100.0f, 1.0f}, 100.0f,
            juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(Converter::stringFromPercent)));

        // note interval array for when tempo sync is engaged
        layout.add(std::make_unique<juce::AudioParameterChoice>
            (noteIntervalParamID, noteIntervalParamIDName, sync_notes, 0));

        // delay time in ms and s
        layout.add(std::make_unique<juce::AudioParameterFloat>
            (delayTimeParamID, delayTimeParamIDName, juce::NormalisableRange<float>
                {minDelayTime, maxDelayTime, 0.001f, 0.25f}, 100.0f,
            juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(Converter::stringFromMilliseconds)
            .withValueFromStringFunction(Converter::millisecondsFromString)));

        // feedback percent
        layout.add(std::make_unique<juce::AudioParameterFloat>
            (feedbackParamID, feedbackParamIDName, juce::NormalisableRange<float>
                {-100.0f, 100.0f, 1.0f}, 0.0f,
            juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(Converter::stringFromPercent)));

        // stereo widener
        layout.add(std::make_unique<juce::AudioParameterFloat>
            (wideParamID, wideParamIDName, juce::NormalisableRange<float>
                {-100.0f, 100.0f, 1.0f}, 0.0f,
            juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(Converter::stringFromPercent)));

        // high-pass filter
        layout.add(std::make_unique<juce::AudioParameterFloat>
            (hpfParamID, hpfParamIDName, juce::NormalisableRange<float>
                {0.0f, 20000.0f, 1.0f, 0.3}, 20000.0f,
            juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(Converter::stringFromHz)
            .withValueFromStringFunction(Converter::hzFromString)));

        // low-pass filter
        layout.add(std::make_unique<juce::AudioParameterFloat>
            (lpfParamID, lpfParamIDName, juce::NormalisableRange<float>
                {0.0f, 20000.0f, 1.0f, 0.3}, 0.0f,
            juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(Converter::stringFromHz)
            .withValueFromStringFunction(Converter::hzFromString)));

        // tempo sync default = off
        layout.add(std::make_unique<juce::AudioParameterBool>
            (syncParamID, syncParamIDName, false));

        // bypass default = off
        layout.add(std::make_unique<juce::AudioParameterBool>
            (bypassParamID, bypassParamIDName, false));

        return layout;
    }
}

