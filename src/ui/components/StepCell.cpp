#include "ui/components/StepCell.h"

namespace zikada {

StepCell::StepCell(int lane, int step)
    : Button(""), laneIndex(lane), stepIndex(step)
{
    setClickingTogglesState(true);
}

void StepCell::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto cornerSize = 3.0f;
    
    juce::Colour baseColour = active ? laneInfos[laneIndex].colour : Colours::bgSurface;
    
    if (highlighted)
        baseColour = baseColour.brighter(0.15f);
    if (down)
        baseColour = baseColour.darker(0.15f);
    
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerSize);
    
    if (active)
    {
        g.setColour(Colours::white);
        g.drawRoundedRectangle(bounds, cornerSize, 1.5f);
        
        auto glowBounds = bounds.expanded(2.0f);
        juce::ColourGradient glow(baseColour.withAlpha(0.4f), bounds.getCentreX(), bounds.getCentreY(),
                                  baseColour.withAlpha(0.0f), glowBounds.getRight(), glowBounds.getBottom(), true);
        g.setGradientFill(glow);
        g.fillRoundedRectangle(glowBounds, cornerSize + 1.0f);
        
        g.setColour(Colours::bgPrimary);
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold")));
        g.drawText(juce::String(stepIndex + 1), bounds, juce::Justification::centred, false);
    }
    else
    {
        g.setColour(Colours::white10);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        
        g.setColour(Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
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
