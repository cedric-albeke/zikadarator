#include "ui/panels/SequencerPanel.h"

namespace zikada {

SequencerPanel::SequencerPanel()
{
    addAndMakeVisible(stepGrid);
    addAndMakeVisible(waveformDisplay);
}

void SequencerPanel::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgPrimary);
    
    auto bounds = getLocalBounds().reduced(12, 8);
    auto inputBounds = bounds.removeFromTop(56);
    g.setColour(Colours::bgSurface);
    g.fillRect(inputBounds);
    g.setColour(Colours::white10);
    g.drawRect(inputBounds, 1);
}

void SequencerPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12, 8);
    auto inputBounds = bounds.removeFromTop(56);
    waveformDisplay.setBounds(inputBounds.reduced(80, 4));
    stepGrid.setBounds(bounds.reduced(0, 4));
}

}
