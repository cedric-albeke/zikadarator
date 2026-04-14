#include "ui/panels/SequencerPanel.h"

namespace zikada {

SequencerPanel::SequencerPanel()
{
    addAndMakeVisible(stepGrid);
}

void SequencerPanel::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgPrimary);
}

void SequencerPanel::resized()
{
    stepGrid.setBounds(getLocalBounds().reduced(16));
}

}
