#pragma once

#include <juce_dsp/juce_dsp.h>

namespace zikada {

class FilterEngine
{
public:
    FilterEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    enum class FilterType
    {
        LowPass12,
        LowPass24,
        HighPass12,
        HighPass24,
        BandPass,
        BandReject,
        Comb,
        NumTypes
    };

    void setFilterType(FilterType type);
    void setCutoff(float frequency);
    void setResonance(float q);
    void setEnabled(bool enabled);

    void process(float* left, float* right, int numSamples);

    float processSampleLeft(float input);
    float processSampleRight(float input);

private:
    double sampleRate{44100.0};
    bool isEnabled{true};
    FilterType currentType{FilterType::LowPass24};
    float cutoffFreq{2000.0f};
    float resonance{0.707f};

    juce::dsp::StateVariableTPTFilter<float> filterLeft;
    juce::dsp::StateVariableTPTFilter<float> filterRight;

    void updateFilter();
};

}