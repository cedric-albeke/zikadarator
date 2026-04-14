#pragma once

#include "../ZikadaLookAndFeel.h"

namespace zikada {

class Knob : public juce::Component
{
public:
    Knob();
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setValue(double newValue);
    double getValue() const { return value; }
    
    void setRange(double min, double max);
    void setDefaultValue(double newDefault);
    
    void setColour(juce::Colour newColour);
    void setLabel(const juce::String& lbl);
    
    std::function<void()> onValueChange;

private:
    double value{0.5};
    double minValue{0.0};
    double maxValue{1.0};
    double defaultValue{0.5};
    juce::Colour accentColour{Colours::neonGreen};
    juce::String label;
    
    double getNormalizedValue() const;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    
    juce::Point<int> lastMousePos;
    double valueOnMouseDown{0.0};
};

}
