#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/ZikadaLookAndFeel.h"
#include "ui/panels/HeaderPanel.h"
#include "ui/panels/SequencerPanel.h"
#include "ui/panels/FooterPanel.h"
#include "ui/panels/SidebarPanel.h"
#include "ui/panels/WorkspacePanel.h"
#include "ui/components/WaveformDisplay.h"

#include <vector>

namespace zikada {

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    WaveformDisplay* getWaveformDisplay();

private:
    enum class Page
    {
        Sequencer = 0,
        Presets,
        Settings
    };

    void timerCallback() override;
    void setPage(Page page);
    void refreshSequencerFromState();
    void refreshPresetBrowser();
    void showHeaderPresetMenu();
    void loadPresetByIndex(int index, bool pushToHistory);
    void setCurrentPresetIndex(int index, bool dirty);
    void markCurrentPresetDirty();
    void syncHeaderPresetDisplay();
    void pushUndoSnapshot();
    void applyHistoryState(const juce::ValueTree& stateTree);
    void undoLastChange();
    void redoLastChange();
    void updateHistoryButtons();

    PluginProcessor& processorRef;
    ZikadaLookAndFeel lookAndFeel;
    HeaderPanel headerPanel;
    SequencerPanel sequencerPanel;
    FooterPanel footerPanel;
    SidebarPanel sidebarPanel;
    WorkspacePanel workspacePanel;
    Page currentPage{Page::Sequencer};
    std::vector<juce::ValueTree> undoStack;
    std::vector<juce::ValueTree> redoStack;
    bool applyingHistory{false};
    int currentPresetIndex{-1};
    bool currentPresetDirty{false};
    double lastFooterHistoryMs{0.0};
    int lastFooterHistoryLane{-1};
    int lastFooterHistorySlot{-1};

    int lastPlayingStep{-1};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

}
