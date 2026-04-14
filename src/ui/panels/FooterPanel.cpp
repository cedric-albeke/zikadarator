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

    modModeButton.setClickingTogglesState(true);
    modModeButton.setColour(juce::TextButton::buttonColourId,   Colours::bgSurface);
    modModeButton.setColour(juce::TextButton::buttonOnColourId, Colours::neonGreen);
    modModeButton.setColour(juce::TextButton::textColourOffId,  Colours::white85);
    modModeButton.setColour(juce::TextButton::textColourOnId,   Colours::bgPrimary);
    modModeButton.onClick = [this]
    {
        modModeActive = modModeButton.getToggleState();
        applyModModeVisibility();
        repaint();
    };
    addAndMakeVisible(modModeButton);

    setupModulationControls();
}

void FooterPanel::setupModulationControls()
{
    for (int i = 0; i < kNumModSlots; ++i)
    {
        auto targetBtn = std::make_unique<juce::TextButton>("OFF");
        targetBtn->onClick = [this, i] { cycleModTarget(i); };
        targetBtn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        targetBtn->setColour(juce::TextButton::textColourOffId, Colours::white85);
        addChildComponent(targetBtn.get());
        modTargetButtons[i] = std::move(targetBtn);

        auto sourceBtn = std::make_unique<juce::TextButton>("STATIC");
        sourceBtn->onClick = [this, i] { cycleModSource(i); };
        sourceBtn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        sourceBtn->setColour(juce::TextButton::textColourOffId, Colours::white85);
        addChildComponent(sourceBtn.get());
        modSourceButtons[i] = std::move(sourceBtn);

        auto amountSlider = std::make_unique<juce::Slider>();
        amountSlider->setSliderStyle(juce::Slider::LinearHorizontal);
        amountSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        amountSlider->setRange(0.0, 100.0, 1.0);
        amountSlider->setValue(0.0);
        amountSlider->setColour(juce::Slider::thumbColourId, Colours::neonGreen);
        amountSlider->setColour(juce::Slider::trackColourId, Colours::white50);
        amountSlider->setColour(juce::Slider::backgroundColourId, Colours::bgSurface);
        amountSlider->onValueChange = [this] { notifyStepDataChanged(); };
        addChildComponent(amountSlider.get());
        modAmountSliders[i] = std::move(amountSlider);

        auto paramSlider = std::make_unique<juce::Slider>();
        paramSlider->setSliderStyle(juce::Slider::LinearHorizontal);
        paramSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        paramSlider->setRange(0.0, 7.0, 1.0);
        paramSlider->setValue(0.0);
        paramSlider->setColour(juce::Slider::thumbColourId, Colours::neonGreen);
        paramSlider->setColour(juce::Slider::trackColourId, Colours::white50);
        paramSlider->setColour(juce::Slider::backgroundColourId, Colours::bgSurface);
        paramSlider->onValueChange = [this] { notifyStepDataChanged(); };
        addChildComponent(paramSlider.get());
        modParamSliders[i] = std::move(paramSlider);

        auto paramLabel = std::make_unique<juce::Label>();
        paramLabel->setText("SHAPE", juce::dontSendNotification);
        paramLabel->setJustificationType(juce::Justification::centred);
        paramLabel->setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        paramLabel->setColour(juce::Label::textColourId, Colours::white50);
        addChildComponent(paramLabel.get());
        modParamLabels[i] = std::move(paramLabel);
    }

    applyModModeVisibility();
}

void FooterPanel::applyModModeVisibility()
{
    for (auto& knob : stepKnobs)
        knob->setVisible(!modModeActive);

    for (int i = 0; i < kNumModSlots; ++i)
    {
        modTargetButtons[i]->setVisible(modModeActive);
        modSourceButtons[i]->setVisible(modModeActive);
        modAmountSliders[i]->setVisible(modModeActive);
        modParamSliders[i]->setVisible(modModeActive);
        modParamLabels[i]->setVisible(modModeActive);
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

    if (modModeActive)
        g.drawText("MODULATION", labelRow, juce::Justification::centredLeft, false);
    else
        g.drawText("STEP DETAIL", labelRow, juce::Justification::centredLeft, false);

    if (hasSelection)
    {
        const auto infoStr = selectedLaneName + "  /  STEP " + juce::String(selectedStep + 1);
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(10.0f, true)
                                  : juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold")));
        g.setColour(Colours::neonGreen.withAlpha(0.90f));
        g.drawText(infoStr, labelRow, juce::Justification::centredRight, false);
    }
    else if (!modModeActive)
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
        auto labelRow = inner.removeFromTop(kLabelH);
        modModeButton.setBounds(labelRow.removeFromRight(50).withSizeKeepingCentre(50, 18));

        constexpr int numKnobs = 7;
        const int     totalGaps = (numKnobs - 1) * kKnobGap;
        const int     knobW     = (inner.getWidth() - totalGaps) / numKnobs;

        for (int i = 0; i < numKnobs; ++i)
        {
            const int kx = inner.getX() + i * (knobW + kKnobGap);
            stepKnobs[i]->setBounds(kx, inner.getY(), knobW, inner.getHeight());
        }

        const int slotGap = 8;
        const int slotW = (inner.getWidth() - (kNumModSlots - 1) * slotGap) / kNumModSlots;
        const int btnH = 20;
        const int sldH = 18;
        const int lblH = 12;
        const int gap = 3;

        for (int i = 0; i < kNumModSlots; ++i)
        {
            int sx = inner.getX() + i * (slotW + slotGap);
            int sy = inner.getY();
            modTargetButtons[i]->setBounds(sx, sy, slotW, btnH);
            sy += btnH + gap;
            modSourceButtons[i]->setBounds(sx, sy, slotW, btnH);
            sy += btnH + gap;
            modAmountSliders[i]->setBounds(sx, sy, slotW, sldH);
            sy += sldH + gap;
            modParamLabels[i]->setBounds(sx, sy, slotW, lblH);
            sy += lblH + gap;
            modParamSliders[i]->setBounds(sx, sy, slotW, std::max(4, inner.getBottom() - sy));
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

    updateModulationControlsFromData(data.modulation);
    updatingFromState = false;

    for (auto& knob : stepKnobs)
        knob->setVisible(!modModeActive);

    applyModModeVisibility();
    repaint();
}

void FooterPanel::updateModulationControlsFromData(const ModulationData& modData)
{
    for (int i = 0; i < kNumModSlots; ++i)
    {
        const auto& slot = modData.slots[static_cast<size_t>(i)];
        modTargetButtons[i]->setButtonText(targetToString(slot.target));
        modSourceButtons[i]->setButtonText(sourceToString(slot.source));
        modAmountSliders[i]->setValue(static_cast<double>(slot.amount) * 100.0);
        updateModParamLabel(i);

        switch (slot.source)
        {
            case ModulationSource::Motion:
                modParamSliders[i]->setRange(0.0, 7.0, 1.0);
                modParamSliders[i]->setValue(static_cast<double>(slot.motionShape));
                break;
            case ModulationSource::EnvFollower:
                modParamSliders[i]->setRange(1.0, 500.0, 1.0);
                modParamSliders[i]->setValue(static_cast<double>(slot.envAttack));
                break;
            case ModulationSource::Random:
                modParamSliders[i]->setRange(1.0, 16.0, 1.0);
                modParamSliders[i]->setValue(static_cast<double>(slot.randomRate));
                break;
            default:
                modParamSliders[i]->setRange(0.0, 1.0, 0.01);
                modParamSliders[i]->setValue(0.0);
                break;
        }
    }
}

void FooterPanel::cycleModTarget(int slot)
{
    if (!hasSelection || updatingFromState)
        return;

    int current = static_cast<int>(targetFromString(modTargetButtons[slot]->getButtonText()));
    int mapped = current + 1;
    mapped = (mapped + 1) % (static_cast<int>(ModulationTarget::NumTargets) + 1);
    current = mapped - 1;

    auto t = static_cast<ModulationTarget>(current);
    modTargetButtons[slot]->setButtonText(targetToString(t));
    notifyStepDataChanged();
}

void FooterPanel::cycleModSource(int slot)
{
    if (!hasSelection || updatingFromState)
        return;

    int current = static_cast<int>(sourceFromString(modSourceButtons[slot]->getButtonText()));
    current = (current + 1) % static_cast<int>(ModulationSource::NumSources);

    auto s = static_cast<ModulationSource>(current);
    modSourceButtons[slot]->setButtonText(sourceToString(s));
    updateModParamLabel(slot);

    switch (s)
    {
        case ModulationSource::Motion:
            modParamSliders[slot]->setRange(0.0, 7.0, 1.0);
            modParamSliders[slot]->setValue(0.0);
            break;
        case ModulationSource::EnvFollower:
            modParamSliders[slot]->setRange(1.0, 500.0, 1.0);
            modParamSliders[slot]->setValue(10.0);
            break;
        case ModulationSource::Random:
            modParamSliders[slot]->setRange(1.0, 16.0, 1.0);
            modParamSliders[slot]->setValue(1.0);
            break;
        default:
            modParamSliders[slot]->setRange(0.0, 1.0, 0.01);
            modParamSliders[slot]->setValue(0.0);
            break;
    }

    notifyStepDataChanged();
}

void FooterPanel::updateModParamLabel(int slot)
{
    auto s = sourceFromString(modSourceButtons[slot]->getButtonText());
    switch (s)
    {
        case ModulationSource::Motion:
            modParamLabels[slot]->setText("SHAPE", juce::dontSendNotification);
            break;
        case ModulationSource::EnvFollower:
            modParamLabels[slot]->setText("ATTACK", juce::dontSendNotification);
            break;
        case ModulationSource::Random:
            modParamLabels[slot]->setText("RATE", juce::dontSendNotification);
            break;
        default:
            modParamLabels[slot]->setText("", juce::dontSendNotification);
            break;
    }
}

juce::String FooterPanel::targetToString(ModulationTarget t)
{
    switch (t)
    {
        case ModulationTarget::FilterCutoff:    return "CUTOFF";
        case ModulationTarget::FilterResonance: return "RESON";
        case ModulationTarget::DelayTime:       return "DELAY";
        case ModulationTarget::DelayFeedback:   return "FEEDBK";
        case ModulationTarget::DelayMix:        return "MIX";
        case ModulationTarget::Volume:          return "VOL";
        case ModulationTarget::Pan:             return "PAN";
        default:                                return "OFF";
    }
}

ModulationTarget FooterPanel::targetFromString(const juce::String& s)
{
    if (s == "CUTOFF")   return ModulationTarget::FilterCutoff;
    if (s == "RESON")    return ModulationTarget::FilterResonance;
    if (s == "DELAY")    return ModulationTarget::DelayTime;
    if (s == "FEEDBK")   return ModulationTarget::DelayFeedback;
    if (s == "MIX")      return ModulationTarget::DelayMix;
    if (s == "VOL")      return ModulationTarget::Volume;
    if (s == "PAN")      return ModulationTarget::Pan;
    return ModulationTarget::None;
}

juce::String FooterPanel::sourceToString(ModulationSource s)
{
    switch (s)
    {
        case ModulationSource::Motion:       return "MOTION";
        case ModulationSource::EnvFollower:  return "ENV";
        case ModulationSource::Random:       return "RAND";
        default:                             return "STATIC";
    }
}

ModulationSource FooterPanel::sourceFromString(const juce::String& s)
{
    if (s == "MOTION")  return ModulationSource::Motion;
    if (s == "ENV")     return ModulationSource::EnvFollower;
    if (s == "RAND")    return ModulationSource::Random;
    return ModulationSource::Static;
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
    data.modulation      = readModulationDataFromControls();

    onStepDataChanged(selectedLane, selectedStep, data);
}

ModulationData FooterPanel::readModulationDataFromControls() const
{
    ModulationData modData;
    for (int i = 0; i < kNumModSlots; ++i)
    {
        auto& slot = modData.slots[static_cast<size_t>(i)];
        slot.target = targetFromString(modTargetButtons[i]->getButtonText());
        slot.source = sourceFromString(modSourceButtons[i]->getButtonText());
        slot.amount = static_cast<float>(modAmountSliders[i]->getValue()) / 100.0f;

        switch (slot.source)
        {
            case ModulationSource::Motion:
                slot.motionShape = static_cast<int>(modParamSliders[i]->getValue());
                break;
            case ModulationSource::EnvFollower:
                slot.envAttack = static_cast<float>(modParamSliders[i]->getValue());
                break;
            case ModulationSource::Random:
                slot.randomRate = static_cast<int>(modParamSliders[i]->getValue());
                break;
            default:
                break;
        }
    }
    return modData;
}

} // namespace zikada
