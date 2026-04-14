#pragma once

#include "../components/StepGrid.h"
#include "../components/WaveformDisplay.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zikada {

class SequencerPanel : public juce::Component
{
public:
    SequencerPanel(juce::AudioProcessorValueTreeState& apvts, SequencerState& seqState);

    void paint(juce::Graphics& g) override;
    void resized() override;

    WaveformDisplay& getWaveformDisplay() { return waveformDisplay; }
    StepGrid& getStepGrid() { return stepGrid; }

private:
    StepGrid stepGrid;
    WaveformDisplay waveformDisplay;
};

}