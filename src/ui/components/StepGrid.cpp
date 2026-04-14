#include "ui/components/StepGrid.h"

namespace zikada {

StepGrid::StepGrid(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    setupGrid();
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

            cells[lane][step] = std::move(cell);
        }

        auto knob = std::make_unique<Knob>();
        knob->setRange(0.0, 100.0);
        knob->setDefaultValue(100.0);
        knob->setColour(laneInfos[lane].colour);
        knob->setLabel("MIX");

        auto mixParamID = getLaneMixID(lane);
        auto* mixParam = apvts.getParameter(mixParamID);
        if (mixParam != nullptr)
            knob->setValue(mixParam->getValue() * 100.0);

        knob->onValueChange = [this, lane, mixParamID]()
        {
            auto* param = apvts.getParameter(mixParamID);
            if (param != nullptr)
                param->setValueNotifyingHost(static_cast<float>(mixKnobs[lane]->getValue()) / 100.0f);
        };

        addAndMakeVisible(knob.get());
        mixKnobs[lane] = std::move(knob);
    }
}

void StepGrid::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgPrimary);

    const auto  bounds       = getLocalBounds();
    const int   labelWidth   = 72;
    const int   labelGap     = 8;
    const int   knobStripW   = 56;
    const int   rulerHeight  = 14;
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
            const int bx = cellAreaX + beat * 4 * stepWidth;

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
            g.setFont(laf != nullptr ? laf->getSpaceMonoFont(11.0f, true)
                                     : juce::Font(juce::FontOptions().withHeight(11.0f)));
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

        const auto nameZone = chipBounds.withTrimmedLeft(8.0f);
        g.setColour(laneCol);
        g.setFont(laf != nullptr ? laf->getVcrFont(13.0f)
                                 : juce::Font(juce::FontOptions().withHeight(13.0f)));
        g.drawText(juce::String(laneInfos[lane].name),
                   nameZone.withHeight(nameZone.getHeight() * 0.54f),
                   juce::Justification::centredLeft, false);

        g.setColour(Colours::white.withAlpha(0.48f));
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(11.0f)
                                 : juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText("ROW " + juce::String(lane + 1),
                   nameZone.withY(nameZone.getY() + nameZone.getHeight() * 0.54f)
                            .withHeight(nameZone.getHeight() * 0.46f),
                   juce::Justification::centredLeft, false);

        for (int grp = 1; grp < 4; ++grp)
        {
            const float divX = static_cast<float>(cellAreaX + grp * 4 * stepWidth);
            g.setColour(Colours::neonGreen.withAlpha(0.10f));
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

void StepGrid::resized()
{
    auto bounds = getLocalBounds();

    const int labelWidth     = 72;
    const int labelGap       = 8;
    const int knobStripWidth = 56;
    const int rulerHeight    = 14;

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

        mixKnobs[lane]->setBounds(knobBounds.reduced(2, 3));
    }
}

void StepGrid::setPlayingStep(int step)
{
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
}

std::pair<int, int> StepGrid::hitTestCell(juce::Point<int> pos) const
{
    const int labelWidth     = 72;
    const int labelGap       = 8;
    const int knobStripWidth = 56;
    const int rulerHeight    = 14;

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

void StepGrid::applyPaintToCell(int lane, int step)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return;

    auto* cell = cells[lane][step].get();
    if (cell->getToggleState() != paintMode)
        cell->setToggleState(paintMode, juce::sendNotification);
}

void StepGrid::mouseDown(const juce::MouseEvent& e)
{
    auto [lane, step] = hitTestCell(e.getPosition());
    if (lane < 0)
        return;

    isPainting    = true;
    paintMode     = !cells[lane][step]->getToggleState();
    lastPaintedLane = lane;
    lastPaintedStep = step;

    applyPaintToCell(lane, step);

    setSelectedStep(lane, step);
    if (onStepSelected)
        onStepSelected(lane, step);
}

void StepGrid::mouseDrag(const juce::MouseEvent& e)
{
    if (!isPainting)
        return;

    auto [lane, step] = hitTestCell(e.getPosition());
    if (lane < 0)
        return;

    if (lane != lastPaintedLane || step != lastPaintedStep)
    {
        lastPaintedLane = lane;
        lastPaintedStep = step;
        applyPaintToCell(lane, step);
    }
}

void StepGrid::mouseUp(const juce::MouseEvent& /*e*/)
{
    isPainting      = false;
    lastPaintedLane = -1;
    lastPaintedStep = -1;
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

StepCell* StepGrid::getCell(int lane, int step)
{
    if (lane < 0 || lane >= numLanes || step < 0 || step >= numSteps)
        return nullptr;
    return cells[lane][step].get();
}

}
