#include "ui/components/VcrLabel.h"

namespace zikada {

VcrLabel::VcrLabel() = default;

VcrLabel::VcrLabel(const juce::String& t)
    : text(t)
{
}

void VcrLabel::setText(const juce::String& t)
{
    text = t;
    repaint();
}

void VcrLabel::setColour(juce::Colour c)
{
    colour = c;
    repaint();
}

void VcrLabel::paint(juce::Graphics& g)
{
    g.setColour(colour);
    g.setFont(font.withHeight(12.0f));
    g.drawText(text, getLocalBounds(), juce::Justification::centred, true);
}

}
