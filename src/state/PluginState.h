#pragma once

#include "ParameterIDs.h"

namespace zikada {

class PluginState
{
public:
    PluginState(juce::AudioProcessor& processor);

    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }
    const juce::AudioProcessorValueTreeState& getValueTreeState() const { return apvts; }

private:
    juce::AudioProcessorValueTreeState apvts;
};

}
