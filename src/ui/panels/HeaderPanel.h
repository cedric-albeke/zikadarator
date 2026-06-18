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
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;

    void setSelectedPage(Page page);
    void setUndoEnabled(bool enabled);
    void setRedoEnabled(bool enabled);
    void setPresetDisplay(const juce::String& presetName, const juce::String& presetMeta, bool dirty);
    void setPresetStepEnabled(bool previousEnabled, bool nextEnabled);
    void setFxDisplayState(int lane, int step, int presetIndex, const juce::String& presetLabel);
    void setFxDisplayPlayhead(int step, int activeLaneMask);
    juce::Component* getPresetMenuTarget() { return &presetSelectButton; }

    std::function<void(Page)> onPageSelected;
    std::function<void()> onUndoRequested;
    std::function<void()> onRedoRequested;
    std::function<void()> onPresetPreviousRequested;
    std::function<void()> onPresetNextRequested;
    std::function<void()> onPresetMenuRequested;

private:
    juce::Image logoImage;
    std::unique_ptr<juce::Drawable> undoIcon;
    std::unique_ptr<juce::Drawable> redoIcon;
    std::unique_ptr<juce::Drawable> saveIcon;
    juce::TextButton undoButton{"UNDO"};
    juce::TextButton redoButton{"REDO"};
    juce::TextButton sequencerTab{"SEQUENCER"};
    juce::TextButton presetsTab{"PRESETS"};
    juce::TextButton settingsTab{"SETTINGS"};
    juce::TextButton presetSelectButton;
    juce::TextButton presetPrevButton{"<"};
    juce::TextButton presetNextButton{">"};
    juce::Label presetMetaLabel;
    juce::Rectangle<int> fxDisplayBounds;
    Page selectedPage{Page::Sequencer};
    juce::String currentPresetName{"CURRENT STATE"};
    juce::String currentPresetMeta{"UNSAVED SNAPSHOT"};
    juce::String fxDisplayLabel{"NO FX"};
    int fxDisplayLane{-1};
    int fxDisplayStep{-1};
    int fxDisplayPresetIndex{-1};
    int fxDisplayPlayheadStep{-1};
    int fxDisplayActiveLaneMask{0};
    double fxDisplayLastPulseMs{0.0};
    bool presetDirty{false};

    void drawFxCrtDisplay(juce::Graphics& g);
};

}
