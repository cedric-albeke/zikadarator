#pragma once

#include "../ZikadaLookAndFeel.h"
#include <array>
#include <memory>

namespace zikada {

class FooterPanel : public juce::Component
{
public:
    FooterPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider     dryWetSlider;
    juce::TextButton bypassButton  { "BYPASS" };
    juce::Label      mixModeLabel;
    juce::Label      outputGainLabel;

    juce::Rectangle<int> dryWetZone;
    juce::Rectangle<int> signalZone;
    juce::Rectangle<int> mixModeHeaderRect;
    juce::Rectangle<int> outputGainHeaderRect;

    void drawDryWetModule (juce::Graphics& g) const;
    void drawSignalModule (juce::Graphics& g) const;
};

} // namespace zikada
