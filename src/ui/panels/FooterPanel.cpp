#include "ui/panels/FooterPanel.h"

namespace zikada {

FooterPanel::FooterPanel()
{
    addAndMakeVisible(dryWetSlider);
    addAndMakeVisible(bypassButton);
    addAndMakeVisible(mixModeLabel);
    
    dryWetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dryWetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(100.0);
    
    mixModeLabel.setText("LINEAR", juce::dontSendNotification);
    mixModeLabel.setJustificationType(juce::Justification::centred);
}

void FooterPanel::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgAccent);
    g.setColour(Colours::neonGreen);
    g.drawLine(0, 0, getWidth(), 0, 2.0f);
}

void FooterPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    
    bypassButton.setBounds(bounds.removeFromRight(100));
    mixModeLabel.setBounds(bounds.removeFromRight(100));
    dryWetSlider.setBounds(bounds);
}

}
