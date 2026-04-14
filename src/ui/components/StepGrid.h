#pragma once

#include "StepCell.h"
#include "../components/Knob.h"
#include "../../state/ParameterIDs.h"
#include "../../state/SequencerState.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zikada {

class StepGrid : public juce::Component
{
public:
    StepGrid(juce::AudioProcessorValueTreeState& apvts, SequencerState& seqState);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void setPlayingStep(int step);
    void setSelectedStep(int lane, int step);
    void refreshLane(int lane);
    void setStepActive(int lane, int step, bool active);
    StepCell* getCell(int lane, int step);

    void refreshChainVisuals(int lane);
    void setStepChainLength(int lane, int step, int length);
    int getStepChainLength(int lane, int step) const;
    bool isStepConsumedByChain(int lane, int step) const;
    void removeChainAt(int lane, int step);

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    std::function<void(int lane, int step)> onStepSelected;
    std::function<void(int lane, int step, int chainLength)> onChainChanged;

    static constexpr int numLanes = 6;
    static constexpr int numSteps = 16;

private:
    int lastPlayingStep{-1};
    int selectedLane{-1};
    int selectedStep{-1};
    juce::AudioProcessorValueTreeState& apvts;
    SequencerState& sequencerState;
    std::array<std::array<std::unique_ptr<StepCell>, numSteps>, numLanes> cells;
    std::array<std::array<std::unique_ptr<juce::ButtonParameterAttachment>, numSteps>, numLanes> attachments;
    std::array<std::unique_ptr<Knob>, numLanes> mixKnobs;
    std::array<std::unique_ptr<juce::TextButton>, numLanes> muteButtons;
    std::array<std::unique_ptr<juce::TextButton>, numLanes> soloButtons;
    std::array<std::unique_ptr<juce::ButtonParameterAttachment>, numLanes> muteAttachments;
    std::array<std::unique_ptr<juce::ButtonParameterAttachment>, numLanes> soloAttachments;

    juce::TextButton chainExtendButton{"+"};
    int hoverLane{-1};
    int hoverStep{-1};

    bool paintMode{false};
    bool isPainting{false};
    int lastPaintedLane{-1};
    int lastPaintedStep{-1};

    void setupGrid();
    std::pair<int, int> hitTestCell(juce::Point<int> pos) const;
    void applyPaintToCell(int lane, int step);
};

}