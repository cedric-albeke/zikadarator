#pragma once

#include "../ZikadaLookAndFeel.h"

namespace zikada {

class StepCell : public juce::Button
{
public:
    StepCell(int laneIndex, int stepIndex);

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    int getLaneIndex() const { return laneIndex; }
    int getStepIndex() const { return stepIndex; }

    void setActive(bool isActive);
    bool isActive() const { return active; }

    void setTied(bool isTied);
    bool isTied() const { return tied; }

private:
    int laneIndex{0};
    int stepIndex{0};
    bool active{false};
    bool tied{false};
};

}
