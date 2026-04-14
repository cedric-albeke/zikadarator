#pragma once

#include "../ZikadaLookAndFeel.h"

namespace zikada {

class FooterPanel : public juce::Component
{
public:
    FooterPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider dryWetSlider;
    juce::TextButton bypassButton{"BYPASS"};
    juce::Label mixModeLabel;
};

}
