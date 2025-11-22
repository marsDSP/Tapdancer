#pragma once
#include <Includes.h>

namespace MarsDSP {

        inline const juce::ParameterID gainParamID { "gainInDb", 1 };
        static constexpr const char* gainParamIDName = "Gain";

        inline const juce::ParameterID mixParamID { "mixPercent", 1 };
        static constexpr const char* mixParamIDName = "Mix";

        inline const juce::ParameterID feedbackParamID { "feedbackHz", 1 };
        static constexpr const char* feedbackParamIDName = "Feedback";

        inline const juce::ParameterID hpfParamID { "hpfHz", 1 };
        static constexpr const char* hpfParamIDName = "HPF";

        inline const juce::ParameterID lpfParamID { "lpfHz", 1 };
        static constexpr const char* lpfParamIDName = "LPF";

        inline const juce::ParameterID delayTimeParamID { "delayTimeMs", 1 };
        static constexpr const char* delayTimeParamIDName = "Delay Time";

        inline const juce::ParameterID noteIntervalParamID { "noteInterval", 1 };
        static constexpr const char* noteIntervalParamIDName = "Interval";

        inline const juce::ParameterID syncParamID { "sync", 1 };
        static constexpr const char* syncParamIDName = "Sync";

        inline const juce::ParameterID bypassParamID { "bypass", 1 };
        static constexpr const char* bypassParamIDName = "Bypass";
        inline bool isBypassed = false;

        inline const juce::StringArray sync_notes
        {
            "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
            "1/1 D", "1/2 D", "1/4 D", "1/8 D", "1/16 D",
            "1/1 T", "1/2 T", "1/4 T", "1/8 T", "1/16 T"
        };

        static constexpr float minDelayTime = 5.0f;
        static constexpr float maxDelayTime = 5000.0f;
}