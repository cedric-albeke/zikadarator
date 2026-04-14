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
        bool isFavorite{false};
        int recentRank{-1};
        juce::File file;
        juce::ValueTree state;
    };

    PresetManager();

    void refresh();
    const std::vector<PresetItem>& getItems() const { return items; }

    bool saveUserPreset(const juce::String& name, const juce::ValueTree& state);
    bool loadPreset(int index, juce::ValueTree& outState) const;
    bool deleteUserPreset(int index);
    bool toggleFavorite(const juce::String& name);
    bool isFavorite(const juce::String& name) const;
    void markPresetUsed(const juce::String& name);

private:
    juce::File presetDirectory;
    juce::File metadataFile;
    std::vector<PresetItem> items;
    juce::StringArray favoritePresetNames;
    juce::StringArray recentPresetNames;

    void addFactoryPresets();
    void addUserPresets();
    void loadMetadata();
    void saveMetadata() const;
    void sortItems();

    static juce::ValueTree createBaseState();
    static juce::ValueTree createInitFactoryState();
    static juce::ValueTree createNeonGateFactoryState();
    static juce::ValueTree createSpaceBloomFactoryState();
};

} // namespace zikada
