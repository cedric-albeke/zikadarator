#include "state/PluginState.h"

namespace zikada {

PluginState::PluginState(juce::AudioProcessor& processor)
    : apvts(processor, nullptr, JucePlugin_Name, createParameterLayout())
{
}

}
