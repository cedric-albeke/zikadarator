#pragma once

#include "../components/StepGrid.h"
#include "../components/WaveformDisplay.h"

namespace zikada {

class SequencerPanel : public juce::Component
{
public:
    SequencerPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

    WaveformDisplay& getWaveformDisplay() { return waveformDisplay; }

private:
    StepGrid stepGrid;
    WaveformDisplay waveformDisplay;
};

}
