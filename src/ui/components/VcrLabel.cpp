#include "ui/components/VcrLabel.h"

namespace zikada {

VcrLabel::VcrLabel()
    : font(juce::FontOptions().withHeight(12.0f))
{
}

VcrLabel::VcrLabel(const juce::String& t)
    : text(t), font(juce::FontOptions().withHeight(12.0f))
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
    const auto* zikadaLookAndFeel = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());
    g.setFont(zikadaLookAndFeel != nullptr ? zikadaLookAndFeel->getVcrFont(font.getHeight())
                                           : font.withHeight(font.getHeight()));
    g.drawText(text, getLocalBounds(), juce::Justification::centred, true);
}

}
