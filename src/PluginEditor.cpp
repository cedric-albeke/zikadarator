#include "PluginEditor.h"

namespace zikada {

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      lookAndFeel(),
      headerPanel(),
      sequencerPanel(p.getPluginState().getValueTreeState()),
      footerPanel(),
      sidebarPanel(),
      workspacePanel()
{
    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(headerPanel);
    addAndMakeVisible(sequencerPanel);
    addAndMakeVisible(footerPanel);
    addAndMakeVisible(sidebarPanel);
    addAndMakeVisible(workspacePanel);
    workspacePanel.bindToParameters(processorRef.getPluginState().getValueTreeState());

    headerPanel.onPageSelected = [this](HeaderPanel::Page page)
    {
        switch (page)
        {
            case HeaderPanel::Page::Sequencer: setPage(Page::Sequencer); break;
            case HeaderPanel::Page::Presets:   setPage(Page::Presets); break;
            case HeaderPanel::Page::Settings:  setPage(Page::Settings); break;
        }
    };

    headerPanel.onUndoRequested = [this] { undoLastChange(); };
    headerPanel.onRedoRequested = [this] { redoLastChange(); };
    headerPanel.onPresetBrowserRequested = [this]
    {
        headerPanel.setSelectedPage(HeaderPanel::Page::Presets);
        setPage(Page::Presets);
    };
    headerPanel.onPresetPreviousRequested = [this]
    {
        const auto& items = processorRef.getPresetManager().getItems();
        if (items.empty())
            return;

        const int target = currentPresetIndex >= 0 ? (currentPresetIndex + static_cast<int>(items.size()) - 1) % static_cast<int>(items.size())
                                                   : static_cast<int>(items.size()) - 1;
        loadPresetByIndex(target, true);
    };
    headerPanel.onPresetNextRequested = [this]
    {
        const auto& items = processorRef.getPresetManager().getItems();
        if (items.empty())
            return;

        const int target = currentPresetIndex >= 0 ? (currentPresetIndex + 1) % static_cast<int>(items.size())
                                                   : 0;
        loadPresetByIndex(target, true);
    };

    workspacePanel.onSavePreset = [this](const juce::String& name)
    {
        if (processorRef.getPresetManager().saveUserPreset(name, processorRef.exportFullState()))
        {
            refreshPresetBrowser();
            const auto savedName = name.trim().toUpperCase();
            const auto& items = processorRef.getPresetManager().getItems();
            for (int i = 0; i < static_cast<int>(items.size()); ++i)
            {
                if (items[static_cast<size_t>(i)].name == savedName)
                {
                    setCurrentPresetIndex(i, false);
                    return;
                }
            }

            syncHeaderPresetDisplay();
        }
    };

    workspacePanel.onLoadPreset = [this](int index)
    {
        loadPresetByIndex(index, true);
    };

    workspacePanel.onDeletePreset = [this](int index)
    {
        if (processorRef.getPresetManager().deleteUserPreset(index))
            refreshPresetBrowser();
    };

    sequencerPanel.getStepGrid().onStepSelected = [this](int lane, int step)
    {
        const auto& stepData = processorRef.getSequencerState().getStepData(lane, step);
        sidebarPanel.setSelectedStep(lane, step, stepData);

        int slotIndex = stepData.presetIndex >= 1 && stepData.presetIndex <= 4
                          ? stepData.presetIndex - 1
                          : 0;
        const auto& userSlot = processorRef.getSequencerState().getUserSlot(lane, slotIndex);
        footerPanel.setSelectedSlot(lane, slotIndex, userSlot, laneInfos[lane].name);
    };

    sidebarPanel.onPresetAssigned = [this](int lane, int step, int presetIndex)
    {
        pushUndoSnapshot();
        auto stepData = processorRef.getSequencerState().getStepData(lane, step);
        stepData.active = true;
        stepData.presetIndex = presetIndex;
        processorRef.getSequencerState().setStepData(lane, step, stepData);

        sequencerPanel.getStepGrid().setStepActive(lane, step, true);

        auto* cell = sequencerPanel.getStepGrid().getCell(lane, step);
        if (cell != nullptr)
        {
            if (presetIndex >= 1 && presetIndex <= 4)
                cell->setPresetLabel("U" + juce::String(presetIndex));
            else
                cell->setPresetLabel("");
        }

        sequencerPanel.getStepGrid().refreshLane(lane);

        int slotIndex = presetIndex >= 1 && presetIndex <= 4
                          ? presetIndex - 1
                          : 0;
        const auto& userSlot = processorRef.getSequencerState().getUserSlot(lane, slotIndex);
        footerPanel.setSelectedSlot(lane, slotIndex, userSlot, laneInfos[lane].name);
        markCurrentPresetDirty();
    };

    footerPanel.onSlotDataChanged = [this](int lane, int slotIndex, const UserSlotData& data)
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        if (nowMs - lastFooterHistoryMs > 350.0 || lane != lastFooterHistoryLane || slotIndex != lastFooterHistorySlot)
        {
            pushUndoSnapshot();
            lastFooterHistoryMs = nowMs;
            lastFooterHistoryLane = lane;
            lastFooterHistorySlot = slotIndex;
        }

        processorRef.getSequencerState().setUserSlot(lane, slotIndex, data);
        sequencerPanel.getStepGrid().refreshLane(lane);
        markCurrentPresetDirty();
    };

    refreshSequencerFromState();
    refreshPresetBrowser();
    setCurrentPresetIndex(0, false);
    updateHistoryButtons();

    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(900, 600, 2400, 1600);

    setPage(Page::Sequencer);

    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditor::timerCallback()
{
    if (currentPage != Page::Sequencer)
        return;

    const bool isPlaying = processorRef.isPlaying();
    const int  step      = processorRef.getCurrentStep();

    if (!isPlaying)
    {
        if (lastPlayingStep >= 0)
        {
            sequencerPanel.getStepGrid().setPlayingStep(-1);
            lastPlayingStep = -1;
        }
        return;
    }

    if (step != lastPlayingStep)
    {
        sequencerPanel.getStepGrid().setPlayingStep(step);
        lastPlayingStep = step;
    }
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colours::shellBg);

    const auto bf = getLocalBounds().toFloat();
    juce::ColourGradient bloom(
        Colours::bgAccent.withAlpha(0.20f), bf.getCentreX(), bf.getCentreY(),
        Colours::shellBg.withAlpha(0.0f),  bf.getX(),       bf.getY(), true);
    g.setGradientFill(bloom);
    g.fillAll();
}

void PluginEditor::resized()
{
    namespace PM = PanelMetrics;
    auto bounds = getLocalBounds().reduced(PM::kShellInset);

    headerPanel.setBounds(bounds.removeFromTop(80));
    bounds.removeFromTop(PM::kModuleGap);

    workspacePanel.setBounds(bounds);

    if (currentPage != Page::Sequencer)
        return;

    footerPanel.setBounds(bounds.removeFromBottom(170));
    bounds.removeFromBottom(PM::kModuleGap);

    sidebarPanel.setBounds(bounds.removeFromRight(260));
    bounds.removeFromRight(PM::kModuleGap);
    sequencerPanel.setBounds(bounds);
}

void PluginEditor::setPage(Page page)
{
    currentPage = page;

    const bool showSequencer = page == Page::Sequencer;
    sequencerPanel.setVisible(showSequencer);
    footerPanel.setVisible(showSequencer);
    sidebarPanel.setVisible(showSequencer);
    workspacePanel.setVisible(!showSequencer);

    if (page == Page::Presets)
        workspacePanel.setMode(WorkspacePanel::Mode::Presets);
    else if (page == Page::Settings)
        workspacePanel.setMode(WorkspacePanel::Mode::Settings);

    resized();
    repaint();
}

void PluginEditor::refreshSequencerFromState()
{
    for (int lane = 0; lane < StepGrid::numLanes; ++lane)
    {
        for (int step = 0; step < StepGrid::numSteps; ++step)
        {
            const auto& stepData = processorRef.getSequencerState().getStepData(lane, step);
            auto* cell = sequencerPanel.getStepGrid().getCell(lane, step);
            if (cell == nullptr)
                continue;

            cell->setActive(stepData.active);
            if (stepData.active && stepData.presetIndex >= 1 && stepData.presetIndex <= 4)
                cell->setPresetLabel("U" + juce::String(stepData.presetIndex));
            else
                cell->setPresetLabel("");
        }

        sequencerPanel.getStepGrid().refreshLane(lane);
    }
}

void PluginEditor::refreshPresetBrowser()
{
    processorRef.getPresetManager().refresh();
    workspacePanel.setPresetItems(processorRef.getPresetManager().getItems());
    syncHeaderPresetDisplay();
}

void PluginEditor::loadPresetByIndex(int index, bool pushToHistory)
{
    juce::ValueTree stateTree;
    if (!processorRef.getPresetManager().loadPreset(index, stateTree))
        return;

    if (pushToHistory)
        pushUndoSnapshot();

    applyHistoryState(stateTree);
    setCurrentPresetIndex(index, false);
}

void PluginEditor::setCurrentPresetIndex(int index, bool dirty)
{
    const auto& items = processorRef.getPresetManager().getItems();
    if (index < 0 || index >= static_cast<int>(items.size()))
    {
        currentPresetIndex = -1;
        currentPresetDirty = dirty;
        syncHeaderPresetDisplay();
        return;
    }

    currentPresetIndex = index;
    currentPresetDirty = dirty;
    syncHeaderPresetDisplay();
}

void PluginEditor::markCurrentPresetDirty()
{
    currentPresetDirty = true;
    syncHeaderPresetDisplay();
}

void PluginEditor::syncHeaderPresetDisplay()
{
    const auto& items = processorRef.getPresetManager().getItems();

    if (currentPresetIndex >= 0 && currentPresetIndex < static_cast<int>(items.size()))
    {
        const auto& item = items[static_cast<size_t>(currentPresetIndex)];
        const auto meta = item.category.toUpperCase() + juce::String(" / ") + (item.isFactory ? "FACTORY" : "USER");
        headerPanel.setPresetDisplay(item.name, meta, currentPresetDirty);
    }
    else
    {
        headerPanel.setPresetDisplay("CURRENT STATE", "UNSAVED SNAPSHOT", currentPresetDirty);
    }

    const bool hasPresets = !items.empty();
    headerPanel.setPresetStepEnabled(hasPresets, hasPresets);
}

void PluginEditor::pushUndoSnapshot()
{
    if (applyingHistory)
        return;

    auto snapshot = processorRef.exportFullState();
    if (!undoStack.empty() && undoStack.back().isEquivalentTo(snapshot))
        return;

    undoStack.push_back(snapshot.createCopy());
    if (undoStack.size() > 128)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
    updateHistoryButtons();
}

void PluginEditor::applyHistoryState(const juce::ValueTree& stateTree)
{
    applyingHistory = true;
    processorRef.applyFullState(stateTree);
    refreshSequencerFromState();
    applyingHistory = false;
    updateHistoryButtons();
    syncHeaderPresetDisplay();
}

void PluginEditor::undoLastChange()
{
    if (undoStack.empty())
        return;

    redoStack.push_back(processorRef.exportFullState().createCopy());
    auto target = undoStack.back().createCopy();
    undoStack.pop_back();
    applyHistoryState(target);
}

void PluginEditor::redoLastChange()
{
    if (redoStack.empty())
        return;

    undoStack.push_back(processorRef.exportFullState().createCopy());
    auto target = redoStack.back().createCopy();
    redoStack.pop_back();
    applyHistoryState(target);
}

void PluginEditor::updateHistoryButtons()
{
    headerPanel.setUndoEnabled(!undoStack.empty());
    headerPanel.setRedoEnabled(!redoStack.empty());
}

WaveformDisplay* PluginEditor::getWaveformDisplay()
{
    return &sequencerPanel.getWaveformDisplay();
}

}
