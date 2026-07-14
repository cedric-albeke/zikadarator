#pragma once

#include "../ZikadaLookAndFeel.h"

namespace zikada {

class StepCell : public juce::Button
{
public:
    StepCell(int laneIndex, int stepIndex);

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Rectangle<float> getChainBadgeBounds() const;

    int getLaneIndex() const { return laneIndex; }
    int getStepIndex() const { return stepIndex; }

    void setActive(bool isActive);
    bool isActive() const { return active; }

    void setTied(bool isTied);
    bool isTied() const { return tied; }

    void setChained(bool isChained);
    bool isChained() const { return chained; }

    void setChainable(bool isChainable);
    bool isChainable() const { return chainable; }

    void setHovered(bool isHovered);
    bool isHovered() const { return hovered; }

    void setChainAnimPhase(int phase);

    void setPlaying(bool isPlaying);
    bool isPlaying() const { return playing; }

    void setSelected(bool isSelected);
    bool isSelected() const { return selected; }

    void setPresetIndex(int index);
    int getPresetIndex() const { return presetIndex; }

    std::function<void()> onSelected;

private:
    int laneIndex{0};
    int stepIndex{0};
    bool active{false};
    bool tied{false};
    bool chained{false};
    bool chainable{false};
    bool hovered{false};
    bool playing{false};
    int  chainAnimPhase{0};
    bool selected{false};
    int presetIndex{0};
};

}
