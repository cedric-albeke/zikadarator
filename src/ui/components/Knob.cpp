#include "ui/components/Knob.h"

namespace zikada {

Knob::Knob()
{
    setSize(48, 64);
}

void Knob::paint(juce::Graphics& g)
{
    const auto* laf    = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());
    const auto  bounds = getLocalBounds();
    const float bw     = static_cast<float>(bounds.getWidth());
    const float bh     = static_cast<float>(bounds.getHeight());

    const float labelH = label.isNotEmpty() ? 12.0f : 0.0f;
    const float valueH = 14.0f;
    const float arcH   = bh - labelH - valueH;

    if (label.isNotEmpty())
    {
        g.setColour(accentColour.withAlpha(0.72f));
        g.setFont(laf != nullptr ? laf->getVcrFont(10.0f)
                                 : juce::Font(juce::FontOptions().withHeight(10.0f)));
        g.drawText(label,
                   juce::Rectangle<float>(0.0f, 0.0f, bw, labelH).toNearestInt(),
                   juce::Justification::centred, false);
    }

    const auto arcBounds = juce::Rectangle<float>(0.0f, labelH, bw, arcH).reduced(4.0f);
    const auto centre    = arcBounds.getCentre();
    const float radius   = juce::jmin(arcBounds.getWidth(), arcBounds.getHeight()) / 2.0f - 1.0f;

    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float endAngle   = juce::MathConstants<float>::pi * 2.75f;
    const float arcRange   = endAngle - startAngle;
    const float valueAngle = startAngle + static_cast<float>(getNormalizedValue()) * arcRange;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, radius, radius,
                                 0.0f, startAngle, endAngle, true);
    g.setColour(Colours::white.withAlpha(0.10f));
    g.strokePath(backgroundArc,
                 juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    if (getNormalizedValue() > 0.001)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, radius, radius,
                               0.0f, startAngle, valueAngle, true);

        g.setColour(accentColour.withAlpha(0.20f));
        g.strokePath(valueArc,
                     juce::PathStrokeType(7.0f, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));

        g.setColour(accentColour);
        g.strokePath(valueArc,
                     juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));
    }

    // JUCE addCentredArc measures angles clockwise from 12 o'clock,
    // but cos/sin use counter-clockwise from 3 o'clock.
    // Offset by -pi/2 to convert JUCE angle convention to trig convention.
    const float trigAngle = valueAngle - juce::MathConstants<float>::halfPi;
    const float thumbX = centre.x + radius * std::cos(trigAngle);
    const float thumbY = centre.y + radius * std::sin(trigAngle);
    g.setColour(Colours::white.withAlpha(0.88f));
    g.fillEllipse(thumbX - 2.5f, thumbY - 2.5f, 5.0f, 5.0f);

    g.setColour(Colours::displayBezel);
    g.fillEllipse(centre.x - 3.5f, centre.y - 3.5f, 7.0f, 7.0f);
    g.setColour(accentColour.withAlpha(0.90f));
    g.fillEllipse(centre.x - 1.5f, centre.y - 1.5f, 3.0f, 3.0f);

    const int pct = static_cast<int>(std::round(getNormalizedValue() * 100.0));
    const auto valueStr  = juce::String(pct) + "%";
    const auto valueRect = juce::Rectangle<float>(0.0f, bh - valueH, bw, valueH);

    g.setColour(Colours::white.withAlpha(0.05f));
    g.fillRoundedRectangle(valueRect.reduced(5.0f, 2.0f), 2.0f);

    g.setColour(accentColour.withAlpha(0.88f));
    g.setFont(laf != nullptr ? laf->getSpaceMonoFont(12.0f, true)
                             : juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    g.drawText(valueStr, valueRect.toNearestInt(), juce::Justification::centred, false);
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

void Knob::setLabel(const juce::String& lbl)
{
    label = lbl;
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
