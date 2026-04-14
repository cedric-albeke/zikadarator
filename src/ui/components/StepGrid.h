#pragma once

#include "StepCell.h"
#include "../components/Knob.h"
#include "../../state/ParameterIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zikada {

class StepGrid : public juce::Component
{
public:
    explicit StepGrid(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setPlayingStep(int step);
    void setSelectedStep(int lane, int step);

    std::function<void(int lane, int step)> onStepSelected;

    static constexpr int numLanes = 6;
    static constexpr int numSteps = 16;

private:
    int lastPlayingStep{-1};
    int selectedLane{-1};
    int selectedStep{-1};
    juce::AudioProcessorValueTreeState& apvts;
    std::array<std::array<std::unique_ptr<StepCell>, numSteps>, numLanes> cells;
    std::array<std::array<std::unique_ptr<juce::ButtonParameterAttachment>, numSteps>, numLanes> attachments;
    std::array<std::unique_ptr<Knob>, numLanes> mixKnobs;

    void setupGrid();
};

}