#include "ui/panels/HeaderPanel.h"

namespace zikada {

HeaderPanel::HeaderPanel()
    : titleLabel("ZIKADA")
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
    
    playButton.setClickingTogglesState(true);
}

void HeaderPanel::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgAccent);
    
    auto bounds = getLocalBounds().toFloat();
    g.setColour(Colours::neonGreen);
    g.drawLine(0.0f, bounds.getBottom() - 1.0f, bounds.getRight(), bounds.getBottom() - 1.0f, 2.0f);
    
    auto statusBounds = bounds.removeFromRight(140.0f).reduced(8.0f);
    auto dotX = statusBounds.getCentreX() - 30.0f;
    auto dotY = statusBounds.getCentreY();
    
    juce::Path dot;
    dot.addEllipse(dotX - 4.0f, dotY - 4.0f, 8.0f, 8.0f);
    g.setColour(Colours::neonGreen);
    g.fillPath(dot);
    
    g.setColour(Colours::neonGreen.withAlpha(0.4f));
    g.fillEllipse(dotX - 7.0f, dotY - 7.0f, 14.0f, 14.0f);
    
    g.setColour(Colours::white85);
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("SYSTEM ONLINE", static_cast<int>(dotX + 12.0f), static_cast<int>(dotY - 5.0f), 90, 10, juce::Justification::left, false);
}

void HeaderPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8, 0);
    
    titleLabel.setBounds(bounds.removeFromLeft(120));
    
    auto buttonWidth = 72;
    auto buttonHeight = 28;
    auto spacing = 8;
    
    auto buttonArea = bounds.removeFromRight((buttonWidth + spacing) * 5 + 8);
    buttonArea = buttonArea.withHeight(buttonHeight).withCentre(buttonArea.getCentre());
    
    randomButton.setBounds(buttonArea.removeFromRight(buttonWidth));
    buttonArea.removeFromRight(spacing);
    redoButton.setBounds(buttonArea.removeFromRight(buttonWidth));
    buttonArea.removeFromRight(spacing);
    undoButton.setBounds(buttonArea.removeFromRight(buttonWidth));
    buttonArea.removeFromRight(spacing);
    presetButton.setBounds(buttonArea.removeFromRight(buttonWidth));
    buttonArea.removeFromRight(spacing);
    playButton.setBounds(buttonArea.removeFromRight(buttonWidth));
}

}
