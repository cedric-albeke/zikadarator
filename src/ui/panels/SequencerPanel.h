#pragma once

#include "../components/StepGrid.h"

namespace zikada {

class SequencerPanel : public juce::Component
{
public:
    SequencerPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    StepGrid stepGrid;
};

}
