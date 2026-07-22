#include "ui/components/Knob.h"
#include <cmath>

namespace zikada {

Knob::Knob()
{
    setSize(48, 64);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setHelpText("Use the arrow keys to adjust; Home and End select the minimum and maximum.");
}

void Knob::paint(juce::Graphics& g)
{
    const auto* laf    = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());
    const auto  bounds = getLocalBounds();
    const float bw     = static_cast<float>(bounds.getWidth());
    const float bh     = static_cast<float>(bounds.getHeight());

    const float labelH = label.isNotEmpty() ? 11.0f : 0.0f;
    const float valueH = 10.0f;
    const float arcH   = bh - labelH - valueH;

    // Label (above knob, clean)
    if (label.isNotEmpty())
    {
        g.setColour(Colours::white.withAlpha(0.50f));
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(11.0f)
                                 : juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText(label,
                   juce::Rectangle<float>(0.0f, 0.0f, bw, labelH).toNearestInt(),
                   juce::Justification::centred, false);
    }

    const auto arcBounds = juce::Rectangle<float>(0.0f, labelH, bw, arcH).reduced(4.0f);
    const auto centre    = arcBounds.getCentre();
    const float radius   = juce::jmin(arcBounds.getWidth(), arcBounds.getHeight()) / 2.0f - 2.0f;

    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float endAngle   = juce::MathConstants<float>::pi * 2.75f;
    const float arcRange   = endAngle - startAngle;
    const float valueAngle = startAngle + static_cast<float>(getNormalizedValue()) * arcRange;

    const bool isHovering = isMouseOverOrDragging();
    const float arcAlpha = isHovering ? 1.0f : 0.85f;

    // Outer ring (bgSurface with inner highlight)
    juce::Path outerRing;
    outerRing.addCentredArc(centre.x, centre.y, radius, radius,
                            0.0f, startAngle, endAngle, true);
    g.setColour(Colours::bgSurface);
    g.strokePath(outerRing,
                 juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
    g.setColour(Colours::white.withAlpha(0.08f));
    g.strokePath(outerRing,
                 juce::PathStrokeType(1.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    // Background arc (subtle track)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, radius - 2.0f, radius - 2.0f,
                                 0.0f, startAngle, endAngle, true);
    g.setColour(Colours::white.withAlpha(0.06f));
    g.strokePath(backgroundArc,
                 juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    // Value arc (colored indicator with glow)
    if (getNormalizedValue() > 0.001)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, radius - 2.0f, radius - 2.0f,
                               0.0f, startAngle, valueAngle, true);

        // Glow (thick, low alpha) — expands on hover
        const float glowWidth = isHovering ? 7.0f : 5.0f;
        const float glowAlpha = isHovering ? 0.25f : 0.18f;
        g.setColour(accentColour.withAlpha(glowAlpha * arcAlpha));
        g.strokePath(valueArc,
                     juce::PathStrokeType(glowWidth, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));

        // Core arc (precise, high alpha)
        g.setColour(accentColour.withAlpha(0.90f * arcAlpha));
        g.strokePath(valueArc,
                     juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));
    }

    // 3D Center cap
    const float capRadius = radius * 0.60f;
    const auto capBounds = juce::Rectangle<float>(
        centre.x - capRadius, centre.y - capRadius,
        capRadius * 2.0f, capRadius * 2.0f);

    // Cap shadow (gives depth)
    g.setColour(juce::Colours::black.withAlpha(0.40f));
    g.fillEllipse(capBounds.translated(0.0f, 1.0f));

    // Cap body (radial gradient for 3D effect)
    juce::ColourGradient capGrad(
        Colours::panelRaised, capBounds.getX(), capBounds.getY(),
        Colours::bgSurface, capBounds.getRight(), capBounds.getBottom(), true);
    g.setGradientFill(capGrad);
    g.fillEllipse(capBounds);

    // Cap highlight (top edge)
    g.setColour(Colours::white.withAlpha(0.08f));
    g.drawEllipse(capBounds.reduced(1.0f), 1.0f);

    // Cap inner shadow (bottom edge)
    g.setColour(juce::Colours::black.withAlpha(0.30f));
    g.drawEllipse(capBounds.translated(0.0f, 0.5f).reduced(1.0f), 1.0f);

    // Center dot (value indicator, grows on hover)
    const float dotRadius = isHovering ? 3.5f : 2.5f;
    const float dotAlpha = isHovering ? 1.0f : 0.70f;
    g.setColour(accentColour.withAlpha(dotAlpha));
    g.fillEllipse(centre.x - dotRadius, centre.y - dotRadius,
                  dotRadius * 2.0f, dotRadius * 2.0f);

    // Value text (below knob, clean, no background box)
    const auto valueStr  = formatValue();
    const auto valueRect = juce::Rectangle<float>(0.0f, bh - valueH, bw, valueH);

    g.setColour(accentColour.withAlpha(0.72f));
    g.setFont(laf != nullptr ? laf->getSpaceMonoFont(10.0f)
                             : juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText(valueStr, valueRect.toNearestInt(), juce::Justification::centred, false);

    if (hasKeyboardFocus(false))
    {
        g.setColour(accentColour.withAlpha(0.90f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f, 1.5f);
    }
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
        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
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
    setTitle(label);
    setDescription("Adjust the " + label.toLowerCase() + " value.");
    repaint();
}

void Knob::setDisplayMode(KnobDisplayMode newMode)
{
    displayMode = newMode;
    repaint();
}

void Knob::setScaleMode(KnobScaleMode newMode)
{
    scaleMode = newMode;
    value = juce::jlimit(minValue, maxValue, value);
    repaint();
}

double Knob::getNormalizedValue() const
{
    return valueToNormalized(value);
}

double Knob::valueToNormalized(double rawValue) const
{
    if (maxValue <= minValue)
        return 0.0;

    rawValue = juce::jlimit(minValue, maxValue, rawValue);

    if (scaleMode == KnobScaleMode::Logarithmic && minValue > 0.0 && maxValue > minValue)
    {
        const double logMin = std::log(minValue);
        const double logMax = std::log(maxValue);
        return juce::jlimit(0.0, 1.0, (std::log(rawValue) - logMin) / (logMax - logMin));
    }

    return juce::jlimit(0.0, 1.0, (rawValue - minValue) / (maxValue - minValue));
}

double Knob::normalizedToValue(double normalizedValue) const
{
    normalizedValue = juce::jlimit(0.0, 1.0, normalizedValue);

    if (scaleMode == KnobScaleMode::Logarithmic && minValue > 0.0 && maxValue > minValue)
    {
        const double logMin = std::log(minValue);
        const double logMax = std::log(maxValue);
        return std::exp(logMin + normalizedValue * (logMax - logMin));
    }

    return minValue + normalizedValue * (maxValue - minValue);
}

juce::String Knob::formatValue() const
{
    switch (displayMode)
    {
        case KnobDisplayMode::Percent:
            return juce::String(static_cast<int>(std::round(value * 100.0))) + "%";

        case KnobDisplayMode::Hertz:
            if (value >= 1000.0)
                return juce::String(value / 1000.0, value >= 10000.0 ? 0 : 1) + "kHz";
            return juce::String(static_cast<int>(std::round(value))) + "Hz";

        case KnobDisplayMode::Decimal:
            return juce::String(value, value < 1.0 ? 2 : 1);

        case KnobDisplayMode::Seconds:
            if (value < 1.0)
                return juce::String(static_cast<int>(std::round(value * 1000.0))) + "ms";
            return juce::String(value, 1) + "s";

        case KnobDisplayMode::Gain:
            return juce::String(value, value < 1.0 ? 2 : 1) + "x";

        case KnobDisplayMode::Pan:
        {
            if (std::abs(value) < 0.01)
                return "C";

            const auto side = value < 0.0 ? juce::String("L") : juce::String("R");
            return side + juce::String(static_cast<int>(std::round(std::abs(value) * 100.0)));
        }

        case KnobDisplayMode::NormalizedPercent:
        default:
            return juce::String(static_cast<int>(std::round(getNormalizedValue() * 100.0))) + "%";
    }
}

void Knob::setValueAsCompleteGesture(double newValue)
{
    newValue = juce::jlimit(minValue, maxValue, newValue);
    if (newValue == value)
        return;

    if (onDragStart)
        onDragStart();
    setValue(newValue);
    if (onDragEnd)
        onDragEnd();
}

bool Knob::keyPressed(const juce::KeyPress& key)
{
    const int code = key.getKeyCode();
    double normalized = getNormalizedValue();

    if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey)
        normalized += 0.01;
    else if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey)
        normalized -= 0.01;
    else if (code == juce::KeyPress::homeKey)
        normalized = 0.0;
    else if (code == juce::KeyPress::endKey)
        normalized = 1.0;
    else
        return false;

    setValueAsCompleteGesture(normalizedToValue(normalized));
    return true;
}

void Knob::focusGained(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    repaint();
}

void Knob::focusLost(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    repaint();
}

class Knob::AccessibilityValue final : public juce::AccessibilityValueInterface
{
public:
    explicit AccessibilityValue(Knob& owner) : knob(owner) {}

    bool isReadOnly() const override { return false; }
    double getCurrentValue() const override { return knob.getValue(); }
    juce::String getCurrentValueAsString() const override { return knob.formatValue(); }
    void setValue(double newValue) override { knob.setValueAsCompleteGesture(newValue); }
    void setValueAsString(const juce::String& newValue) override
    {
        const auto text = newValue.trim().toLowerCase();
        const bool panPrefix = text.startsWithChar('l') || text.startsWithChar('r');
        const double parsed = (panPrefix ? text.substring(1) : text).getDoubleValue();

        switch (knob.displayMode)
        {
            case KnobDisplayMode::NormalizedPercent:
                setValue(knob.normalizedToValue(parsed / 100.0));
                break;
            case KnobDisplayMode::Percent:
                setValue(parsed / 100.0);
                break;
            case KnobDisplayMode::Hertz:
                setValue(text.contains("khz") ? parsed * 1000.0 : parsed);
                break;
            case KnobDisplayMode::Seconds:
                setValue(text.contains("ms") ? parsed / 1000.0 : parsed);
                break;
            case KnobDisplayMode::Pan:
                setValue(text.startsWithChar('l') ? -std::abs(parsed) / 100.0
                         : text.startsWithChar('r') ?  std::abs(parsed) / 100.0
                                                    : 0.0);
                break;
            case KnobDisplayMode::Decimal:
            case KnobDisplayMode::Gain:
                setValue(parsed);
                break;
        }
    }

    AccessibleValueRange getRange() const override
    {
        if (knob.maxValue <= knob.minValue)
            return {};

        return {{knob.minValue, knob.maxValue}, (knob.maxValue - knob.minValue) / 100.0};
    }

private:
    Knob& knob;
};

std::unique_ptr<juce::AccessibilityHandler> Knob::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::slider,
        juce::AccessibilityActions{},
        juce::AccessibilityHandler::Interfaces{std::make_unique<AccessibilityValue>(*this)});
}

void Knob::mouseDown(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    normalizedOnMouseDown = getNormalizedValue();
    if (onDragStart)
        onDragStart();
}

void Knob::mouseDrag(const juce::MouseEvent& event)
{
    const auto deltaY = -event.getDistanceFromDragStartY();
    const auto normalizedDelta = static_cast<double>(deltaY) / 200.0;
    setValue(normalizedToValue(normalizedOnMouseDown + normalizedDelta));
}

void Knob::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (onDragEnd)
        onDragEnd();
}

void Knob::mouseDoubleClick(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (onDefaultValueRequested)
        onDefaultValueRequested(defaultValue);
    else
        setValue(defaultValue);
}

KnobParameterAttachment::KnobParameterAttachment(juce::RangedAudioParameter& parameter,
                                                 Knob& control,
                                                 juce::UndoManager* undoManager)
    : knob(control),
      attachment(parameter, [this](float value) { setValue(value); }, undoManager)
{
    knob.onDragStart = [this] { beginGesture(); };
    knob.onValueChange = [this]
    {
        if (!ignoreCallbacks)
            attachment.setValueAsPartOfGesture(static_cast<float>(knob.getValue()));
    };
    knob.onDragEnd = [this] { endGesture(); };
    knob.onDefaultValueRequested = [this](double value) { resetToDefault(value); };
    sendInitialUpdate();
}

KnobParameterAttachment::~KnobParameterAttachment()
{
    endGesture();
    knob.onDragStart = nullptr;
    knob.onValueChange = nullptr;
    knob.onDragEnd = nullptr;
    knob.onDefaultValueRequested = nullptr;
}

void KnobParameterAttachment::sendInitialUpdate()
{
    attachment.sendInitialUpdate();
}

void KnobParameterAttachment::setValue(float newValue)
{
    const juce::ScopedValueSetter<bool> scopedIgnore(ignoreCallbacks, true);
    knob.setValue(newValue);
}

void KnobParameterAttachment::beginGesture()
{
    if (gestureActive)
        return;

    gestureActive = true;
    attachment.beginGesture();
}

void KnobParameterAttachment::endGesture()
{
    if (!gestureActive)
        return;

    gestureActive = false;
    attachment.endGesture();
}

void KnobParameterAttachment::resetToDefault(double defaultValue)
{
    endGesture();
    attachment.setValueAsCompleteGesture(static_cast<float>(defaultValue));
}

}
