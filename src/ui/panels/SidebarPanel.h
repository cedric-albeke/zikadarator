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
    int          presetIndex;
};

class SidebarPanel : public juce::Component
{
public:
    SidebarPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSelectedStep(int lane, int step, const StepData& stepData);

    std::function<void(int lane, int step, int presetIndex)> onPresetAssigned;

private:
    int currentLane{-1};
    int currentStep{-1};
    bool hasSelection{false};

    juce::Label laneHeaderLabel;
    std::vector<std::unique_ptr<juce::TextButton>> presetButtons;

    std::vector<LanePresetDef> getPresetsForLane(int lane) const;
    void buildPresetGrid();
    void highlightPresetButton(int presetIndex);
    void notifyPresetAssigned(int presetIndex);
};

}
