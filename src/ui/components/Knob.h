#pragma once

#include "../ZikadaLookAndFeel.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zikada {

enum class KnobDisplayMode
{
    NormalizedPercent,
    Percent,
    Hertz,
    Decimal,
    Seconds,
    Gain,
    Pan
};

enum class KnobScaleMode
{
    Linear,
    Logarithmic
};

class Knob : public juce::Component
{
public:
    Knob();
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    
    void setValue(double newValue);
    double getValue() const { return value; }
    
    void setRange(double min, double max);
    void setDefaultValue(double newDefault);
    
    void setColour(juce::Colour newColour);
    void setLabel(const juce::String& lbl);
    void setDisplayMode(KnobDisplayMode newMode);
    void setScaleMode(KnobScaleMode newMode);
    
    std::function<void()> onValueChange;
    std::function<void()> onDragStart;
    std::function<void()> onDragEnd;
    std::function<void(double)> onDefaultValueRequested;

private:
    class AccessibilityValue;

    double value{0.5};
    double minValue{0.0};
    double maxValue{1.0};
    double defaultValue{0.5};
    juce::Colour accentColour{Colours::neonGreen};
    juce::String label;
    KnobDisplayMode displayMode{KnobDisplayMode::NormalizedPercent};
    KnobScaleMode scaleMode{KnobScaleMode::Linear};
    
    double getNormalizedValue() const;
    double valueToNormalized(double rawValue) const;
    double normalizedToValue(double normalizedValue) const;
    juce::String formatValue() const;
    void setValueAsCompleteGesture(double newValue);
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    
    double normalizedOnMouseDown{0.0};
};

class KnobParameterAttachment
{
public:
    KnobParameterAttachment(juce::RangedAudioParameter& parameter,
                            Knob& knob,
                            juce::UndoManager* undoManager = nullptr);
    ~KnobParameterAttachment();

    void sendInitialUpdate();

private:
    void setValue(float newValue);
    void beginGesture();
    void endGesture();
    void resetToDefault(double defaultValue);

    Knob& knob;
    juce::ParameterAttachment attachment;
    bool ignoreCallbacks{false};
    bool gestureActive{false};
};

}
