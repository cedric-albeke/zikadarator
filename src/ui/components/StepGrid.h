#pragma once

#include "StepCell.h"

namespace zikada {

class StepGrid : public juce::Component
{
public:
    StepGrid();

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int numLanes = 6;
    static constexpr int numSteps = 16;

private:
    std::array<std::array<std::unique_ptr<StepCell>, numSteps>, numLanes> cells;
    void setupGrid();
};

}
