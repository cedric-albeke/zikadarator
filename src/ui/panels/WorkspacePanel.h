#pragma once

#include "../ZikadaLookAndFeel.h"
#include "../../state/PresetManager.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

#include <functional>
#include <memory>
#include <vector>

namespace zikada {

class WorkspacePanel : public juce::Component,
                       private juce::ListBoxModel
{
public:
    enum class Mode
    {
        Presets = 0,
        Settings
    };

    WorkspacePanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setMode(Mode newMode);
    void setPresetItems(std::vector<PresetManager::PresetItem> newItems);
    void bindToParameters(juce::AudioProcessorValueTreeState& apvts);

    std::function<void(const juce::String&)> onSavePreset;
    std::function<void(int)> onLoadPreset;
    std::function<void(int)> onDeletePreset;
    std::function<void(int)> onToggleFavoritePreset;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    enum class PresetSourceFilter
    {
        All = 0,
        Factory,
        User,
        Favorites
    };

    Mode mode{Mode::Presets};
    PresetSourceFilter presetSourceFilter{PresetSourceFilter::All};

    juce::Label titleLabel;
    juce::Label bodyLabel;

    juce::Label presetHintLabel;
    juce::Label presetDetailTitle;
    juce::Label presetDetailMeta;
    juce::Label presetDetailBody;
    juce::Label presetSaveLabel;
    juce::Label presetInfoTitleA;
    juce::Label presetInfoBodyA;
    juce::Label presetInfoTitleB;
    juce::Label presetInfoBodyB;

    juce::TextEditor presetSearchEditor;
    juce::ComboBox presetCategoryBox;
    juce::ListBox presetList;
    juce::TextEditor presetNameEditor;
    juce::TextButton allFilterButton{"ALL"};
    juce::TextButton factoryFilterButton{"FACTORY"};
    juce::TextButton userFilterButton{"USER"};
    juce::TextButton favoriteFilterButton{"FAV"};
    juce::TextButton saveButton{"SAVE CURRENT"};
    juce::TextButton loadButton{"LOAD PRESET"};
    juce::TextButton deleteButton{"DELETE USER"};
    juce::TextButton favoritePresetButton{"STAR"};

    juce::Label settingsLeadLabel;
    juce::Label settingsDeviceTitle;
    juce::Label settingsProductTitle;
    juce::Label settingsNotesTitle;
    juce::Label settingsNotesBody;
    juce::Label standaloneMuteLabel;
    juce::Label dryWetLabel;
    juce::Label outputGainLabel;
    juce::Label tempoLabel;
    juce::Label mixModeLabel;
    juce::Label clockSourceLabel;
    juce::Label stepResolutionLabel;
    juce::ToggleButton standaloneMuteButton{"Mute audio input"};
    juce::ToggleButton bypassToggle{"Bypass audio"};
    juce::Slider dryWetSlider;
    juce::Slider outputGainSlider;
    juce::Slider tempoSlider;
    juce::ComboBox mixModeBox;
    juce::ComboBox clockSourceBox;
    juce::ComboBox stepResolutionBox;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> standaloneDeviceSelector;
    juce::Value muteInputValue;

    std::unique_ptr<SliderAttachment> dryWetAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> tempoAttachment;
    std::unique_ptr<ComboBoxAttachment> mixModeAttachment;
    std::unique_ptr<ComboBoxAttachment> clockSourceAttachment;
    std::unique_ptr<ComboBoxAttachment> stepResolutionAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    std::vector<PresetManager::PresetItem> presetItems;
    std::vector<int> filteredPresetIndices;
    int selectedPresetRow{-1};

    juce::Rectangle<int> heroZone;
    juce::Rectangle<int> presetBrowserZone;
    juce::Rectangle<int> presetDetailZone;
    juce::Rectangle<int> presetInfoZoneA;
    juce::Rectangle<int> presetInfoZoneB;
    juce::Rectangle<int> settingsDeviceZone;
    juce::Rectangle<int> settingsProductZone;
    juce::Rectangle<int> settingsNotesZone;

    void refreshCopy();
    void applyVisibility();
    void rebuildPresetFilter();
    void updatePresetDetail();
    void updatePresetInfoPanels();
    void setPresetSourceFilter(PresetSourceFilter filter);
    void rebuildStandaloneSettingsComponent();
    void styleStandaloneSettingsComponent();

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
};

} // namespace zikada
