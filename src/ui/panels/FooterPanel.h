#pragma once

#include "../ZikadaLookAndFeel.h"
#include "../components/Knob.h"
#include "../../state/StepData.h"
#include <array>
#include <memory>

namespace zikada {

class FooterPanel : public juce::Component
{
public:
    FooterPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSelectedStep(int lane, int step, const StepData& data, const juce::String& laneName);

    std::function<void(int lane, int step, const StepData& data)> onStepDataChanged;

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

    std::array<std::unique_ptr<Knob>, 7> stepKnobs;
    bool         hasSelection{false};
    int          selectedLane{-1};
    int          selectedStep{-1};
    juce::String selectedLaneName;
    bool         updatingFromState{false};

    void drawDryWetModule (juce::Graphics& g) const;
    void drawDetailDock   (juce::Graphics& g) const;
    void drawSignalModule (juce::Graphics& g) const;
    void notifyStepDataChanged();
};

} // namespace zikada
