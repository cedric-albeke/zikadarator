#include "PluginEditor.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

namespace zikada {

namespace {

std::unique_ptr<juce::FileLogger> uiLogFile;

void ensureUiLogger()
{
    static bool initialised = false;

    if (initialised)
        return;

    initialised = true;

    if (auto* logger = juce::FileLogger::createDefaultAppLogger("ZIKADARATOR",
                                                                "UI-Debug.log",
                                                                "ZIKADARATOR UI debug log",
                                                                512 * 1024))
    {
        uiLogFile.reset(logger);
        juce::Logger::setCurrentLogger(uiLogFile.get());
        juce::Logger::writeToLog("[ZIKADARATOR] logging to " + uiLogFile->getLogFile().getFullPathName());
    }
}

void debugUiLog(const juce::String& message)
{
    ensureUiLogger();
    juce::Logger::writeToLog("[ZIKADARATOR] " + message);

#if JUCE_DEBUG
    DBG("[ZIKADARATOR] " + message);
#endif
}

}

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      lookAndFeel(),
      headerPanel(),
      sequencerPanel(p.getPluginState().getValueTreeState(), p.getSequencerState()),
      footerPanel(processorRef.getPluginState().getValueTreeState()),
      sidebarPanel(),
      workspacePanel()
{
    debugUiLog("PluginEditor constructed");
    setOpaque(true);
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
    headerPanel.onPresetMenuRequested = [this] { showHeaderPresetMenu(); };
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

    workspacePanel.onToggleFavoritePreset = [this](int index)
    {
        const auto& items = processorRef.getPresetManager().getItems();
        if (index < 0 || index >= static_cast<int>(items.size()))
            return;

        const auto presetName = items[static_cast<size_t>(index)].name;
        processorRef.getPresetManager().toggleFavorite(presetName);
        refreshPresetBrowser();
        const auto& refreshedItems = processorRef.getPresetManager().getItems();
        for (int i = 0; i < static_cast<int>(refreshedItems.size()); ++i)
            if (refreshedItems[static_cast<size_t>(i)].name == presetName)
                return setCurrentPresetIndex(i, currentPresetDirty);

        syncHeaderPresetDisplay();
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

    sequencerPanel.getStepGrid().onChainChanged = [this](int, int, int)
    {
        markCurrentPresetDirty();
    };

    sidebarPanel.onPresetAssigned = [this](int lane, int step, int presetIndex)
    {
        pushUndoSnapshot();
        sequencerPanel.getStepGrid().removeChainAt(lane, step);
        auto stepData = processorRef.getSequencerState().getStepData(lane, step);
        stepData.active = true;
        stepData.presetIndex = presetIndex;
        processorRef.getSequencerState().setStepData(lane, step, stepData);

        sequencerPanel.getStepGrid().setStepActive(lane, step, true);

        auto* cell = sequencerPanel.getStepGrid().getCell(lane, step);
        if (cell != nullptr)
            cell->setPresetIndex(presetIndex);

        sequencerPanel.getStepGrid().refreshChainVisuals(lane);
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
    applyWineSafeRenderingIfNeeded();

    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditor::timerCallback()
{
    applyWineSafeRenderingIfNeeded();
    footerPanel.refreshGlobalControlLabels();

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

void PluginEditor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();
    wineSafeRendererApplied = false;
    debugUiLog("parentHierarchyChanged: peer reset requested");
    applyWineSafeRenderingIfNeeded();
}

void PluginEditor::visibilityChanged()
{
    AudioProcessorEditor::visibilityChanged();
    if (isShowing())
        wineSafeRendererApplied = false;

    debugUiLog("visibilityChanged: showing=" + juce::String(isShowing() ? 1 : 0));
    applyWineSafeRenderingIfNeeded();
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

#if JUCE_DEBUG
    const auto pageName = pageToString(currentPage);
    g.setColour(juce::Colours::magenta.withAlpha(0.65f));
    g.setFont(12.0f);
    g.drawText("page=" + juce::String(pageName)
                 + " wineRenderer=" + juce::String(wineSafeRendererApplied ? "on" : "off")
                 + " size=" + juce::String(getWidth()) + "x" + juce::String(getHeight()),
               getLocalBounds().removeFromBottom(18).reduced(8, 0),
               juce::Justification::centredRight,
               false);
#endif
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

    sidebarPanel.setBounds(bounds.removeFromRight(300));
    bounds.removeFromRight(PM::kModuleGap);
    sequencerPanel.setBounds(bounds);
}

void PluginEditor::setPage(Page page)
{
    currentPage = page;
    const auto pageName = pageToString(page);

    const bool showSequencer = page == Page::Sequencer;
    sequencerPanel.setVisible(showSequencer);
    footerPanel.setVisible(showSequencer);
    sidebarPanel.setVisible(showSequencer);
    workspacePanel.setVisible(!showSequencer);

    if (page == Page::Presets)
        workspacePanel.setMode(WorkspacePanel::Mode::Presets);
    else if (page == Page::Settings)
        workspacePanel.setMode(WorkspacePanel::Mode::Settings);

    debugUiLog("setPage(" + juce::String(pageName)
               + "): workspaceVisible=" + juce::String(workspacePanel.isVisible() ? 1 : 0)
               + ", sequencerVisible=" + juce::String(sequencerPanel.isVisible() ? 1 : 0));

    resized();
    workspacePanel.repaint();
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
            if (stepData.active)
                cell->setPresetIndex(stepData.presetIndex);
            else
                cell->setPresetIndex(0);
        }

        sequencerPanel.getStepGrid().refreshChainVisuals(lane);
        sequencerPanel.getStepGrid().refreshLane(lane);
    }
}

void PluginEditor::refreshPresetBrowser()
{
    processorRef.getPresetManager().refresh();
    workspacePanel.setPresetItems(processorRef.getPresetManager().getItems());
    syncHeaderPresetDisplay();
}

void PluginEditor::showHeaderPresetMenu()
{
    const auto& items = processorRef.getPresetManager().getItems();
    if (items.empty())
        return;

    debugUiLog("showHeaderPresetMenu: redirecting to preset browser instead of spawning PopupMenu, itemCount="
               + juce::String(static_cast<int>(items.size())));
    headerPanel.setSelectedPage(HeaderPanel::Page::Presets);
    setPage(Page::Presets);
}

void PluginEditor::loadPresetByIndex(int index, bool pushToHistory)
{
    const auto& currentItems = processorRef.getPresetManager().getItems();
    if (index < 0 || index >= static_cast<int>(currentItems.size()))
        return;

    const auto presetName = currentItems[static_cast<size_t>(index)].name;
    juce::ValueTree stateTree;
    if (!processorRef.getPresetManager().loadPreset(index, stateTree))
        return;

    if (pushToHistory)
        pushUndoSnapshot();

    applyHistoryState(stateTree);
    processorRef.getPresetManager().markPresetUsed(presetName);
    refreshPresetBrowser();
    const auto& refreshedItems = processorRef.getPresetManager().getItems();
    for (int i = 0; i < static_cast<int>(refreshedItems.size()); ++i)
        if (refreshedItems[static_cast<size_t>(i)].name == presetName)
            return setCurrentPresetIndex(i, false);

    syncHeaderPresetDisplay();
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
        const auto meta = (item.isFavorite ? juce::String("FAV / ") : juce::String())
                        + item.category.toUpperCase() + juce::String(" / ")
                        + (item.isFactory ? "FACTORY" : "USER");
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

const char* PluginEditor::pageToString(Page page)
{
    switch (page)
    {
        case Page::Sequencer: return "Sequencer";
        case Page::Presets:   return "Presets";
        case Page::Settings:  return "Settings";
    }
    return "Unknown";
}

bool PluginEditor::isRunningUnderWine()
{
#if JUCE_WINDOWS
    if (auto* ntdll = ::GetModuleHandleA("ntdll.dll"))
        return ::GetProcAddress(ntdll, "wine_get_version") != nullptr;
#endif

    return false;
}

void PluginEditor::applyWineSafeRenderingIfNeeded()
{
    if (wineSafeRendererApplied || !isRunningUnderWine())
        return;

    auto* peer = getPeer();

    if (peer == nullptr)
    {
        debugUiLog("applyWineSafeRenderingIfNeeded: no peer yet");
        return;
    }

    const auto engines = peer->getAvailableRenderingEngines();
    const auto softwareIndex = engines.indexOf("Software Renderer");
    debugUiLog("applyWineSafeRenderingIfNeeded: engines=" + engines.joinIntoString(", ")
               + ", current=" + juce::String(peer->getCurrentRenderingEngine())
               + ", softwareIndex=" + juce::String(softwareIndex));

    if (softwareIndex >= 0 && peer->getCurrentRenderingEngine() != softwareIndex)
    {
        peer->setCurrentRenderingEngine(softwareIndex);
        debugUiLog("applyWineSafeRenderingIfNeeded: switched to software renderer");
    }

    wineSafeRendererApplied = true;
    peer->repaint(getLocalBounds());
    repaint();
}

WaveformDisplay* PluginEditor::getWaveformDisplay()
{
    return &sequencerPanel.getWaveformDisplay();
}

}
