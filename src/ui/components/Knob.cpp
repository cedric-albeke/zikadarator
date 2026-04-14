#include "ui/components/Knob.h"

namespace zikada {

Knob::Knob()
{
    setSize(48, 48);
}

void Knob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    auto centre = bounds.getCentre();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 4.0f;
    
    auto startAngle = juce::MathConstants<float>::pi * 1.25f;
    auto endAngle = juce::MathConstants<float>::pi * 2.75f;
    auto range = endAngle - startAngle;
    auto valueAngle = startAngle + static_cast<float>(getNormalizedValue()) * range;
    
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(Colours::darkGrey);
    g.strokePath(backgroundArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, valueAngle, true);
    g.setColour(accentColour);
    g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    auto thumbX = centre.x + radius * std::cos(valueAngle);
    auto thumbY = centre.y + radius * std::sin(valueAngle);
    g.setColour(Colours::white);
    g.fillEllipse(thumbX - 3.0f, thumbY - 3.0f, 6.0f, 6.0f);
    
    g.setColour(Colours::bgPrimary);
    g.fillEllipse(centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f);
    g.setColour(accentColour);
    g.fillEllipse(centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);
}

void Knob::resized()
{
}

void Knob::setValue(double newValue)
{
    newValue = juce::jlimit(minValue, maxValue, newValue);
    if (value != newValue)
    {
        value = newValue;
        repaint();
        if (onValueChange)
            onValueChange();
    }
}

void Knob::setRange(double min, double max)
{
    minValue = min;
    maxValue = max;
    value = juce::jlimit(minValue, maxValue, value);
    repaint();
}

void Knob::setDefaultValue(double newDefault)
{
    defaultValue = newDefault;
}

void Knob::setColour(juce::Colour newColour)
{
    accentColour = newColour;
    repaint();
}

double Knob::getNormalizedValue() const
{
    return (value - minValue) / (maxValue - minValue);
}

void Knob::mouseDown(const juce::MouseEvent& event)
{
    lastMousePos = event.getScreenPosition();
    valueOnMouseDown = value;
}

void Knob::mouseDrag(const juce::MouseEvent& event)
{
    auto deltaY = lastMousePos.y - event.getScreenPosition().getY();
    auto sensitivity = (maxValue - minValue) / 200.0;
    auto newValue = valueOnMouseDown + deltaY * sensitivity;
    setValue(newValue);
}

void Knob::mouseDoubleClick(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    setValue(defaultValue);
}

}
