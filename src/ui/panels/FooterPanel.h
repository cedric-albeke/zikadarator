#pragma once

#include "../ZikadaLookAndFeel.h"
#include "../components/Knob.h"
#include "../../state/UserSlotData.h"
#include "../../state/ParameterIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>

namespace zikada {

class FooterPanel : public juce::Component
{
public:
    explicit FooterPanel(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSelectedSlot(int lane, int slot, const UserSlotData& data, const juce::String& laneName);

    std::function<void(int lane, int slot, const UserSlotData& data)> onSlotDataChanged;

private:
    juce::Slider     dryWetSlider;
    juce::TextButton bypassButton  { "BYPASS" };
    juce::Label      mixModeLabel;
    juce::Label      outputGainLabel;

    juce::Rectangle<int> dryWetZone;
    juce::Rectangle<int> detailZone;
    juce::Rectangle<int> signalZone;

    juce::Rectangle<int> mixModeHeaderRect;
    juce::Rectangle<int> outputGainHeaderRect;
    juce::Rectangle<int> stepResHeaderRect;

    juce::Label    stepResLabel;
    juce::ComboBox stepResolutionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        stepResolutionAttachment;

    std::array<std::unique_ptr<Knob>, 7> stepKnobs;

    juce::TextButton modModeButton { "MOD" };
    static constexpr int kNumModSlots = 3;
    std::array<std::unique_ptr<juce::TextButton>, kNumModSlots> modTargetButtons;
    std::array<std::unique_ptr<juce::TextButton>, kNumModSlots> modSourceButtons;
    std::array<std::unique_ptr<juce::Slider>, kNumModSlots> modAmountSliders;
    std::array<std::unique_ptr<juce::Slider>, kNumModSlots> modParamSliders;
    std::array<std::unique_ptr<juce::Label>, kNumModSlots> modParamLabels;

    bool         hasSelection{false};
    int          selectedLane{-1};
    int          selectedSlot{-1};
    juce::String selectedLaneName;
    bool         updatingFromState{false};
    bool         modModeActive{false};

    void drawDryWetModule (juce::Graphics& g) const;
    void drawDetailDock   (juce::Graphics& g) const;
    void drawSignalModule (juce::Graphics& g) const;
    void notifySlotDataChanged();

    void setupModulationControls();
    void updateModulationControlsFromData(const ModulationData& modData);
    ModulationData readModulationDataFromControls() const;
    void applyModModeVisibility();
    void cycleModTarget(int slot);
    void cycleModSource(int slot);
    void updateModParamLabel(int slot);

    static juce::String targetToString(ModulationTarget t);
    static ModulationTarget targetFromString(const juce::String& s);
    static juce::String sourceToString(ModulationSource s);
    static ModulationSource sourceFromString(const juce::String& s);
};

} // namespace zikada
