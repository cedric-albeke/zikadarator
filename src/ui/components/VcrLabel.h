#pragma once

#include "../ZikadaLookAndFeel.h"

namespace zikada {

class VcrLabel : public juce::Component
{
public:
    VcrLabel();
    explicit VcrLabel(const juce::String& text);

    void setText(const juce::String& newText);
    const juce::String& getText() const { return text; }

    void setColour(juce::Colour newColour);
    juce::Colour getColour() const { return colour; }

    void paint(juce::Graphics& g) override;

private:
    juce::String text;
    juce::Colour colour{Colours::neonGreen};
    juce::Font font;
};

}
