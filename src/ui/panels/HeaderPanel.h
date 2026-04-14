#pragma once

#include "../ZikadaLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

namespace zikada {

class HeaderPanel : public juce::Component
{
public:
    enum class Page
    {
        Sequencer = 0,
        Presets,
        Settings
    };

    HeaderPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSelectedPage(Page page);
    void setUndoEnabled(bool enabled);
    void setRedoEnabled(bool enabled);
    void setPresetDisplay(const juce::String& presetName, const juce::String& presetMeta, bool dirty);
    void setPresetStepEnabled(bool previousEnabled, bool nextEnabled);

    std::function<void(Page)> onPageSelected;
    std::function<void()> onUndoRequested;
    std::function<void()> onRedoRequested;
    std::function<void()> onPresetPreviousRequested;
    std::function<void()> onPresetNextRequested;
    std::function<void()> onPresetBrowserRequested;

private:
    juce::Image logoImage;
    std::unique_ptr<juce::Drawable> undoIcon;
    std::unique_ptr<juce::Drawable> redoIcon;
    juce::TextButton undoButton{"UNDO"};
    juce::TextButton redoButton{"REDO"};
    juce::TextButton sequencerTab{"SEQUENCER"};
    juce::TextButton presetsTab{"PRESETS"};
    juce::TextButton settingsTab{"SETTINGS"};
    juce::TextButton presetSelectButton;
    juce::TextButton presetPrevButton{"<"};
    juce::TextButton presetNextButton{">"};
    juce::Label presetMetaLabel;
    Page selectedPage{Page::Sequencer};
    juce::String currentPresetName{"CURRENT STATE"};
    juce::String currentPresetMeta{"UNSAVED SNAPSHOT"};
    bool presetDirty{false};
};

}
