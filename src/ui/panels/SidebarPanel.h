#pragma once

#include "../ZikadaLookAndFeel.h"
#include "../components/Knob.h"
#include "../../state/UserSlotData.h"
#include "../../state/StepData.h"
#include <array>
#include <memory>
#include <vector>

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
    void setUserSlotData(int lane, int slotIndex, const UserSlotData& data);

    std::function<void(int lane, int step, int presetIndex)> onPresetAssigned;
    std::function<void(int lane, int slotIndex, const UserSlotData& data)> onUserSlotChanged;

private:
    int currentLane{-1};
    int currentStep{-1};
    int currentSlot{0};
    bool hasSelection{false};
    bool updatingFromState{false};

    juce::Label laneHeaderLabel;
    juce::Component presetGridContainer;
    std::vector<std::unique_ptr<juce::TextButton>> presetButtons;
    std::array<juce::TextButton, 4> slotTabs;
    std::array<std::unique_ptr<Knob>, 7> paramKnobs;

    static constexpr int kNumModSlots = 3;
    std::array<std::unique_ptr<juce::TextButton>, kNumModSlots> modTargetButtons;
    std::array<std::unique_ptr<juce::TextButton>, kNumModSlots> modSourceButtons;
    std::array<std::unique_ptr<juce::Slider>, kNumModSlots> modAmountSliders;
    std::array<std::unique_ptr<juce::Slider>, kNumModSlots> modParamSliders;
    std::array<std::unique_ptr<juce::Label>, kNumModSlots> modParamLabels;

    void setupControls();
    void buildPresetGrid();
    std::vector<LanePresetDef> getPresetsForLane(int lane) const;
    void highlightPresetButton(int presetIndex);
    void updateParamKnobs(const UserSlotData& data);
    void updateModulationControls(const ModulationData& modData);
    void notifyUserSlotChanged();
    void notifyPresetAssigned(int presetIndex);

    void cycleModTarget(int slot);
    void cycleModSource(int slot);
    void updateModParamLabel(int slot);

    static juce::String targetToString(ModulationTarget t);
    static ModulationTarget targetFromString(const juce::String& s);
    static juce::String sourceToString(ModulationSource s);
    static ModulationSource sourceFromString(const juce::String& s);
};

}
