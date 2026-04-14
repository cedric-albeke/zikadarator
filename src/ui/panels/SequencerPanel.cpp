#include "ui/panels/SequencerPanel.h"

namespace zikada {

SequencerPanel::SequencerPanel(juce::AudioProcessorValueTreeState& apvts)
    : stepGrid(apvts)
{
    addAndMakeVisible(stepGrid);
    addAndMakeVisible(waveformDisplay);
}

void SequencerPanel::paint(juce::Graphics& g)
{
    namespace PM = PanelMetrics;

    ZikadaLookAndFeel::drawPremiumPanel(g, getLocalBounds(), false);

    auto inner = getLocalBounds().reduced(PM::kPadding, 10);
    auto inputStripBounds = inner.removeFromTop(98);

    ZikadaLookAndFeel::drawDeviceDisplay(g, inputStripBounds);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());
    if (laf != nullptr)
    {
        auto displayInnerF = inputStripBounds.reduced(3, 3).toFloat();
        auto labelAreaF    = displayInnerF.withWidth(90.0f);

        g.setFont(laf->getVcrFont(14.0f));
        g.setColour(Colours::neonGreen.withAlpha(0.82f));
        g.drawText("SIGNAL",
                   labelAreaF.withHeight(labelAreaF.getHeight() * 0.5f),
                   juce::Justification::centred, false);

        g.setColour(Colours::white.withAlpha(0.50f));
        g.drawText("INPUT",
                   labelAreaF.withY(labelAreaF.getY() + labelAreaF.getHeight() * 0.5f)
                             .withHeight(labelAreaF.getHeight() * 0.5f),
                   juce::Justification::centred, false);

        const float sepX = displayInnerF.getX() + 90.0f;
        g.setColour(Colours::white.withAlpha(0.08f));
        g.drawLine(sepX, displayInnerF.getY() + 5.0f,
                   sepX, displayInnerF.getBottom() - 5.0f, 1.0f);
    }

    inner.removeFromTop(8);
    auto gridSurface = inner;
    g.setColour(Colours::bgSurface.withAlpha(0.42f));
    g.fillRoundedRectangle(gridSurface.toFloat(), PM::kInnerCorner);
    g.setColour(Colours::white.withAlpha(0.07f));
    g.drawRoundedRectangle(gridSurface.toFloat().reduced(0.5f), PM::kInnerCorner, 1.0f);
}

void SequencerPanel::resized()
{
    namespace PM = PanelMetrics;
    auto inner = getLocalBounds().reduced(PM::kPadding, 10);

    auto inputStripBounds = inner.removeFromTop(98);
    auto displayInner     = inputStripBounds.reduced(3, 3);
    displayInner.removeFromLeft(90);
    waveformDisplay.setBounds(displayInner);

    inner.removeFromTop(8);
    stepGrid.setBounds(inner.reduced(6, 6));
}

}