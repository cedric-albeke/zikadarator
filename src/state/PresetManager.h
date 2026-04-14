#pragma once

#include "ParameterIDs.h"
#include "SequencerState.h"

#include <vector>

namespace zikada {

class PresetManager
{
public:
    struct PresetItem
    {
        juce::String name;
        juce::String category;
        juce::String subtitle;
        bool isFactory{false};
        juce::File file;
        juce::ValueTree state;
    };

    PresetManager();

    void refresh();
    const std::vector<PresetItem>& getItems() const { return items; }

    bool saveUserPreset(const juce::String& name, const juce::ValueTree& state);
    bool loadPreset(int index, juce::ValueTree& outState) const;
    bool deleteUserPreset(int index);

private:
    juce::File presetDirectory;
    std::vector<PresetItem> items;

    void addFactoryPresets();
    void addUserPresets();

    static juce::ValueTree createBaseState();
    static juce::ValueTree createInitFactoryState();
    static juce::ValueTree createNeonGateFactoryState();
    static juce::ValueTree createSpaceBloomFactoryState();
};

} // namespace zikada
