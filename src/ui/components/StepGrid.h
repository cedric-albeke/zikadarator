#pragma once

#include "StepCell.h"
#include "../components/Knob.h"
#include "../../state/ParameterIDs.h"
#include "../../state/SequencerState.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zikada {

class StepGrid : public juce::Component, private juce::Timer
{
public:
    StepGrid(juce::AudioProcessorValueTreeState& apvts, SequencerState& seqState);
    ~StepGrid() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;

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
    std::function<void()> onStepPresetEditStarting;
    std::function<void(int lane, int step, int presetIndex)> onStepPresetChanged;

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

    int hoverLane{-1};
    int hoverStep{-1};

    bool paintMode{false};
    bool isPainting{false};
    int lastPaintedLane{-1};
    int lastPaintedStep{-1};

    bool chainDrawMode{false};
    int  chainDrawLane{-1};
    int  chainDrawStartStep{-1};
    int  chainDrawCurrentStep{-1};

    bool chainEraseMode{false};
    int  chainEraseLane{-1};
    int  chainEraseRoot{-1};
    int  chainEraseStartStep{-1};
    int  chainEraseCurrentStep{-1};
    int  chainEraseOriginalLength{1};

    int chainAnimPhase{0};
    juce::Image staticGridLayer;
    bool staticGridLayerDirty{true};

    void setupGrid();
    void timerCallback() override;
    void renderStaticGridLayer(juce::Graphics& g);
    void invalidateStaticGridLayer();
    juce::Rectangle<int> getStepColumnBounds(int step) const;
    std::pair<int, int> hitTestCell(juce::Point<int> pos) const;
    void applyPaintToCell(int lane, int step);
    void cyclePresetAt(int lane, int step, int direction);
    void selectStepAndNotify(int lane, int step);
    bool moveSelectionBy(int laneDelta, int stepDelta);
    bool toggleSelectedStep();
    void drawKeyboardFocusRing(juce::Graphics& g);
    void drawPlayheadRail(juce::Graphics& g);
    int findChainRoot(int lane, int step) const;
};

}
