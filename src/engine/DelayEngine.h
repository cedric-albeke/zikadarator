#pragma once

#include <juce_dsp/juce_dsp.h>

namespace zikada {

class DelayEngine
{
public:
    DelayEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setDelayTime(float seconds);
    void setFeedback(float feedback);
    void setMix(float mix);
    void setEnabled(bool enabled);

    void process(float* left, float* right, int numSamples);

private:
    double sampleRate{44100.0};
    bool isEnabled{true};

    float delayTimeSec{0.5f};
    float feedback{0.4f};
    float mix{0.5f};
    float maxDelaySamples{1.0f};
    bool hasProcessed{false};

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySamplesSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> feedbackSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
};

}
