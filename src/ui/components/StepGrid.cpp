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
}

void StepGrid::resized()
{
    auto bounds = getLocalBounds();
    int laneHeight = bounds.getHeight() / numLanes;
    int stepWidth = bounds.getWidth() / numSteps;
    
    for (int lane = 0; lane < numLanes; ++lane)
    {
        auto laneBounds = bounds.removeFromTop(laneHeight);
        for (int step = 0; step < numSteps; ++step)
        {
            cells[lane][step]->setBounds(laneBounds.removeFromLeft(stepWidth).reduced(2));
        }
    }
}

}
