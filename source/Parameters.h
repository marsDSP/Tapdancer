#pragma once

#include <Includes.h>
#include <Globals.h>

namespace MarsDSP {

    class Parameters
    {
    public:

        explicit Parameters(juce::AudioProcessorValueTreeState& vts);
        static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
        ~Parameters() = default;

        template<typename T>
        static void castParameter(juce::AudioProcessorValueTreeState& vts,
                                    const juce::ParameterID& id, T& dest)
        {
            dest = dynamic_cast<T>(vts.getParameter(id.getParamID()));
            jassert(dest);
        }

        juce::AudioParameterFloat* gainInDb { nullptr };
        juce::AudioParameterFloat* mixPercent { nullptr };
        juce::AudioParameterFloat* delayTimeMs { nullptr };
        juce::AudioParameterFloat* feedbackHz { nullptr };
        juce::AudioParameterFloat* widePercent { nullptr };
        juce::AudioParameterFloat* hpfHz { nullptr };
        juce::AudioParameterFloat* lpfHz { nullptr };

        juce::AudioParameterChoice* noteInterval { nullptr };
        juce::AudioParameterChoice* oversamplingChoice { nullptr };
        juce::AudioParameterBool* sync { nullptr };
        juce::AudioParameterBool* bypass { nullptr };

    private:

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameters)
    };
}