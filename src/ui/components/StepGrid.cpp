#include "ui/components/StepGrid.h"
#include "ui/components/PresetIcons.h"

namespace zikada {

namespace StepGridMetrics {
    constexpr int kLaneLabelWidth = 110;
    constexpr int kLaneLabelGap = 8;
    constexpr int kLaneControlStripWidth = 56;
    constexpr int kRulerHeight = 14;
    constexpr int kBeatGroupSize = 4;
}

namespace {
    void drawBeatGroupBackgrounds(juce::Graphics& g, int cellAreaX, int cellAreaW,
                                  int laneY, int laneHeight, int stepWidth)
    {
        using namespace StepGridMetrics;

        for (int beat = 0; beat < StepGrid::numSteps / kBeatGroupSize; ++beat)
        {
            const int groupX = cellAreaX + beat * kBeatGroupSize * stepWidth;
            const int groupW = juce::jmin(kBeatGroupSize * stepWidth, cellAreaX + cellAreaW - groupX);
            if (groupW <= 0)
                continue;

            auto beatGroupBounds = juce::Rectangle<int>(groupX, laneY + 2, groupW, laneHeight - 4).toFloat();
            g.setColour((beat % 2 == 0 ? Colours::displayBezel : Colours::bgAccent).withAlpha(0.22f));
            g.fillRoundedRectangle(beatGroupBounds.reduced(1.0f, 0.0f), 3.0f);

            if (beat > 0)
            {
                g.setColour(Colours::neonGreen.withAlpha(0.18f));
                g.drawLine(beatGroupBounds.getX(), beatGroupBounds.getY() + 4.0f,
                           beatGroupBounds.getX(), beatGroupBounds.getBottom() - 4.0f, 1.2f);
            }
        }
    }
}

StepGrid::StepGrid(juce::AudioProcessorValueTreeState& state, SequencerState& seqState)
    : apvts(state), sequencerState(seqState)
{
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setExplicitFocusOrder(200);
    setTitle("Sequencer grid");
    setHelpText("Arrow keys select a step. Space or Enter toggles it. Page Up or Page Down changes its preset. Shift+Left or Shift+Right resizes its tie.");
    updateAccessibilityDescription();
    setupGrid();
}

StepGrid::~StepGrid()
{
    stopTimer();
}

void StepGrid::timerCallback()
{
    if (! hasAnimatedChains())
    {
        updateChainAnimationTimer();
        return;
    }

    chainAnimPhase = (chainAnimPhase + 1) % 16;

    for (int lane = 0; lane < numLanes; ++lane)
    {
        for (int step = 0; step < numSteps; ++step)
        {
            if (!isStepConsumedByChain(lane, step))
                continue;

            cells[lane][step]->setChainAnimPhase(chainAnimPhase);
            cells[lane][step]->repaint();
        }
    }
}

int StepGrid::findChainRoot(int lane, int step) const
{
    for (int lookback = step; lookback >= 0; --lookback)
    {
        const auto& s = sequencerState.getStepData(lane, lookback);
        if (s.active && s.presetIndex > 0 && step < lookback + s.chainLength)
            return lookback;
    }
    return step;
}

bool StepGrid::hasAnimatedChains() const
{
    for (int lane = 0; lane < numLanes; ++lane)
    {
        for (int step = 0; step < numSteps; ++step)
        {
            const auto& s = sequencerState.getStepData(lane, step);
            if (s.active && s.presetIndex > 0 && s.chainLength > 1)
                return true;
        }
    }

    return false;
}

void StepGrid::updateChainAnimationTimer()
{
    if (hasAnimatedChains())
    {
        if (! isTimerRunning())
            startTimerHz(12);
    }
    else if (isTimerRunning())
    {
        stopTimer();
    }
}

void StepGrid::setupGrid()
{
    for (int lane = 0; lane < numLanes; ++lane)
    {
        for (int step = 0; step < numSteps; ++step)
        {
            auto cell = std::make_unique<StepCell>(lane, step);
            addAndMakeVisible(cell.get());

            auto paramID = getStepActiveID(lane, step);
            auto* param = apvts.getParameter(paramID);
            if (param != nullptr)
            {
                attachments[lane][step] = std::make_unique<juce::ButtonParameterAttachment>(
                    *param, *cell, nullptr);
            }

            cell->onSelected = [this, lane, step]()
            {
                setSelectedStep(lane, step);
                if (onStepSelected)
                    onStepSelected(lane, step);
            };

            cell->setInterceptsMouseClicks(false, false);
            cell->setWantsKeyboardFocus(false);
            cell->setAccessible(false);

            cells[lane][step] = std::move(cell);
        }

        auto knob = std::make_unique<Knob>();
        knob->setRange(0.0, 100.0);
        knob->setDefaultValue(100.0);
        knob->setColour(laneInfos[lane].colour);
        knob->setLabel("MIX");
        knob->setTitle(juce::String(laneInfos[lane].name) + " lane mix");
        knob->setExplicitFocusOrder(201 + lane * 3);

        auto mixParamID = getLaneMixID(lane);
        auto* mixParam = apvts.getParameter(mixParamID);

        addAndMakeVisible(knob.get());
        mixKnobs[lane] = std::move(knob);
        if (mixParam != nullptr)
            mixAttachments[lane] = std::make_unique<KnobParameterAttachment>(*mixParam, *mixKnobs[lane]);

        auto muteBtn = std::make_unique<KeyboardTextButton>("M");
        muteBtn->setTitle(juce::String(laneInfos[lane].name) + " lane mute");
        muteBtn->setTooltip("Mute " + juce::String(laneInfos[lane].name) + " lane");
        muteBtn->setExplicitFocusOrder(202 + lane * 3);
        muteBtn->setClickingTogglesState(true);
        muteBtn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        muteBtn->setColour(juce::TextButton::buttonOnColourId, Colours::warning);
        muteBtn->setColour(juce::TextButton::textColourOffId, Colours::white50);
        muteBtn->setColour(juce::TextButton::textColourOnId, Colours::bgPrimary);
        auto* muteParam = apvts.getParameter(getLaneMuteID(lane));
        if (muteParam != nullptr)
            muteAttachments[lane] = std::make_unique<juce::ButtonParameterAttachment>(*muteParam, *muteBtn, nullptr);
        addAndMakeVisible(muteBtn.get());
        muteButtons[lane] = std::move(muteBtn);

        auto soloBtn = std::make_unique<KeyboardTextButton>("S");
        soloBtn->setTitle(juce::String(laneInfos[lane].name) + " lane solo");
        soloBtn->setTooltip("Solo " + juce::String(laneInfos[lane].name) + " lane");
        soloBtn->setExplicitFocusOrder(203 + lane * 3);
        soloBtn->setClickingTogglesState(true);
        soloBtn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        soloBtn->setColour(juce::TextButton::buttonOnColourId, Colours::neonGreen);
        soloBtn->setColour(juce::TextButton::textColourOffId, Colours::white50);
        soloBtn->setColour(juce::TextButton::textColourOnId, Colours::bgPrimary);
        auto* soloParam = apvts.getParameter(getLaneSoloID(lane));
        if (soloParam != nullptr)
            soloAttachments[lane] = std::make_unique<juce::ButtonParameterAttachment>(*soloParam, *soloBtn, nullptr);
        addAndMakeVisible(soloBtn.get());
        soloButtons[lane] = std::move(soloBtn);
    }
}

void StepGrid::paint(juce::Graphics& g)
{
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    if (staticGridLayer.isNull()
        || staticGridLayerDirty
        || staticGridLayer.getWidth() != getWidth()
        || staticGridLayer.getHeight() != getHeight())
    {
        staticGridLayer = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics layerGraphics(staticGridLayer);
        renderStaticGridLayer(layerGraphics);
        staticGridLayerDirty = false;
    }

    g.drawImageAt(staticGridLayer, 0, 0);
}

void StepGrid::renderStaticGridLayer(juce::Graphics& g)
{
    using namespace StepGridMetrics;

    g.fillAll(Colours::bgPrimary);

    const auto  bounds       = getLocalBounds();
    const int   labelWidth   = kLaneLabelWidth;
    const int   labelGap     = kLaneLabelGap;
    const int   knobStripW   = kLaneControlStripWidth;
    const int   rulerHeight  = kRulerHeight;
    const int   totalW       = bounds.getWidth();
    const int   cellAreaW    = totalW - labelWidth - labelGap - knobStripW;
    const int   stepWidth    = cellAreaW / numSteps;
    const int   cellAreaX    = bounds.getX() + labelWidth + labelGap;

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    {
        const auto rulerRect = juce::Rectangle<int>(cellAreaX, bounds.getY(), cellAreaW, rulerHeight);

        g.setColour(Colours::bgPrimary.withAlpha(0.70f));
        g.fillRect(rulerRect);

        for (int beat = 0; beat < 4; ++beat)
        {
            const int bx = cellAreaX + beat * kBeatGroupSize * stepWidth;

            if (beat > 0)
            {
                g.setColour(Colours::neonGreen.withAlpha(0.30f));
                g.drawLine(static_cast<float>(bx),
                           static_cast<float>(bounds.getY() + 2),
                           static_cast<float>(bx),
                           static_cast<float>(bounds.getY() + rulerHeight - 2), 1.0f);
            }

            g.setColour(beat == 0 ? Colours::neonGreen.withAlpha(0.90f)
                                  : Colours::white.withAlpha(0.55f));
            g.setFont(laf != nullptr ? laf->getSpaceMonoFont(13.0f, true)
                                     : juce::Font(juce::FontOptions().withHeight(13.0f)));
            g.drawText(juce::String(beat * 4 + 1),
                       juce::Rectangle<int>(bx + 3, bounds.getY() + 1, 20, rulerHeight - 2),
                       juce::Justification::centredLeft, false);

            for (int s = 0; s < 4; ++s)
            {
                const int tickX = bx + s * stepWidth + stepWidth / 2;
                g.setColour(Colours::white.withAlpha(0.08f));
                g.drawLine(static_cast<float>(tickX),
                           static_cast<float>(bounds.getY() + rulerHeight - 3),
                           static_cast<float>(tickX),
                           static_cast<float>(bounds.getY() + rulerHeight - 1), 0.8f);
            }
        }

        g.setColour(Colours::neonGreen.withAlpha(0.15f));
        g.drawLine(static_cast<float>(cellAreaX),
                   static_cast<float>(bounds.getY() + rulerHeight - 0.5f),
                   static_cast<float>(cellAreaX + cellAreaW),
                   static_cast<float>(bounds.getY() + rulerHeight - 0.5f), 1.0f);
    }

    const int laneAreaH  = bounds.getHeight() - rulerHeight;
    const int laneHeight = laneAreaH / numLanes;
    const int laneAreaY  = bounds.getY() + rulerHeight;

    for (int lane = 0; lane < numLanes; ++lane)
    {
        const int ly       = laneAreaY + lane * laneHeight;
        const auto laneCol = laneInfos[lane].colour;

        if (lane % 2 == 0)
        {
            g.setColour(Colours::bgAccent.withAlpha(0.25f));
            g.fillRect(bounds.getX(), ly, totalW, laneHeight);
        }

        drawBeatGroupBackgrounds(g, cellAreaX, cellAreaW, ly, laneHeight, stepWidth);

        const auto chipBounds = juce::Rectangle<int>(
            bounds.getX() + 2, ly + 3, labelWidth - 4, laneHeight - 6).toFloat();

        g.setColour(Colours::displayBezel);
        g.fillRoundedRectangle(chipBounds, 4.0f);

        g.setColour(laneCol.withAlpha(0.90f));
        g.fillRoundedRectangle(chipBounds.withWidth(3.5f)
                                          .withTrimmedTop(5.0f)
                                          .withTrimmedBottom(5.0f), 1.5f);

        g.setColour(laneCol.withAlpha(0.22f));
        g.drawRoundedRectangle(chipBounds, 4.0f, 1.0f);

        float iconSize = chipBounds.getHeight() * 0.35f;
        auto iconBounds = juce::Rectangle<float>(
            chipBounds.getX() + 8.0f,
            chipBounds.getCentreY() - iconSize * 0.5f,
            iconSize, iconSize);
        PresetIcons::drawLaneIcon(g, lane, iconBounds, laneCol, 1.5f);

        const auto nameZone = chipBounds.withTrimmedLeft(8.0f + iconSize + 4.0f);
        g.setColour(laneCol);
        g.setFont(laf != nullptr ? laf->getVcrFont(15.0f)
                                 : juce::Font(juce::FontOptions().withHeight(15.0f)));
        g.drawText(juce::String(laneInfos[lane].name),
                   nameZone.withHeight(nameZone.getHeight() * 0.54f),
                   juce::Justification::centredLeft, false);

        g.setColour(Colours::white.withAlpha(0.48f));
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(13.0f)
                                 : juce::Font(juce::FontOptions().withHeight(13.0f)));
        g.drawText("ROW " + juce::String(lane + 1),
                   nameZone.withY(nameZone.getY() + nameZone.getHeight() * 0.54f)
                            .withHeight(nameZone.getHeight() * 0.46f),
                   juce::Justification::centredLeft, false);

        for (int grp = 1; grp < 4; ++grp)
        {
            const float divX = static_cast<float>(cellAreaX + grp * kBeatGroupSize * stepWidth);
            g.setColour(Colours::neonGreen.withAlpha(0.16f));
            g.drawLine(divX, static_cast<float>(ly + 4),
                       divX, static_cast<float>(ly + laneHeight - 4), 1.0f);
        }

        {
            const int   stripX    = bounds.getX() + totalW - knobStripW;
            const float stripXf   = static_cast<float>(stripX);
            const float stripYf   = static_cast<float>(ly + 1);
            const float stripWf   = static_cast<float>(knobStripW);
            const float stripHf   = static_cast<float>(laneHeight - 2);

            g.setColour(Colours::displayBezel);
            g.fillRect(stripXf, stripYf, stripWf, stripHf);

            g.setColour(laneCol.withAlpha(0.75f));
            g.fillRect(stripXf, stripYf + 4.0f, 2.5f, stripHf - 8.0f);

            g.setColour(laneCol.withAlpha(0.20f));
            g.drawLine(stripXf + 3.0f, stripYf + 0.5f,
                       stripXf + stripWf, stripYf + 0.5f, 1.0f);

            g.setColour(Colours::white.withAlpha(0.06f));
            g.drawLine(stripXf + 0.5f, stripYf,
                       stripXf + 0.5f, stripYf + stripHf, 1.0f);
        }

        if (lane < numLanes - 1)
        {
            const float sepY = static_cast<float>(ly + laneHeight);
            g.setColour(Colours::white.withAlpha(0.07f));
            g.drawLine(static_cast<float>(bounds.getX()),     sepY,
                       static_cast<float>(bounds.getRight()), sepY, 1.0f);
        }
    }
}

void StepGrid::paintOverChildren(juce::Graphics& g)
{
    if (chainDrawMode && chainDrawCurrentStep > chainDrawStartStep)
    {
        auto* startCell = cells[chainDrawLane][chainDrawStartStep].get();
        auto* endCell   = cells[chainDrawLane][chainDrawCurrentStep].get();
        if (startCell != nullptr && endCell != nullptr)
        {
            auto startBounds = startCell->getBounds().toFloat();
            auto endBounds   = endCell->getBounds().toFloat();
            float y  = startBounds.getCentreY();
            float x1 = startBounds.getRight() - 4.0f;
            float x2 = endBounds.getX() + 4.0f;

            g.setColour(laneInfos[chainDrawLane].colour.withAlpha(0.55f));
            g.drawLine(x1, y, x2, y, 3.0f);

            int steps = chainDrawCurrentStep - chainDrawStartStep;
            for (int i = 1; i < steps; ++i)
            {
                float x = juce::jmap(static_cast<float>(i), 0.0f, static_cast<float>(steps), x1, x2);
                g.fillEllipse(x - 2.5f, y - 2.5f, 5.0f, 5.0f);
            }
        }
    }

    if (chainEraseMode && chainEraseCurrentStep < chainEraseStartStep)
    {
        auto* rootCell = cells[chainEraseLane][chainEraseRoot].get();
        auto* endCell  = cells[chainEraseLane][chainEraseCurrentStep].get();
        if (rootCell != nullptr && endCell != nullptr)
        {
            auto rootBounds = rootCell->getBounds().toFloat();
            auto endBounds  = endCell->getBounds().toFloat();
            float y  = rootBounds.getCentreY();
            float x1 = rootBounds.getRight() - 4.0f;
            float x2 = endBounds.getX() + 4.0f;

            g.setColour(Colours::warning.withAlpha(0.55f));
            g.drawLine(x1, y, x2, y, 3.0f);
        }
    }

    drawKeyboardFocusRing(g);
    drawPlayheadRail(g);
}

void StepGrid::drawPlayheadRail(juce::Graphics& g)
{
    if (lastPlayingStep < 0 || lastPlayingStep >= numSteps)
        return;

    using namespace StepGridMetrics;

    const auto bounds = getLocalBounds();
    const int labelWidth = kLaneLabelWidth;
    const int labelGap = kLaneLabelGap;
    const int knobStripW = kLaneControlStripWidth;
    const int rulerHeight = kRulerHeight;
    const int cellAreaW = bounds.getWidth() - labelWidth - labelGap - knobStripW;
    const int stepWidth = cellAreaW / numSteps;
    const int cellAreaX = bounds.getX() + labelWidth + labelGap;

    const float x = static_cast<float>(cellAreaX + lastPlayingStep * stepWidth + stepWidth / 2);
    const float y1 = static_cast<float>(bounds.getY() + rulerHeight);
    const float y2 = static_cast<float>(bounds.getBottom());

    g.setColour(Colours::neonGreen.withAlpha(0.22f));
    g.drawLine(x, y1, x, y2, 3.0f);

    g.setColour(Colours::neonGreen.withAlpha(0.55f));
    g.drawLine(x, y1, x, y2, 1.0f);

    g.setColour(Colours::neonGreen.withAlpha(0.12f));
    g.fillRect(x - static_cast<float>(stepWidth) * 0.5f, y1, static_cast<float>(stepWidth), y2 - y1);
}

void StepGrid::resized()
{
    using namespace StepGridMetrics;

    auto bounds = getLocalBounds();

    const int labelWidth     = kLaneLabelWidth;
    const int labelGap       = kLaneLabelGap;
    const int knobStripWidth = kLaneControlStripWidth;
    const int rulerHeight    = kRulerHeight;

    bounds.removeFromTop(rulerHeight);

    const int laneHeight = bounds.getHeight() / numLanes;

    for (int lane = 0; lane < numLanes; ++lane)
    {
        auto laneBounds = bounds.removeFromTop(laneHeight);
        laneBounds.removeFromLeft(labelWidth + labelGap);
        auto knobBounds = laneBounds.removeFromRight(knobStripWidth);
        const int stepWidth = laneBounds.getWidth() / numSteps;

        for (int step = 0; step < numSteps; ++step)
        {
            cells[lane][step]->setBounds(laneBounds.removeFromLeft(stepWidth).reduced(2));
        }

        auto knobArea = knobBounds.reduced(2, 3);
        auto btnRow = knobArea.removeFromTop(16);
        muteButtons[lane]->setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(1, 0));
        soloButtons[lane]->setBounds(btnRow.reduced(1, 0));
        knobArea.removeFromTop(2);
        mixKnobs[lane]->setBounds(knobArea);
    }

    invalidateStaticGridLayer();
}

void StepGrid::setPlayingStep(int step)
{
    const int previousPlayingStep = lastPlayingStep;

    if (lastPlayingStep >= 0 && lastPlayingStep < numSteps)
    {
        for (int lane = 0; lane < numLanes; ++lane)
            cells[lane][lastPlayingStep]->setPlaying(false);
    }

    if (step >= 0 && step < numSteps)
    {
        for (int lane = 0; lane < numLanes; ++lane)
            cells[lane][step]->setPlaying(true);
    }

    lastPlayingStep = step;

    if (previousPlayingStep >= 0 && previousPlayingStep < numSteps)
        repaint(getStepColumnBounds(previousPlayingStep).expanded(4, 0));

    if (lastPlayingStep >= 0 && lastPlayingStep < numSteps && lastPlayingStep != previousPlayingStep)
        repaint(getStepColumnBounds(lastPlayingStep).expanded(4, 0));
}

void StepGrid::lookAndFeelChanged()
{
    invalidateStaticGridLayer();
}

void StepGrid::invalidateStaticGridLayer()
{
    staticGridLayerDirty = true;
    repaint();
}

juce::Rectangle<int> StepGrid::getStepColumnBounds(int step) const
{
    if (step < 0 || step >= numSteps)
        return {};

    using namespace StepGridMetrics;

    const auto bounds = getLocalBounds();
    const int cellAreaW = bounds.getWidth() - kLaneLabelWidth - kLaneLabelGap - kLaneControlStripWidth;
    const int stepWidth = cellAreaW / numSteps;

    if (stepWidth <= 0)
        return {};

    const int cellAreaX = bounds.getX() + kLaneLabelWidth + kLaneLabelGap;
    const int x = cellAreaX + step * stepWidth;
    const int y = bounds.getY() + kRulerHeight;
    return { x, y, stepWidth, juce::jmax(0, bounds.getHeight() - kRulerHeight) };
}

void StepGrid::setSelectedStep(int lane, int step)
{
    if (selectedLane >= 0 && selectedStep >= 0
        && selectedLane < numLanes && selectedStep < numSteps)
        cells[selectedLane][selectedStep]->setSelected(false);

    selectedLane = lane;
    selectedStep = step;

    if (selectedLane >= 0 && selectedStep >= 0
        && selectedLane < numLanes && selectedStep < numSteps)
        cells[selectedLane][selectedStep]->setSelected(true);

    updateAccessibilityDescription();
    repaint();
}

void StepGrid::updateAccessibilityDescription()
{
    if (selectedLane < 0 || selectedLane >= numLanes || selectedStep < 0 || selectedStep >= numSteps)
    {
        setTitle("Sequencer grid");
        setDescription("Six effect lanes with sixteen steps each; no step is selected.");
        return;
    }

    const auto& data = sequencerState.getStepData(selectedLane, selectedStep);
    auto description = juce::String(laneInfos[selectedLane].name)
                     + " lane, step " + juce::String(selectedStep + 1);
    description += data.active ? ", active" : ", inactive";
    if (data.active && data.presetIndex > 0)
        description += ", preset " + juce::String(data.presetIndex);
    setTitle("Sequencer grid: " + description);
    setDescription(description + ".");

    if (auto* handler = getAccessibilityHandler())
        handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
}

std::unique_ptr<juce::AccessibilityHandler> StepGrid::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::group);
}

std::pair<int, int> StepGrid::hitTestCell(juce::Point<int> pos) const
{
    using namespace StepGridMetrics;

    const int labelWidth     = kLaneLabelWidth;
    const int labelGap       = kLaneLabelGap;
    const int knobStripWidth = kLaneControlStripWidth;
    const int rulerHeight    = kRulerHeight;

    const auto bounds  = getLocalBounds();
    const int cellAreaX = bounds.getX() + labelWidth + labelGap;
    const int cellAreaW = bounds.getWidth() - labelWidth - labelGap - knobStripWidth;

    if (pos.y < rulerHeight || pos.x < cellAreaX || pos.x >= cellAreaX + cellAreaW)
        return { -1, -1 };

    const int laneAreaH = bounds.getHeight() - rulerHeight;
    const int laneHeight = laneAreaH / numLanes;
    const int stepWidth  = cellAreaW  / numSteps;

    if (laneHeight == 0 || stepWidth == 0)
        return { -1, -1 };

    const int step = (pos.x - cellAreaX) / stepWidth;
    const int lane = (pos.y - rulerHeight) / laneHeight;

    if (step < 0 || step >= numSteps || lane < 0 || lane >= numLanes)
        return { -1, -1 };

    return { lane, step };
}

void StepGrid::applyPaintSourceToCell(int lane, int step)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return;

    removeChainAt(lane, step);

    auto paintedData = paintSourceData;
    paintedData.chainLength = 1;
    if (!paintedData.active)
        paintedData.presetIndex = 0;

    sequencerState.setStepData(lane, step, paintedData);
    setStepActiveAsCompleteGesture(lane, step, paintedData.active);

    if (auto* cell = getCell(lane, step))
    {
        cell->setToggleState(paintedData.active, juce::dontSendNotification);
        cell->setActive(paintedData.active);
        cell->setPresetIndex(paintedData.active ? paintedData.presetIndex : 0);
    }

    if (onStepPresetChanged)
        onStepPresetChanged(lane, step, paintedData.presetIndex);
}

void StepGrid::resetPaintGesture()
{
    isPainting = false;
    paintEditStarted = false;
    lastPaintedLane = -1;
    lastPaintedStep = -1;
}

void StepGrid::cyclePresetAt(int lane, int step, int direction)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps || direction == 0)
        return;

    if (isStepConsumedByChain(lane, step))
        step = findChainRoot(lane, step);

    auto data = sequencerState.getStepData(lane, step);
    const int currentPreset = data.active && data.presetIndex > 0 ? data.presetIndex : 0;
    int nextPreset = currentPreset + direction;

    if (nextPreset < 1)
        nextPreset = 20;
    else if (nextPreset > 20)
        nextPreset = 1;

    data.active = true;
    data.presetIndex = nextPreset;
    data.chainLength = juce::jlimit(1, numSteps - step, data.chainLength);

    if (onStepPresetEditStarting)
        onStepPresetEditStarting();

    sequencerState.setStepData(lane, step, data);
    setStepActive(lane, step, true);

    if (auto* cell = getCell(lane, step))
    {
        cell->setActive(true);
        cell->setPresetIndex(nextPreset);
    }

    setSelectedStep(lane, step);
    refreshChainVisuals(lane);
    refreshLane(lane);

    if (onStepPresetChanged)
        onStepPresetChanged(lane, step, nextPreset);
}

void StepGrid::selectStepAndNotify(int lane, int step)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return;

    setSelectedStep(lane, step);

    if (onStepSelected)
        onStepSelected(lane, step);
}

bool StepGrid::moveSelectionBy(int laneDelta, int stepDelta)
{
    const int baseLane = selectedLane >= 0 ? selectedLane : 0;
    const int baseStep = selectedStep >= 0 ? selectedStep : 0;
    const int nextLane = juce::jlimit(0, numLanes - 1, baseLane + laneDelta);
    const int nextStep = juce::jlimit(0, numSteps - 1, baseStep + stepDelta);

    selectStepAndNotify(nextLane, nextStep);
    return true;
}

bool StepGrid::toggleSelectedStep()
{
    if (selectedLane < 0 || selectedLane >= numLanes || selectedStep < 0 || selectedStep >= numSteps)
    {
        selectStepAndNotify(0, 0);
        return true;
    }

    int lane = selectedLane;
    int step = selectedStep;

    if (isStepConsumedByChain(lane, step))
        step = findChainRoot(lane, step);

    auto data = sequencerState.getStepData(lane, step);

    if (onStepPresetEditStarting)
        onStepPresetEditStarting();

    if (data.active)
    {
        data.active = false;
        data.presetIndex = 0;
        data.chainLength = 1;
    }
    else
    {
        data.active = true;
        if (data.presetIndex <= 0)
            data.presetIndex = 1;
        data.chainLength = juce::jlimit(1, numSteps - step, data.chainLength);
    }

    sequencerState.setStepData(lane, step, data);
    setStepActive(lane, step, data.active);

    if (auto* cell = getCell(lane, step))
    {
        cell->setToggleState(data.active, juce::dontSendNotification);
        cell->setActive(data.active);
        cell->setPresetIndex(data.active ? data.presetIndex : 0);
    }

    setSelectedStep(lane, step);
    refreshChainVisuals(lane);
    refreshLane(lane);

    if (onStepPresetChanged)
        onStepPresetChanged(lane, step, data.presetIndex);

    return true;
}

bool StepGrid::cycleSelectedPreset(int direction)
{
    if (selectedLane < 0 || selectedLane >= numLanes || selectedStep < 0 || selectedStep >= numSteps)
        selectStepAndNotify(0, 0);

    cyclePresetAt(selectedLane, selectedStep, direction);
    return true;
}

bool StepGrid::resizeSelectedChain(int delta)
{
    if (selectedLane < 0 || selectedLane >= numLanes || selectedStep < 0 || selectedStep >= numSteps)
    {
        selectStepAndNotify(0, 0);
        return true;
    }

    const int lane = selectedLane;
    const int root = findChainRoot(lane, selectedStep);
    auto data = sequencerState.getStepData(lane, root);
    if (!data.active || data.presetIndex <= 0)
        return true;

    const int maximumLength = numSteps - root;
    const int nextLength = juce::jlimit(1, maximumLength, data.chainLength + delta);
    if (nextLength == data.chainLength)
        return true;

    if (nextLength > data.chainLength)
    {
        const auto& nextData = sequencerState.getStepData(lane, root + data.chainLength);
        if (nextData.active && nextData.presetIndex > 0)
            return true;
    }

    if (onStepPresetEditStarting)
        onStepPresetEditStarting();

    data.chainLength = nextLength;
    sequencerState.setStepData(lane, root, data);
    selectStepAndNotify(lane, root);
    refreshChainVisuals(lane);
    refreshLane(lane);

    if (onChainChanged)
        onChainChanged(lane, root, nextLength);

    return true;
}

bool StepGrid::handleKeyCommand(const juce::KeyPress& key)
{
    const int code = key.getKeyCode();
    const bool shiftDown = key.getModifiers().isShiftDown();

    if (shiftDown && code == juce::KeyPress::leftKey)  return resizeSelectedChain(-1);
    if (shiftDown && code == juce::KeyPress::rightKey) return resizeSelectedChain(1);
    if (code == juce::KeyPress::pageUpKey)             return cycleSelectedPreset(1);
    if (code == juce::KeyPress::pageDownKey)           return cycleSelectedPreset(-1);
    if (code == juce::KeyPress::leftKey)               return moveSelectionBy(0, -1);
    if (code == juce::KeyPress::rightKey)              return moveSelectionBy(0, 1);
    if (code == juce::KeyPress::upKey)                 return moveSelectionBy(-1, 0);
    if (code == juce::KeyPress::downKey)               return moveSelectionBy(1, 0);
    if (code == juce::KeyPress::homeKey)               return moveSelectionBy(0, -(selectedStep >= 0 ? selectedStep : 0));
    if (code == juce::KeyPress::endKey)                return moveSelectionBy(0, numSteps - 1 - (selectedStep >= 0 ? selectedStep : 0));
    if (code == juce::KeyPress::spaceKey || code == juce::KeyPress::returnKey)
        return toggleSelectedStep();
    return false;
}

void StepGrid::drawKeyboardFocusRing(juce::Graphics& g)
{
    if (! hasKeyboardFocus(false)
        || selectedLane < 0 || selectedLane >= numLanes
        || selectedStep < 0 || selectedStep >= numSteps)
        return;

    if (auto* cell = getCell(selectedLane, selectedStep))
    {
        const auto bounds = cell->getBounds().toFloat().expanded(3.0f);
        const auto laneColour = laneInfos[selectedLane].colour;

        g.setColour(laneColour.withAlpha(0.18f));
        g.fillRoundedRectangle(bounds.expanded(2.0f), 6.0f);

        g.setColour(Colours::white.withAlpha(0.70f));
        g.drawRoundedRectangle(bounds, 5.0f, 1.5f);

        g.setColour(Colours::neonGreen.withAlpha(0.95f));
        g.drawRoundedRectangle(bounds.expanded(2.0f), 7.0f, 1.2f);
    }
}

void StepGrid::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    resetPaintGesture();

    for (int l = 0; l < numLanes; ++l)
    {
        for (int s = 0; s < numSteps; ++s)
        {
            if (!cells[l][s]->isChainable())
                continue;

            const auto cellBounds = cells[l][s]->getBounds();
            const auto plusBounds = cells[l][s]->getChainBadgeBounds()
                                        .translated(static_cast<float>(cellBounds.getX()),
                                                    static_cast<float>(cellBounds.getY()))
                                        .expanded(2.0f);
            if (plusBounds.contains(e.position))
            {
                auto data = sequencerState.getStepData(l, s);
                if (onStepPresetEditStarting)
                    onStepPresetEditStarting();
                data.chainLength++;
                sequencerState.setStepData(l, s, data);
                refreshChainVisuals(l);
                if (onChainChanged)
                    onChainChanged(l, s, data.chainLength);
                return;
            }
        }
    }

    auto [lane, step] = hitTestCell(e.getPosition());
    if (lane < 0)
        return;

    if (e.mods.isRightButtonDown())
    {
        bool isChained = isStepConsumedByChain(lane, step) || getStepChainLength(lane, step) > 1;
        if (isChained)
        {
            chainEraseMode = true;
            chainEraseLane = lane;
            chainEraseRoot = (getStepChainLength(lane, step) > 1) ? step : findChainRoot(lane, step);
            chainEraseStartStep = step;
            chainEraseCurrentStep = step;
            chainEraseOriginalLength = getStepChainLength(chainEraseLane, chainEraseRoot);
        }
        else
        {
            auto data = sequencerState.getStepData(lane, step);
            if (data.active)
            {
                if (onStepPresetEditStarting)
                    onStepPresetEditStarting();
                data.active = false;
                data.presetIndex = 0;
                data.chainLength = 1;
                sequencerState.setStepData(lane, step, data);
                setStepActive(lane, step, false);
                if (auto* cell = getCell(lane, step))
                {
                    cell->setToggleState(false, juce::dontSendNotification);
                    cell->setActive(false);
                    cell->setPresetIndex(0);
                }
                refreshChainVisuals(lane);
                refreshLane(lane);
                if (onStepPresetChanged)
                    onStepPresetChanged(lane, step, 0);
            }
        }
        return;
    }

    if (e.mods.isShiftDown())
    {
        const int root = findChainRoot(lane, step);
        const auto& rootData = sequencerState.getStepData(lane, root);
        if (rootData.active && rootData.presetIndex > 0)
        {
            chainDrawMode = true;
            chainDrawDragged = false;
            chainDrawLane = lane;
            chainDrawStartStep = root;
            chainDrawCurrentStep = root;
        }
    }
    else
    {
        const int sourceStep = findChainRoot(lane, step);
        paintSourceData = sequencerState.getStepData(lane, sourceStep);
        if (const auto* activeGate = apvts.getRawParameterValue(getStepActiveID(lane, sourceStep)))
            paintSourceData.active = activeGate->load() >= 0.5f;
        paintSourceData.chainLength = 1;
        if (!paintSourceData.active)
            paintSourceData.presetIndex = 0;
        else if (paintSourceData.presetIndex <= 0)
            paintSourceData.presetIndex = 1;

        isPainting = true;
        paintEditStarted = false;
        lastPaintedLane = lane;
        lastPaintedStep = step;
    }

    selectStepAndNotify(lane, step);
}

void StepGrid::mouseDrag(const juce::MouseEvent& e)
{
    if (chainDrawMode)
    {
        auto [lane, step] = hitTestCell(e.getPosition());
        if (lane == chainDrawLane && step >= chainDrawStartStep && step != chainDrawCurrentStep)
        {
            bool valid = true;
            for (int s = chainDrawStartStep + 1; s <= step; ++s)
            {
                const auto& d = sequencerState.getStepData(chainDrawLane, s);
                if (d.active && d.presetIndex > 0) { valid = false; break; }
            }
            if (valid)
            {
                chainDrawCurrentStep = step;
                chainDrawDragged = true;
                repaint();
            }
        }
        return;
    }

    if (chainEraseMode)
    {
        auto [lane, step] = hitTestCell(e.getPosition());
        if (lane == chainEraseLane && step >= chainEraseRoot && step <= chainEraseStartStep && step != chainEraseCurrentStep)
        {
            chainEraseCurrentStep = step;
            repaint();
        }
        return;
    }

    if (isPainting)
    {
        auto [lane, step] = hitTestCell(e.getPosition());
        if (lane != lastPaintedLane || step < 0 || step == lastPaintedStep)
            return;

        if (!paintEditStarted)
        {
            if (onStepPresetEditStarting)
                onStepPresetEditStarting();
            paintEditStarted = true;
        }

        const int direction = step > lastPaintedStep ? 1 : -1;
        for (int paintedStep = lastPaintedStep + direction;
             paintedStep != step + direction;
             paintedStep += direction)
        {
            applyPaintSourceToCell(lane, paintedStep);
        }

        lastPaintedStep = step;
        setSelectedStep(lane, step);
        refreshChainVisuals(lane);
        refreshLane(lane);
        return;
    }
}

void StepGrid::mouseUp(const juce::MouseEvent&)
{
    if (chainDrawMode)
    {
        if (chainDrawDragged && chainDrawCurrentStep > chainDrawStartStep)
        {
            int length = chainDrawCurrentStep - chainDrawStartStep + 1;
            auto data = sequencerState.getStepData(chainDrawLane, chainDrawStartStep);
            if (data.chainLength != length)
            {
                if (onStepPresetEditStarting)
                    onStepPresetEditStarting();
                data.chainLength = length;
                sequencerState.setStepData(chainDrawLane, chainDrawStartStep, data);
                refreshChainVisuals(chainDrawLane);
                if (onChainChanged)
                    onChainChanged(chainDrawLane, chainDrawStartStep, length);
            }
        }
        chainDrawMode = false;
        chainDrawDragged = false;
        chainDrawLane = -1;
        chainDrawStartStep = -1;
        chainDrawCurrentStep = -1;
        repaint();
        return;
    }

    if (chainEraseMode)
    {
        int newLength = juce::jmax(1, chainEraseCurrentStep - chainEraseRoot + 1);
        if (newLength != chainEraseOriginalLength)
        {
            if (onStepPresetEditStarting)
                onStepPresetEditStarting();
            auto data = sequencerState.getStepData(chainEraseLane, chainEraseRoot);
            data.chainLength = newLength;
            sequencerState.setStepData(chainEraseLane, chainEraseRoot, data);
            refreshChainVisuals(chainEraseLane);
            if (onChainChanged)
                onChainChanged(chainEraseLane, chainEraseRoot, newLength);
        }
        chainEraseMode = false;
        chainEraseLane = -1;
        chainEraseRoot = -1;
        chainEraseStartStep = -1;
        chainEraseCurrentStep = -1;
        chainEraseOriginalLength = 1;
        repaint();
        return;
    }

    if (isPainting)
    {
        resetPaintGesture();
        repaint();
    }
}

void StepGrid::mouseMove(const juce::MouseEvent& e)
{
    auto [lane, step] = hitTestCell(e.getPosition());
    if (lane != hoverLane || step != hoverStep)
    {
        if (hoverLane >= 0 && hoverStep >= 0)
            cells[hoverLane][hoverStep]->setHovered(false);

        hoverLane = lane;
        hoverStep = step;

        if (hoverLane >= 0 && hoverStep >= 0)
            cells[hoverLane][hoverStep]->setHovered(true);
    }
}

void StepGrid::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY == 0.0f)
        return;

    auto [lane, step] = hitTestCell(e.getPosition());
    if (lane < 0 || step < 0)
    {
        if (! e.mods.isShiftDown())
            return;

        lane = selectedLane;
        step = selectedStep;
    }

    const int direction = wheel.deltaY >= 0.0f ? 1 : -1;
    cyclePresetAt(lane, step, direction);
}

bool StepGrid::keyPressed(const juce::KeyPress& key)
{
    if (!hasKeyboardFocus(false))
        return false;

    return handleKeyCommand(key);
}

void StepGrid::focusGained(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    repaint();
}

void StepGrid::focusLost(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    repaint();
}

void StepGrid::mouseExit(const juce::MouseEvent& /*e*/)
{
    if (hoverLane >= 0 && hoverStep >= 0)
        cells[hoverLane][hoverStep]->setHovered(false);
    hoverLane = -1;
    hoverStep = -1;
}

void StepGrid::refreshChainVisuals(int lane)
{
    if (lane < 0 || lane >= numLanes)
        return;

    for (int step = 0; step < numSteps; ++step)
    {
        cells[lane][step]->setTied(false);
        cells[lane][step]->setChained(false);
    }

    for (int step = 0; step < numSteps; ++step)
    {
        const auto& s = sequencerState.getStepData(lane, step);
        if (s.active && s.presetIndex > 0 && s.chainLength > 1)
        {
            for (int c = 1; c < s.chainLength && (step + c) < numSteps; ++c)
            {
                cells[lane][step + c]->setTied(true);
                cells[lane][step + c]->setChained(true);
            }
        }
    }

    for (int step = 0; step < numSteps; ++step)
    {
        const auto& s = sequencerState.getStepData(lane, step);
        bool canExtend = s.active && s.presetIndex > 0 && !isStepConsumedByChain(lane, step)
                         && s.chainLength < (numSteps - step);
        if (canExtend)
        {
            int nextStep = step + s.chainLength;
            if (nextStep < numSteps)
            {
                const auto& nextData = sequencerState.getStepData(lane, nextStep);
                if (nextData.active && nextData.presetIndex > 0)
                    canExtend = false;
            }
        }
        cells[lane][step]->setChainable(canExtend);
    }

    updateChainAnimationTimer();
}

void StepGrid::setStepChainLength(int lane, int step, int length)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return;

    auto data = sequencerState.getStepData(lane, step);
    data.chainLength = juce::jlimit(1, numSteps - step, length);
    sequencerState.setStepData(lane, step, data);
    refreshChainVisuals(lane);
    if (onChainChanged)
        onChainChanged(lane, step, data.chainLength);
}

int StepGrid::getStepChainLength(int lane, int step) const
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return 1;
    return sequencerState.getStepData(lane, step).chainLength;
}

bool StepGrid::isStepConsumedByChain(int lane, int step) const
{
    for (int lookback = step - 1; lookback >= 0; --lookback)
    {
        const auto& s = sequencerState.getStepData(lane, lookback);
        if (s.active && s.presetIndex > 0 && step < lookback + s.chainLength)
            return true;
    }
    return false;
}

void StepGrid::removeChainAt(int lane, int step)
{
    auto data = sequencerState.getStepData(lane, step);
    if (data.chainLength > 1)
    {
        data.chainLength = 1;
        sequencerState.setStepData(lane, step, data);
        refreshChainVisuals(lane);
        if (onChainChanged)
            onChainChanged(lane, step, 1);
        return;
    }

    for (int lookback = step - 1; lookback >= 0; --lookback)
    {
        const auto& s = sequencerState.getStepData(lane, lookback);
        if (s.active && s.presetIndex > 0 && step < lookback + s.chainLength)
        {
            auto rootData = s;
            rootData.chainLength = step - lookback;
            sequencerState.setStepData(lane, lookback, rootData);
            refreshChainVisuals(lane);
            if (onChainChanged)
                onChainChanged(lane, lookback, rootData.chainLength);
            return;
        }
    }
}

void StepGrid::refreshLane(int lane)
{
    if (lane < 0 || lane >= numLanes)
        return;

    for (int step = 0; step < numSteps; ++step)
        cells[lane][step]->repaint();
}

void StepGrid::setStepActive(int lane, int step, bool active)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return;

    auto paramID = getStepActiveID(lane, step);
    auto* param = apvts.getParameter(paramID);
    if (param != nullptr)
        param->setValueNotifyingHost(active ? 1.0f : 0.0f);
}

void StepGrid::setStepActiveAsCompleteGesture(int lane, int step, bool active)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return;

    if (auto* param = apvts.getParameter(getStepActiveID(lane, step)))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(active ? 1.0f : 0.0f);
        param->endChangeGesture();
    }
}

StepCell* StepGrid::getCell(int lane, int step)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return nullptr;
    return cells[lane][step].get();
}

}
