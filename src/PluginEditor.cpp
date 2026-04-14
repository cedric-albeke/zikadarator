#include "PluginEditor.h"

namespace zikada {

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      lookAndFeel(),
      headerPanel(),
      sequencerPanel(p.getPluginState().getValueTreeState()),
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
    g.fillAll(Colours::shellBg);

    const auto bf = getLocalBounds().toFloat();
    juce::ColourGradient bloom(
        Colours::bgAccent.withAlpha(0.20f), bf.getCentreX(), bf.getCentreY(),
        Colours::shellBg.withAlpha(0.0f),  bf.getX(),       bf.getY(), true);
    g.setGradientFill(bloom);
    g.fillAll();
}

void PluginEditor::resized()
{
    namespace PM = PanelMetrics;
    auto bounds = getLocalBounds().reduced(PM::kShellInset);

    headerPanel.setBounds(bounds.removeFromTop(72));
    bounds.removeFromTop(PM::kModuleGap);
    footerPanel.setBounds(bounds.removeFromBottom(64));
    bounds.removeFromBottom(PM::kModuleGap);
    sequencerPanel.setBounds(bounds);
}

WaveformDisplay* PluginEditor::getWaveformDisplay()
{
    return &sequencerPanel.getWaveformDisplay();
}

}
