#include "ui/components/StepGrid.h"

namespace zikada {

StepGrid::StepGrid()
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
            cells[lane][step] = std::move(cell);
        }
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
    int stepWidth = (bounds.getWidth() - gridStartX) / numSteps;
    
    for (int lane = 0; lane < numLanes; ++lane)
    {
        auto laneBounds = bounds.removeFromTop(laneHeight);
        laneBounds.removeFromLeft(gridStartX);
        
        for (int step = 0; step < numSteps; ++step)
        {
            cells[lane][step]->setBounds(laneBounds.removeFromLeft(stepWidth).reduced(2));
        }
    }
}

}
