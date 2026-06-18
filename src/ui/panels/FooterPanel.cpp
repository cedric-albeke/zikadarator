#include "ui/panels/FooterPanel.h"

namespace zikada {

namespace {
    constexpr int kDryWetW  = 180;
    constexpr int kSignalW  = 300;
    constexpr int kZoneGap  = 4;
    constexpr int kHPad     = 6;
    constexpr int kVPad     = 2;
    constexpr int kLabelH   = 10;
    constexpr int kInnerPad = 6;
    constexpr int kKnobGap  = 3;
    constexpr int kDetailGroupHeaderH = 12;
    constexpr int kDetailGroupGap = 3;

    constexpr std::array<double, 7> kKnobMin  = { 20.0, 0.1,   0.0, 0.0, 0.0, 0.0, -1.0 };
    constexpr std::array<double, 7> kKnobMax  = { 20000.0, 10.0, 1.0, 1.0, 1.0, 2.0,  1.0 };
    constexpr std::array<double, 7> kKnobDef  = { 2000.0, 0.707, 0.25, 0.3, 0.5, 1.0,  0.0 };
    constexpr std::array<const char*, 7> kKnobLabel = {
        "CUTOFF", "RESON", "DELAY", "FEEDBK", "MIX", "VOL", "PAN"
    };
    constexpr std::array<double, 7> kLoopKnobMin  = { 0.25, 0.25, 0.0, 0.0, 0.0, 0.0, -1.0 };
    constexpr std::array<double, 7> kLoopKnobMax  = { 4.0,  4.0,  1.0, 1.0, 1.0, 2.0,  1.0 };
    constexpr std::array<double, 7> kLoopKnobDef  = { 0.5,  1.0,  0.0, 0.45, 0.65, 1.0, 0.0 };
    constexpr std::array<const char*, 7> kLoopKnobLabel = {
        "LEN", "RATE", "REV", "FADE", "MIX", "VOL", "PAN"
    };

    juce::Rectangle<int> getKnobGroupBounds(juce::Rectangle<int> area, int firstKnob, int knobCount)
    {
        constexpr int numKnobs = 7;
        const int totalGaps = (numKnobs - 1) * kKnobGap;
        const int knobW = juce::jmax(4, (area.getWidth() - totalGaps) / numKnobs);
        const int x = area.getX() + firstKnob * (knobW + kKnobGap);
        const int w = knobCount * knobW + (knobCount - 1) * kKnobGap;
        return { x, area.getY(), w, area.getHeight() };
    }

    void debugFooterLog(const juce::String& message)
    {
        juce::Logger::writeToLog("[ZIKADARATOR] FooterPanel " + message);

#if JUCE_DEBUG
        DBG("[ZIKADARATOR] FooterPanel " + message);
#endif
    }
}

FooterPanel::FooterPanel(juce::AudioProcessorValueTreeState& valueTreeState)
    : apvts(valueTreeState)
{
    dryWetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dryWetSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(100.0);
    dryWetSlider.onValueChange = [this] { repaint(); };
    addAndMakeVisible(dryWetSlider);
    dryWetAttachment = std::make_unique<
        juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterIDs::dryWet, dryWetSlider);

    mixModeLabel.setText("LINEAR", juce::dontSendNotification);
    mixModeLabel.setJustificationType(juce::Justification::centred);
    mixModeLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    mixModeLabel.setColour(juce::Label::textColourId, Colours::white);
    addAndMakeVisible(mixModeLabel);

    outputGainLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    outputGainLabel.setJustificationType(juce::Justification::centred);
    outputGainLabel.setColour(juce::Label::textColourId, Colours::white);
    outputGainLabel.setText("0.0 dB", juce::dontSendNotification);
    addAndMakeVisible(outputGainLabel);

    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId,   Colours::bgSurface);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,  Colours::neonGreen);
    bypassButton.setColour(juce::TextButton::textColourOffId,   Colours::white50);
    bypassButton.setColour(juce::TextButton::textColourOnId,    Colours::bgPrimary);
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<
        juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParameterIDs::bypass, bypassButton);

    stepResLabel.setText("STEP RES", juce::dontSendNotification);
    stepResLabel.setJustificationType(juce::Justification::centredLeft);
    stepResLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    stepResLabel.setColour(juce::Label::textColourId, Colours::white50);
    addAndMakeVisible(stepResLabel);

    stepResolutionBox.addItemList({"1/16", "1/8", "1/4", "1/2"}, 1);
    stepResolutionBox.setColour(juce::ComboBox::backgroundColourId,    Colours::bgSurface);
    stepResolutionBox.setColour(juce::ComboBox::textColourId,           Colours::white50);
    stepResolutionBox.setColour(juce::ComboBox::outlineColourId,        Colours::white50.withAlpha(0.35f));
    stepResolutionBox.setColour(juce::ComboBox::arrowColourId,          Colours::white50);
    stepResolutionBox.setColour(juce::ComboBox::focusedOutlineColourId, Colours::neonGreen);
    stepResolutionBox.onChange = [this] { syncInlineControlState(); };
    addChildComponent(stepResolutionBox);

    stepResolutionButton.setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
    stepResolutionButton.setColour(juce::TextButton::buttonOnColourId, Colours::bgHover.brighter(0.06f));
    stepResolutionButton.setColour(juce::TextButton::textColourOffId, Colours::white50);
    stepResolutionButton.setColour(juce::TextButton::textColourOnId, Colours::white);
    stepResolutionButton.onClick = [this] { cycleStepResolution(); };
    addAndMakeVisible(stepResolutionButton);

    stepResolutionAttachment = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParameterIDs::stepResolution, stepResolutionBox);

    for (int i = 0; i < 7; ++i)
    {
        auto knob = std::make_unique<Knob>();
        knob->setRange(kKnobMin[i], kKnobMax[i]);
        knob->setDefaultValue(kKnobDef[i]);
        knob->setValue(kKnobDef[i]);
        knob->setLabel(kKnobLabel[i]);
        knob->setColour(Colours::neonGreen);
        knob->onValueChange = [this] { notifySlotDataChanged(); };
        addChildComponent(knob.get());
        stepKnobs[i] = std::move(knob);
    }
    applyKnobConfigForLane(0);

    modModeButton.setClickingTogglesState(true);
    modModeButton.setColour(juce::TextButton::buttonColourId,   Colours::bgSurface);
    modModeButton.setColour(juce::TextButton::buttonOnColourId, Colours::neonGreen);
    modModeButton.setColour(juce::TextButton::textColourOffId,  Colours::white50);
    modModeButton.setColour(juce::TextButton::textColourOnId,   Colours::bgPrimary);
    modModeButton.onClick = [this]
    {
        modModeActive = modModeButton.getToggleState();
        applyModModeVisibility();
        resized();
        repaint();
    };
    addAndMakeVisible(modModeButton);

    setupModulationControls();
    refreshGlobalControlLabels();
    syncInlineControlState();
}

void FooterPanel::applyKnobConfigForLane(int lane)
{
    const bool loopLane = lane == 1;
    const auto& mins = loopLane ? kLoopKnobMin : kKnobMin;
    const auto& maxes = loopLane ? kLoopKnobMax : kKnobMax;
    const auto& defaults = loopLane ? kLoopKnobDef : kKnobDef;
    const auto& labels = loopLane ? kLoopKnobLabel : kKnobLabel;

    for (int i = 0; i < 7; ++i)
    {
        stepKnobs[i]->setRange(mins[static_cast<size_t>(i)], maxes[static_cast<size_t>(i)]);
        stepKnobs[i]->setDefaultValue(defaults[static_cast<size_t>(i)]);
        stepKnobs[i]->setLabel(labels[static_cast<size_t>(i)]);
        stepKnobs[i]->setScaleMode(KnobScaleMode::Linear);
    }

    if (loopLane)
    {
        stepKnobs[0]->setDisplayMode(KnobDisplayMode::Gain);
        stepKnobs[1]->setDisplayMode(KnobDisplayMode::Gain);
        stepKnobs[2]->setDisplayMode(KnobDisplayMode::Percent);
        stepKnobs[3]->setDisplayMode(KnobDisplayMode::Percent);
    }
    else
    {
        stepKnobs[0]->setDisplayMode(KnobDisplayMode::Hertz);
        stepKnobs[0]->setScaleMode(KnobScaleMode::Logarithmic);
        stepKnobs[1]->setDisplayMode(KnobDisplayMode::Decimal);
        stepKnobs[2]->setDisplayMode(KnobDisplayMode::Seconds);
        stepKnobs[3]->setDisplayMode(KnobDisplayMode::Percent);
    }

    stepKnobs[4]->setDisplayMode(KnobDisplayMode::Percent);
    stepKnobs[5]->setDisplayMode(KnobDisplayMode::Gain);
    stepKnobs[6]->setDisplayMode(KnobDisplayMode::Pan);
}

void FooterPanel::refreshGlobalControlLabels()
{
    if (auto* mixMode = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(ParameterIDs::mixMode)))
        mixModeLabel.setText(mixMode->getCurrentChoiceName().toUpperCase(), juce::dontSendNotification);

    if (auto* outputGain = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParameterIDs::outputGain)))
    {
        const auto value = outputGain->get();
        const auto text = juce::String(value, 1) + " dB";
        outputGainLabel.setText(text, juce::dontSendNotification);
    }

    syncInlineControlState();
}

void FooterPanel::syncInlineControlState()
{
    const auto text = stepResolutionBox.getText().isNotEmpty() ? stepResolutionBox.getText() : juce::String("1/8");
    stepResolutionButton.setButtonText(text.toUpperCase());
}

void FooterPanel::cycleStepResolution()
{
    const auto numItems = stepResolutionBox.getNumItems();
    if (numItems <= 0)
        return;

    const auto currentIndex = juce::jmax(0, stepResolutionBox.getSelectedItemIndex());
    const auto nextIndex = (currentIndex + 1) % numItems;
    stepResolutionBox.setSelectedItemIndex(nextIndex, juce::sendNotificationSync);
    debugFooterLog("cycleStepResolution -> " + stepResolutionBox.getText());
}

void FooterPanel::setupModulationControls()
{
    for (int i = 0; i < kNumModSlots; ++i)
    {
        auto targetBtn = std::make_unique<juce::TextButton>("OFF");
        targetBtn->onClick = [this, i] { cycleModTarget(i); };
        targetBtn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface.withAlpha(0.50f));
        targetBtn->setColour(juce::TextButton::textColourOffId, Colours::white50);
        addChildComponent(targetBtn.get());
        modTargetButtons[i] = std::move(targetBtn);

        auto sourceBtn = std::make_unique<juce::TextButton>("STATIC");
        sourceBtn->onClick = [this, i] { cycleModSource(i); };
        sourceBtn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface.withAlpha(0.50f));
        sourceBtn->setColour(juce::TextButton::textColourOffId, Colours::white50);
        addChildComponent(sourceBtn.get());
        modSourceButtons[i] = std::move(sourceBtn);

        auto amountSlider = std::make_unique<juce::Slider>();
        amountSlider->setSliderStyle(juce::Slider::LinearHorizontal);
        amountSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        amountSlider->setRange(0.0, 100.0, 1.0);
        amountSlider->setValue(0.0);
        amountSlider->setColour(juce::Slider::thumbColourId, Colours::neonGreen);
        amountSlider->setColour(juce::Slider::trackColourId, Colours::white50);
        amountSlider->setColour(juce::Slider::backgroundColourId, Colours::bgSurface.withAlpha(0.50f));
        amountSlider->onValueChange = [this] { notifySlotDataChanged(); };
        addChildComponent(amountSlider.get());
        modAmountSliders[i] = std::move(amountSlider);

        auto paramSlider = std::make_unique<juce::Slider>();
        paramSlider->setSliderStyle(juce::Slider::LinearHorizontal);
        paramSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        paramSlider->setRange(0.0, 7.0, 1.0);
        paramSlider->setValue(0.0);
        paramSlider->setColour(juce::Slider::thumbColourId, Colours::neonGreen);
        paramSlider->setColour(juce::Slider::trackColourId, Colours::white50);
        paramSlider->setColour(juce::Slider::backgroundColourId, Colours::bgSurface.withAlpha(0.50f));
        paramSlider->onValueChange = [this] { notifySlotDataChanged(); };
        addChildComponent(paramSlider.get());
        modParamSliders[i] = std::move(paramSlider);

        auto paramLabel = std::make_unique<juce::Label>();
        paramLabel->setText("SHAPE", juce::dontSendNotification);
        paramLabel->setJustificationType(juce::Justification::centred);
        paramLabel->setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
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

    g.setFont(laf != nullptr ? laf->getVcrFont(11.0f)
                              : juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.setColour(Colours::white50);
    g.drawText("DRY / WET", topRow, juce::Justification::centredLeft, false);

    const auto valStr = juce::String(static_cast<int>(dryWetSlider.getValue())) + "%";
    g.setFont(laf != nullptr ? laf->getSpaceMonoFont(12.0f, true)
                              : juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.setColour(Colours::white);
    g.drawText(valStr, topRow, juce::Justification::centredRight, false);
}

void FooterPanel::drawDetailDock(juce::Graphics& g) const
{
    if (detailZone.getWidth() < 80)
        return;

    ZikadaLookAndFeel::drawDeviceDisplay(g, detailZone);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    auto inner    = detailZone.reduced(kInnerPad, 4);

    if (hasSelection && selectedLane >= 0)
    {
        const auto laneCol = laneInfos[selectedLane].colour;
        g.setColour(laneCol.withAlpha(0.90f));
        g.fillRoundedRectangle(inner.toFloat().withWidth(3.5f).withTrimmedTop(6.0f).withTrimmedBottom(6.0f), 1.5f);
    }

    auto labelRow = inner.removeFromTop(kLabelH).toFloat();

    g.setFont(laf != nullptr ? laf->getVcrFont(11.0f)
                              : juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.setColour(Colours::white50);

    if (modModeActive)
        g.drawText("MODULATION", labelRow, juce::Justification::centredLeft, false);
    else
        g.drawText("STEP DETAIL", labelRow, juce::Justification::centredLeft, false);

    auto infoRow = labelRow;
    infoRow.removeFromRight(56.0f);

    if (hasSelection)
    {
        const auto infoStr = selectedLaneName + "  /  U" + juce::String(selectedSlot + 1);
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(12.0f, true)
                                  : juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
        g.setColour(Colours::white);
        g.drawText(infoStr, infoRow, juce::Justification::centredRight, false);
    }
    else if (!modModeActive)
    {
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(11.0f)
                                  : juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.setColour(Colours::white50);
        g.drawText("SELECT A STEP TO EDIT", infoRow, juce::Justification::centredRight, false);

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

    if (!modModeActive)
        drawDetailGroupHeaders(g, inner);
}

void FooterPanel::drawDetailGroupHeaders(juce::Graphics& g, juce::Rectangle<int> knobArea) const
{
    knobArea.removeFromTop(kDetailGroupGap);
    auto headerArea = knobArea.removeFromTop(kDetailGroupHeaderH);
    if (headerArea.getWidth() < 80)
        return;

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());
    auto drawGroupHeader = [&](juce::Rectangle<int> bounds, const juce::String& label)
    {
        auto b = bounds.reduced(1, 0).toFloat();
        g.setColour(Colours::bgSurface.withAlpha(0.38f));
        g.fillRoundedRectangle(b, 2.0f);
        g.setColour(Colours::white.withAlpha(0.07f));
        g.drawRoundedRectangle(b, 2.0f, 0.8f);
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(9.0f, true)
                                  : juce::Font(juce::FontOptions().withHeight(9.0f).withStyle("Bold")));
        g.setColour(Colours::white50);
        g.drawText(label, b, juce::Justification::centred, false);
    };

    drawGroupHeader(getKnobGroupBounds(headerArea, 0, 2), selectedLane == 1 ? "LOOP" : "TONE");
    drawGroupHeader(getKnobGroupBounds(headerArea, 2, 2), selectedLane == 1 ? "TEXTURE" : "SPACE");
    drawGroupHeader(getKnobGroupBounds(headerArea, 4, 3), "OUTPUT");
}

void FooterPanel::drawSignalModule(juce::Graphics& g) const
{
    ZikadaLookAndFeel::drawDeviceDisplay(g, signalZone);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());
    g.setFont(laf != nullptr ? laf->getVcrFont(11.0f)
                              : juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.setColour(Colours::white50);
    g.drawText("MIX MODE",  mixModeHeaderRect.toFloat(),    juce::Justification::centredLeft, false);
    g.drawText("OUTPUT",    outputGainHeaderRect.toFloat(),  juce::Justification::centredLeft, false);
    g.drawText("STEP RES",  stepResHeaderRect.toFloat(),    juce::Justification::centredLeft, false);

    if (mixModeHeaderRect.getRight() > 0 && outputGainHeaderRect.getX() > 0)
    {
        const int sep1 = (mixModeHeaderRect.getRight() + outputGainHeaderRect.getX()) / 2;
        ZikadaLookAndFeel::drawModuleSeparator(g, sep1,
            signalZone.getY() + signalZone.getHeight() / 5,
            signalZone.getHeight() * 3 / 5, false);
    }

    if (outputGainHeaderRect.getRight() > 0 && stepResHeaderRect.getX() > 0)
    {
        const int sep2 = (outputGainHeaderRect.getRight() + stepResHeaderRect.getX()) / 2;
        ZikadaLookAndFeel::drawModuleSeparator(g, sep2,
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

        const int colGap = 4;
        const int colW   = (sz.getWidth() - 2 * colGap) / 3;

        auto mixCol  = sz.removeFromLeft(colW);
        sz.removeFromLeft(colGap);
        auto gainCol = sz.removeFromLeft(colW);
        sz.removeFromLeft(colGap);
        auto resCol  = sz;

        mixModeHeaderRect    = mixCol.removeFromTop(kLabelH);
        outputGainHeaderRect = gainCol.removeFromTop(kLabelH);
        stepResHeaderRect    = resCol.removeFromTop(kLabelH);

        mixModeLabel.setBounds(mixCol.withSizeKeepingCentre(mixCol.getWidth(), 20));
        outputGainLabel.setBounds(gainCol.withSizeKeepingCentre(gainCol.getWidth(), 20));

        stepResLabel.setBounds(stepResHeaderRect);
        stepResolutionButton.setBounds(resCol.withSizeKeepingCentre(resCol.getWidth(), 20));
        stepResolutionBox.setBounds(0, 0, 0, 0);
    }

    {
        auto inner = detailZone.reduced(kInnerPad, 4);
        auto labelRow = inner.removeFromTop(kLabelH);
        modModeButton.setBounds(labelRow.removeFromRight(50).withSizeKeepingCentre(50, 18));
        if (!modModeActive)
        {
            inner.removeFromTop(kDetailGroupGap);
            inner.removeFromTop(kDetailGroupHeaderH);
            inner.removeFromTop(kDetailGroupGap);
        }

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
        const int btnH  = inner.getHeight() * 20 / 100;
        const int sldH  = inner.getHeight() * 14 / 100;
        const int lblH  = 10;
        const int gap   = 3;

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

void FooterPanel::setSelectedSlot(int lane, int slot, const UserSlotData& data, const juce::String& laneName)
{
    hasSelection     = true;
    selectedLane     = lane;
    selectedSlot     = slot;
    selectedLaneName = laneName;

    const auto laneColour = laneInfos[lane].colour;
    auto displayData = data;
    const bool legacyLoopSlot = lane == 1
                             && (data.filterCutoff < 0.25f || data.filterCutoff > 4.0f
                                 || data.filterResonance < 0.25f || data.filterResonance > 4.0f);

    if (legacyLoopSlot)
    {
        displayData.filterCutoff = 0.5f;
        displayData.filterResonance = 1.0f;
        displayData.delayTime = 0.0f;
        displayData.delayFeedback = 0.45f;
        displayData.delayMix = juce::jlimit(0.0f, 1.0f, data.delayMix);
    }

    updatingFromState = true;
    applyKnobConfigForLane(lane);
    stepKnobs[0]->setColour(laneColour);
    stepKnobs[0]->setValue(static_cast<double>(displayData.filterCutoff));
    stepKnobs[1]->setColour(laneColour);
    stepKnobs[1]->setValue(static_cast<double>(displayData.filterResonance));
    stepKnobs[2]->setColour(laneColour.withAlpha(0.80f).brighter(0.15f));
    stepKnobs[2]->setValue(static_cast<double>(displayData.delayTime));
    stepKnobs[3]->setColour(laneColour.withAlpha(0.80f).brighter(0.15f));
    stepKnobs[3]->setValue(static_cast<double>(displayData.delayFeedback));
    stepKnobs[4]->setColour(Colours::neonGreen);
    stepKnobs[4]->setValue(static_cast<double>(displayData.delayMix));
    stepKnobs[5]->setColour(Colours::neonGreen);
    stepKnobs[5]->setValue(static_cast<double>(displayData.volume));
    stepKnobs[6]->setColour(Colours::neonGreen.withAlpha(0.80f));
    stepKnobs[6]->setValue(static_cast<double>(displayData.pan));

    updateModulationControlsFromData(data.modulation);
    updatingFromState = false;

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
    notifySlotDataChanged();
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

    notifySlotDataChanged();
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

void FooterPanel::notifySlotDataChanged()
{
    if (updatingFromState || !hasSelection || !onSlotDataChanged)
        return;

    UserSlotData data;
    data.filterCutoff    = static_cast<float>(stepKnobs[0]->getValue());
    data.filterResonance = static_cast<float>(stepKnobs[1]->getValue());
    data.delayTime       = static_cast<float>(stepKnobs[2]->getValue());
    data.delayFeedback   = static_cast<float>(stepKnobs[3]->getValue());
    data.delayMix        = static_cast<float>(stepKnobs[4]->getValue());
    data.volume          = static_cast<float>(stepKnobs[5]->getValue());
    data.pan             = static_cast<float>(stepKnobs[6]->getValue());
    data.modulation      = readModulationDataFromControls();

    onSlotDataChanged(selectedLane, selectedSlot, data);
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
