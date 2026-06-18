#pragma once

#include "../ZikadaLookAndFeel.h"
#include "../../state/StepData.h"
#include <vector>
#include <memory>

namespace zikada {

struct LanePresetDef
{
    juce::String label;
    juce::String tooltip;
    juce::String infoText;
    int          presetIndex;
};

class SidebarPanel : public juce::Component
{
public:
    SidebarPanel();

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void setSelectedStep(int lane, int step, const StepData& stepData);

    juce::String getPresetLabel(int lane, int presetIndex) const;
    juce::String getPresetTooltip(int lane, int presetIndex) const;
    juce::String getPresetInfo(int lane, int presetIndex) const;

    std::function<void(int lane, int step, int presetIndex)> onPresetAssigned;

private:
    int currentLane{-1};
    int currentStep{-1};
    bool hasSelection{false};
    int hoveredPresetIndex{-1};
    int selectedPresetIndex{-1};
    int focusedPresetButtonIndex{-1};

    juce::Label infoLabel;
    std::vector<std::unique_ptr<juce::TextButton>> presetButtons;

    std::vector<LanePresetDef> getPresetsForLane(int lane) const;
    void buildPresetGrid();
    void highlightPresetButton(int presetIndex);
    void notifyPresetAssigned(int presetIndex);
    void updateInfoForSelection();
    void updateInfoForHover(int presetIndex);
    void moveFocusedPresetBy(int columnDelta, int rowDelta);
    void activateFocusedPreset();
    int findPresetButtonIndex(int presetIndex) const;
};

}
