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

            cells[lane][step] = std::move(cell);
        }

        auto knob = std::make_unique<Knob>();
        knob->setRange(0.0, 100.0);
        knob->setDefaultValue(100.0);
        knob->setColour(laneInfos[lane].colour);

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

    auto bounds = getLocalBounds();
    int laneHeight = bounds.getHeight() / numLanes;
    int labelWidth = 72;
    int gridStartX = labelWidth + 8;

    for (int lane = 0; lane < numLanes; ++lane)
    {
        auto laneBounds = bounds.removeFromTop(laneHeight);
        auto labelBounds = laneBounds.removeFromLeft(labelWidth);

        g.setColour(Colours::bgSurface);
        g.fillRoundedRectangle(labelBounds.toFloat().reduced(2.0f), 4.0f);
        g.setColour(laneInfos[lane].colour);
        g.drawRoundedRectangle(labelBounds.toFloat().reduced(2.0f), 4.0f, 1.5f);

        g.setColour(laneInfos[lane].colour);
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
        g.drawText(juce::String(laneInfos[lane].name), labelBounds, juce::Justification::centred, false);
    }
}

void StepGrid::resized()
{
    auto bounds = getLocalBounds();
    int laneHeight = bounds.getHeight() / numLanes;
    int labelWidth = 72;
    int gridStartX = labelWidth + 8;
    int knobStripWidth = 56;
    int stepWidth = (bounds.getWidth() - gridStartX - knobStripWidth) / numSteps;

    for (int lane = 0; lane < numLanes; ++lane)
    {
        auto laneBounds = bounds.removeFromTop(laneHeight);
        laneBounds.removeFromLeft(gridStartX);

        for (int step = 0; step < numSteps; ++step)
        {
            cells[lane][step]->setBounds(laneBounds.removeFromLeft(stepWidth).reduced(2));
        }

        mixKnobs[lane]->setBounds(laneBounds.removeFromRight(knobStripWidth).reduced(4, 2));
    }
}

}