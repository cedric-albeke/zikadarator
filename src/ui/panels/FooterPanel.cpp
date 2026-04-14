#include "ui/panels/FooterPanel.h"

namespace zikada {

FooterPanel::FooterPanel()
{
    addAndMakeVisible(dryWetSlider);
    addAndMakeVisible(bypassButton);
    addAndMakeVisible(mixModeLabel);
    addAndMakeVisible(outputGainLabel);
    
    dryWetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dryWetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(100.0);
    
    mixModeLabel.setText("LINEAR", juce::dontSendNotification);
    mixModeLabel.setJustificationType(juce::Justification::centred);
    mixModeLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    mixModeLabel.setColour(juce::Label::textColourId, Colours::neonGreen);
    
    outputGainLabel.setText("0.0 dB", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centredRight);
    outputGainLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    outputGainLabel.setColour(juce::Label::textColourId, Colours::white85);
    
    bypassButton.setClickingTogglesState(true);
}

void FooterPanel::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgAccent);
    
    auto bounds = getLocalBounds().toFloat();
    g.setColour(Colours::neonGreen);
    g.drawLine(0.0f, 1.0f, bounds.getRight(), 1.0f, 2.0f);
    
    g.setColour(Colours::white50);
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("DRY/WET", 16, 6, 60, 12, juce::Justification::left, false);
}

void FooterPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8, 4);
    
    bypassButton.setBounds(bounds.removeFromRight(90).withSizeKeepingCentre(90, 28));
    bounds.removeFromRight(8);
    
    outputGainLabel.setBounds(bounds.removeFromRight(80));
    bounds.removeFromRight(8);
    
    mixModeLabel.setBounds(bounds.removeFromRight(80));
    bounds.removeFromRight(16);
    
    dryWetSlider.setBounds(bounds);
}

}
