#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace zikada {

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
        juce::StringArray{"1/8", "1/4", "1/2"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParameterIDs::bypass, "Bypass", false));

    return {params.begin(), params.end()};
}

}
