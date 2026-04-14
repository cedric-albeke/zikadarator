#include "PluginEditor.h"

namespace zikada {

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      lookAndFeel(),
      headerPanel(),
      sequencerPanel(),
      footerPanel()
{
    setLookAndFeel(&lookAndFeel);
    
    addAndMakeVisible(headerPanel);
    addAndMakeVisible(sequencerPanel);
    addAndMakeVisible(footerPanel);
    
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(900, 600, 2400, 1600);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel(nullptr);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgPrimary);
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();
    
    headerPanel.setBounds(bounds.removeFromTop(60));
    footerPanel.setBounds(bounds.removeFromBottom(60));
    sequencerPanel.setBounds(bounds);
}

}
