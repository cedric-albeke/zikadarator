#include "ui/panels/HeaderPanel.h"

namespace zikada {

HeaderPanel::HeaderPanel()
    : titleLabel("ZIKADA FX")
{
    titleLabel.setColour(Colours::neonGreen);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(playButton);
    addAndMakeVisible(presetButton);
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);
    addAndMakeVisible(randomButton);
    
    playButton.setButtonText("PLAY");
    presetButton.setButtonText("PRESET");
    undoButton.setButtonText("UNDO");
    redoButton.setButtonText("REDO");
    randomButton.setButtonText("RANDOM");
}

void HeaderPanel::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgAccent);
    g.setColour(Colours::neonGreen);
    g.drawLine(0, getHeight() - 1, getWidth(), getHeight() - 1, 2.0f);
}

void HeaderPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    
    titleLabel.setBounds(bounds.removeFromLeft(150));
    
    auto buttonWidth = 80;
    randomButton.setBounds(bounds.removeFromRight(buttonWidth));
    redoButton.setBounds(bounds.removeFromRight(buttonWidth));
    undoButton.setBounds(bounds.removeFromRight(buttonWidth));
    presetButton.setBounds(bounds.removeFromRight(buttonWidth));
    playButton.setBounds(bounds.removeFromRight(buttonWidth));
}

}
