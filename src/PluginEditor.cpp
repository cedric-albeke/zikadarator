#include "PluginEditor.h"

namespace zikada {

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      lookAndFeel(),
      headerPanel(),
      sequencerPanel(p.getPluginState().getValueTreeState()),
      footerPanel(),
      sidebarPanel()
{
    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(headerPanel);
    addAndMakeVisible(sequencerPanel);
    addAndMakeVisible(footerPanel);
    addAndMakeVisible(sidebarPanel);

    sequencerPanel.getStepGrid().onStepSelected = [this](int lane, int step)
    {
        const auto& stepData = processorRef.getSequencerState().getStepData(lane, step);
        sidebarPanel.setSelectedStep(lane, step, stepData);

        int slotIndex = stepData.presetIndex >= 1 && stepData.presetIndex <= 4
                          ? stepData.presetIndex - 1
                          : 0;
        const auto& userSlot = processorRef.getSequencerState().getUserSlot(lane, slotIndex);
        sidebarPanel.setUserSlotData(lane, slotIndex, userSlot);
    };

    sidebarPanel.onPresetAssigned = [this](int lane, int step, int presetIndex)
    {
        auto stepData = processorRef.getSequencerState().getStepData(lane, step);
        stepData.active = true;
        stepData.presetIndex = presetIndex;
        processorRef.getSequencerState().setStepData(lane, step, stepData);

        sequencerPanel.getStepGrid().setStepActive(lane, step, true);

        auto* cell = sequencerPanel.getStepGrid().getCell(lane, step);
        if (cell != nullptr)
            cell->setPresetLabel("U" + juce::String(presetIndex));

        sequencerPanel.getStepGrid().refreshLane(lane);
    };

    sidebarPanel.onUserSlotChanged = [this](int lane, int slotIndex, const UserSlotData& data)
    {
        processorRef.getSequencerState().setUserSlot(lane, slotIndex, data);
        sequencerPanel.getStepGrid().refreshLane(lane);
    };

    for (int lane = 0; lane < StepGrid::numLanes; ++lane)
    {
        for (int step = 0; step < StepGrid::numSteps; ++step)
        {
            const auto& stepData = processorRef.getSequencerState().getStepData(lane, step);
            auto* cell = sequencerPanel.getStepGrid().getCell(lane, step);
            if (cell != nullptr && stepData.active && stepData.presetIndex >= 1 && stepData.presetIndex <= 4)
                cell->setPresetLabel("U" + juce::String(stepData.presetIndex));
        }
    }

    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(900, 600, 2400, 1600);

    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditor::timerCallback()
{
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

    headerPanel.setBounds(bounds.removeFromTop(72));
    bounds.removeFromTop(PM::kModuleGap);
    footerPanel.setBounds(bounds.removeFromBottom(140));
    bounds.removeFromBottom(PM::kModuleGap);
    sidebarPanel.setBounds(bounds.removeFromRight(260));
    bounds.removeFromRight(PM::kModuleGap);
    sequencerPanel.setBounds(bounds);
}

WaveformDisplay* PluginEditor::getWaveformDisplay()
{
    return &sequencerPanel.getWaveformDisplay();
}

}
