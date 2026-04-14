#include "ui/panels/FooterPanel.h"

namespace zikada {

namespace {
    constexpr int kDryWetW  = 200;
    constexpr int kSignalW  = 240;
    constexpr int kZoneGap  = 6;
    constexpr int kHPad     = 8;
    constexpr int kVPad     = 4;
    constexpr int kLabelH   = 12;
    constexpr int kInnerPad = 8;
}

FooterPanel::FooterPanel()
{
    dryWetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dryWetSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(100.0);
    dryWetSlider.onValueChange = [this] { repaint(); };
    addAndMakeVisible(dryWetSlider);

    mixModeLabel.setText("LINEAR", juce::dontSendNotification);
    mixModeLabel.setJustificationType(juce::Justification::centred);
    mixModeLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    mixModeLabel.setColour(juce::Label::textColourId, Colours::neonGreen);
    addAndMakeVisible(mixModeLabel);

    outputGainLabel.setText("0.0 dB", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centred);
    outputGainLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    outputGainLabel.setColour(juce::Label::textColourId, Colours::white85);
    addAndMakeVisible(outputGainLabel);

    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId,   Colours::bgSurface);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,  Colours::neonGreen);
    bypassButton.setColour(juce::TextButton::textColourOffId,   Colours::white85);
    bypassButton.setColour(juce::TextButton::textColourOnId,    Colours::bgPrimary);
    addAndMakeVisible(bypassButton);
}

void FooterPanel::paint(juce::Graphics& g)
{
    ZikadaLookAndFeel::drawPremiumPanel(g, getLocalBounds(), true);

    if (getWidth() <= 0)
        return;

    drawDryWetModule(g);
    drawDetailDock(g);
    drawSignalModule(g);

    const int sepY = getHeight() / 5;
    const int sepH = getHeight() * 3 / 5;

    if (detailZone.getX() > 0)
        ZikadaLookAndFeel::drawModuleSeparator(g,
            detailZone.getX() - kZoneGap / 2 - 1, sepY, sepH, false);

    if (signalZone.getX() > 0)
        ZikadaLookAndFeel::drawModuleSeparator(g,
            signalZone.getX() - kZoneGap / 2 - 1, sepY, sepH, false);
}

void FooterPanel::drawDryWetModule(juce::Graphics& g) const
{
    ZikadaLookAndFeel::drawDeviceDisplay(g, dryWetZone);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    auto inner  = dryWetZone.reduced(kInnerPad, 4).toFloat();
    auto topRow = inner.removeFromTop(static_cast<float>(kLabelH));

    g.setFont(laf != nullptr ? laf->getVcrFont(9.0f)
                              : juce::Font(juce::FontOptions().withHeight(9.0f)));
    g.setColour(Colours::neonGreen.withAlpha(0.70f));
    g.drawText("DRY / WET", topRow, juce::Justification::centredLeft, false);

    const auto valStr = juce::String(static_cast<int>(dryWetSlider.getValue())) + "%";
    g.setFont(laf != nullptr ? laf->getSpaceMonoFont(10.0f, true)
                              : juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.setColour(Colours::white85);
    g.drawText(valStr, topRow, juce::Justification::centredRight, false);
}

void FooterPanel::drawDetailDock(juce::Graphics& g) const
{
    if (detailZone.getWidth() < 80)
        return;

    ZikadaLookAndFeel::drawDeviceDisplay(g, detailZone);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    auto inner   = detailZone.reduced(kInnerPad, 4);
    auto labelRow = inner.removeFromTop(kLabelH).toFloat();

    g.setFont(laf != nullptr ? laf->getVcrFont(9.0f)
                              : juce::Font(juce::FontOptions().withHeight(9.0f)));
    g.setColour(Colours::neonGreen.withAlpha(0.55f));
    g.drawText("STEP DETAIL", labelRow, juce::Justification::centredLeft, false);
    g.setColour(Colours::white.withAlpha(0.20f));
    g.drawText("SELECT A STEP TO EDIT", labelRow, juce::Justification::centredRight, false);

    inner.removeFromBottom(3);

    constexpr int numCells  = 16;
    constexpr int cellGap   = 2;
    const int     totalGaps = (numCells - 1) * cellGap;
    const int     cellW     = juce::jmax(4, (inner.getWidth() - totalGaps) / numCells);
    const int     startX    = inner.getX() + (inner.getWidth() - (cellW * numCells + totalGaps)) / 2;

    for (int i = 0; i < numCells; ++i)
    {
        const juce::Rectangle<float> cell(
            static_cast<float>(startX + i * (cellW + cellGap)),
            inner.getY() + 2.0f,
            static_cast<float>(cellW),
            inner.getHeight() - 4.0f);

        g.setColour(Colours::neonGreen.withAlpha((i % 4 == 0) ? 0.22f : 0.10f));
        g.fillRoundedRectangle(cell, 2.0f);
        g.setColour(Colours::neonGreen.withAlpha(0.28f));
        g.drawRoundedRectangle(cell, 2.0f, 0.8f);
    }
}

void FooterPanel::drawSignalModule(juce::Graphics& g) const
{
    const auto sf = signalZone.toFloat();
    g.setColour(Colours::panelRaised.withAlpha(0.30f));
    g.fillRoundedRectangle(sf, PanelMetrics::kInnerCorner);
    g.setColour(Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(sf.reduced(0.5f), PanelMetrics::kInnerCorner, 1.0f);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());
    g.setFont(laf != nullptr ? laf->getVcrFont(9.0f)
                              : juce::Font(juce::FontOptions().withHeight(9.0f)));
    g.setColour(Colours::white50);
    g.drawText("MIX MODE",  mixModeHeaderRect.toFloat(),    juce::Justification::centredLeft, false);
    g.drawText("OUTPUT",    outputGainHeaderRect.toFloat(),  juce::Justification::centredLeft, false);

    if (mixModeHeaderRect.getRight() > 0 && outputGainHeaderRect.getX() > 0)
    {
        const int sepX = (mixModeHeaderRect.getRight() + outputGainHeaderRect.getX()) / 2;
        ZikadaLookAndFeel::drawModuleSeparator(g, sepX,
            signalZone.getY() + signalZone.getHeight() / 5,
            signalZone.getHeight() * 3 / 5, false);
    }
}

void FooterPanel::resized()
{
    namespace PM = PanelMetrics;
    auto content = getLocalBounds().reduced(kHPad, kVPad);

    dryWetZone = content.removeFromLeft(kDryWetW);
    content.removeFromLeft(kZoneGap);
    signalZone = content.removeFromRight(kSignalW);
    content.removeFromRight(kZoneGap);
    detailZone = content;

    {
        auto inner = dryWetZone.reduced(kInnerPad, 4);
        inner.removeFromTop(kLabelH);
        dryWetSlider.setBounds(inner.withSizeKeepingCentre(inner.getWidth(), 24));
    }

    {
        auto sz = signalZone.reduced(kHPad, kVPad);

        bypassButton.setBounds(sz.removeFromRight(86).withSizeKeepingCentre(86, 28));
        sz.removeFromRight(8);

        const int halfW  = sz.getWidth() / 2 - 2;
        auto mixCol  = sz.removeFromLeft(halfW);
        sz.removeFromLeft(4);
        auto gainCol = sz;

        mixModeHeaderRect    = mixCol.removeFromTop(kLabelH);
        outputGainHeaderRect = gainCol.removeFromTop(kLabelH);

        mixModeLabel.setBounds(mixCol.withSizeKeepingCentre(mixCol.getWidth(), 20));
        outputGainLabel.setBounds(gainCol.withSizeKeepingCentre(gainCol.getWidth(), 20));
    }
}

}
