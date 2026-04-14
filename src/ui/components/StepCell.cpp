#include "ui/components/StepCell.h"

namespace zikada {

StepCell::StepCell(int lane, int step)
    : Button(""), laneIndex(lane), stepIndex(step)
{
    setClickingTogglesState(true);
}

void StepCell::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat();
    
    juce::Colour baseColour = active ? laneInfos[laneIndex].colour : Colours::bgSurface;
    
    if (highlighted)
        baseColour = baseColour.brighter(0.1f);
    if (down)
        baseColour = baseColour.darker(0.1f);
    
    g.setColour(baseColour);
    g.fillRect(bounds);
    
    g.setColour(Colours::white10);
    g.drawRect(bounds, 1.0f);
    
    if (active)
    {
        g.setColour(Colours::white);
        g.setFont(juce::Font(10.0f));
        g.drawText(juce::String(stepIndex + 1), bounds, juce::Justification::centred, false);
    }
}

void StepCell::setActive(bool a)
{
    active = a;
    repaint();
}

void StepCell::setTied(bool t)
{
    tied = t;
    repaint();
}

}
