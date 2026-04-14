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
    constexpr int kKnobGap  = 4;

    constexpr std::array<double, 7> kKnobMin  = { 20.0, 0.1,   0.0, 0.0, 0.0, 0.0, -1.0 };
    constexpr std::array<double, 7> kKnobMax  = { 20000.0, 10.0, 1.0, 1.0, 1.0, 2.0,  1.0 };
    constexpr std::array<double, 7> kKnobDef  = { 2000.0, 0.707, 0.25, 0.3, 0.5, 1.0,  0.0 };
    constexpr std::array<const char*, 7> kKnobLabel = {
        "CUTOFF", "RESON", "DELAY", "FEEDBK", "MIX", "VOL", "PAN"
    };
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

    for (int i = 0; i < 7; ++i)
    {
        auto knob = std::make_unique<Knob>();
        knob->setRange(kKnobMin[i], kKnobMax[i]);
        knob->setDefaultValue(kKnobDef[i]);
        knob->setValue(kKnobDef[i]);
        knob->setLabel(kKnobLabel[i]);
        knob->setColour(Colours::neonGreen);
        knob->onValueChange = [this] { notifyStepDataChanged(); };
        addChildComponent(knob.get());
        stepKnobs[i] = std::move(knob);
    }
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

    auto inner    = detailZone.reduced(kInnerPad, 4);
    auto labelRow = inner.removeFromTop(kLabelH).toFloat();

    g.setFont(laf != nullptr ? laf->getVcrFont(9.0f)
                              : juce::Font(juce::FontOptions().withHeight(9.0f)));
    g.setColour(Colours::neonGreen.withAlpha(0.55f));
    g.drawText("STEP DETAIL", labelRow, juce::Justification::centredLeft, false);

    if (hasSelection)
    {
        const auto infoStr = selectedLaneName + "  /  STEP " + juce::String(selectedStep + 1);
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(10.0f, true)
                                  : juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold")));
        g.setColour(Colours::neonGreen.withAlpha(0.90f));
        g.drawText(infoStr, labelRow, juce::Justification::centredRight, false);
    }
    else
    {
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(9.0f)
                                  : juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.setColour(Colours::white.withAlpha(0.20f));
        g.drawText("SELECT A STEP TO EDIT", labelRow, juce::Justification::centredRight, false);

        inner.removeFromBottom(3);
        constexpr int numHints  = 7;
        const int     totalGaps = (numHints - 1) * kKnobGap;
        const int     cellW     = juce::jmax(4, (inner.getWidth() - totalGaps) / numHints);

        for (int i = 0; i < numHints; ++i)
        {
            const juce::Rectangle<float> cell(
                static_cast<float>(inner.getX() + i * (cellW + kKnobGap)),
                inner.getY() + 2.0f,
                static_cast<float>(cellW),
                inner.getHeight() - 4.0f);

            g.setColour(Colours::neonGreen.withAlpha(0.08f));
            g.fillRoundedRectangle(cell, 3.0f);
            g.setColour(Colours::neonGreen.withAlpha(0.15f));
            g.drawRoundedRectangle(cell, 3.0f, 0.8f);
        }
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

    {
        auto inner = detailZone.reduced(kInnerPad, 4);
        inner.removeFromTop(kLabelH);
        inner.removeFromBottom(3);

        constexpr int numKnobs  = 7;
        const int     totalGaps = (numKnobs - 1) * kKnobGap;
        const int     knobW     = (inner.getWidth() - totalGaps) / numKnobs;

        for (int i = 0; i < numKnobs; ++i)
        {
            const int kx = inner.getX() + i * (knobW + kKnobGap);
            stepKnobs[i]->setBounds(kx, inner.getY(), knobW, inner.getHeight());
        }
    }
}

void FooterPanel::setSelectedStep(int lane, int step, const StepData& data, const juce::String& laneName)
{
    hasSelection     = true;
    selectedLane     = lane;
    selectedStep     = step;
    selectedLaneName = laneName;

    const auto laneColour = laneInfos[lane].colour;

    updatingFromState = true;
    stepKnobs[0]->setColour(laneColour);
    stepKnobs[0]->setValue(static_cast<double>(data.filterCutoff));
    stepKnobs[1]->setColour(laneColour);
    stepKnobs[1]->setValue(static_cast<double>(data.filterResonance));
    stepKnobs[2]->setColour(laneColour.withAlpha(0.80f).brighter(0.15f));
    stepKnobs[2]->setValue(static_cast<double>(data.delayTime));
    stepKnobs[3]->setColour(laneColour.withAlpha(0.80f).brighter(0.15f));
    stepKnobs[3]->setValue(static_cast<double>(data.delayFeedback));
    stepKnobs[4]->setColour(Colours::neonGreen);
    stepKnobs[4]->setValue(static_cast<double>(data.delayMix));
    stepKnobs[5]->setColour(Colours::neonGreen);
    stepKnobs[5]->setValue(static_cast<double>(data.volume));
    stepKnobs[6]->setColour(Colours::neonGreen.withAlpha(0.80f));
    stepKnobs[6]->setValue(static_cast<double>(data.pan));
    updatingFromState = false;

    for (auto& knob : stepKnobs)
        knob->setVisible(true);

    repaint();
}

void FooterPanel::notifyStepDataChanged()
{
    if (updatingFromState || !hasSelection || !onStepDataChanged)
        return;

    StepData data;
    data.filterCutoff    = static_cast<float>(stepKnobs[0]->getValue());
    data.filterResonance = static_cast<float>(stepKnobs[1]->getValue());
    data.delayTime       = static_cast<float>(stepKnobs[2]->getValue());
    data.delayFeedback   = static_cast<float>(stepKnobs[3]->getValue());
    data.delayMix        = static_cast<float>(stepKnobs[4]->getValue());
    data.volume          = static_cast<float>(stepKnobs[5]->getValue());
    data.pan             = static_cast<float>(stepKnobs[6]->getValue());

    onStepDataChanged(selectedLane, selectedStep, data);
}

} // namespace zikada
