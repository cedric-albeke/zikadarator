#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace zikada {

inline const juce::Identifier stateSchemaVersionProperty{"zikadaStateSchemaVersion"};
inline constexpr int currentStateSchemaVersion = 2;
inline const juce::Identifier parameterStateTreeType{"PARAM"};
inline const juce::Identifier parameterStateIdProperty{"id"};
inline const juce::Identifier parameterStateValueProperty{"value"};

class ParameterIDs
{
public:
    static inline const juce::String dryWet{"dryWet"};
    static inline const juce::String outputGain{"outputGain"};
    static inline const juce::String mixMode{"mixMode"};
    static inline const juce::String clockSource{"clockSource"};
    static inline const juce::String tempo{"tempo"};
    static inline const juce::String stepResolution{"stepResolution"};
    static inline const juce::String bypass{"bypass"};
};

inline juce::String getStepActiveID(int lane, int step)
{
    return "stepActive_L" + juce::String(lane) + "_S" + juce::String(step);
}

inline juce::String getLaneMixID(int lane)
{
    return "laneMix_L" + juce::String(lane);
}

inline juce::String getLaneMuteID(int lane)
{
    return "laneMute_L" + juce::String(lane);
}

inline juce::String getLaneSoloID(int lane)
{
    return "laneSolo_L" + juce::String(lane);
}

inline juce::ValueTree findParameterState(const juce::ValueTree& root, const juce::String& parameterID)
{
    for (const auto& child : root)
        if (child.hasType(parameterStateTreeType)
            && child.getProperty(parameterStateIdProperty).toString() == parameterID)
            return child;

    return {};
}

inline void setParameterStateValue(juce::ValueTree& root,
                                   const juce::String& parameterID,
                                   float value)
{
    auto parameterState = findParameterState(root, parameterID);
    if (!parameterState.isValid())
    {
        parameterState = juce::ValueTree(parameterStateTreeType);
        parameterState.setProperty(parameterStateIdProperty, parameterID, nullptr);
        root.addChild(parameterState, -1, nullptr);
    }

    parameterState.setProperty(parameterStateValueProperty, value, nullptr);
}

inline float getParameterStateValue(const juce::ValueTree& root,
                                    const juce::String& parameterID,
                                    float fallback = 0.0f)
{
    const auto parameterState = findParameterState(root, parameterID);
    return parameterState.isValid()
        ? static_cast<float>(parameterState.getProperty(parameterStateValueProperty, fallback))
        : fallback;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::dryWet, "Dry/Wet", 0.0f, 100.0f, 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::outputGain, "Output Gain", -24.0f, 24.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterIDs::mixMode, "Mix Mode",
        juce::StringArray{"Linear", "Ducking", "Sidechain", "Multiply", "Screen", "Difference"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterIDs::clockSource, "Clock Source",
        juce::StringArray{"Host", "Free"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::tempo, "Tempo", 20.0f, 300.0f, 120.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterIDs::stepResolution, "Step Resolution",
        juce::StringArray{"1/16", "1/8", "1/4", "1/2"}, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParameterIDs::bypass, "Bypass", false));

    for (int lane = 0; lane < 6; ++lane)
    {
        for (int step = 0; step < 16; ++step)
        {
            auto id = getStepActiveID(lane, step);
            auto name = "Step L" + juce::String(lane + 1) + " S" + juce::String(step + 1);
            params.push_back(std::make_unique<juce::AudioParameterBool>(id, name, false));
        }
    }

    for (int lane = 0; lane < 6; ++lane)
    {
        auto id = getLaneMixID(lane);
        auto name = "Mix L" + juce::String(lane + 1);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id, name, 0.0f, 100.0f, 100.0f));
    }

    for (int lane = 0; lane < 6; ++lane)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            getLaneMuteID(lane), "Mute L" + juce::String(lane + 1), false));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            getLaneSoloID(lane), "Solo L" + juce::String(lane + 1), false));
    }

    return {params.begin(), params.end()};
}

}
