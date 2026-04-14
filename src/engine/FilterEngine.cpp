#include "engine/FilterEngine.h"

namespace zikada {

FilterEngine::FilterEngine() = default;

void FilterEngine::prepare(double sr, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);
    sampleRate = sr;

    filterLeft.prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 1});
    filterRight.prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 1});

    reset();
    updateFilter();
}

void FilterEngine::reset()
{
    filterLeft.reset();
    filterRight.reset();
}

void FilterEngine::setFilterType(FilterType type)
{
    if (currentType != type)
    {
        currentType = type;
        updateFilter();
    }
}

void FilterEngine::setCutoff(float frequency)
{
    cutoffFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.5), frequency);
    updateFilter();
}

void FilterEngine::setResonance(float q)
{
    resonance = juce::jlimit(0.1f, 10.0f, q);
    updateFilter();
}

void FilterEngine::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

void FilterEngine::updateFilter()
{
    using Type = juce::dsp::StateVariableTPTFilter<float>::Type;

    Type type = Type::lowpass;

    switch (currentType)
    {
        case FilterType::LowPass12:
        case FilterType::LowPass24:
            type = Type::lowpass;
            break;
        case FilterType::HighPass12:
        case FilterType::HighPass24:
            type = Type::highpass;
            break;
        case FilterType::BandPass:
            type = Type::bandpass;
            break;
        case FilterType::BandReject:
            type = Type::bandpass;
            break;
        default:
            type = Type::lowpass;
            break;
    }

    filterLeft.setType(type);
    filterRight.setType(type);
    filterLeft.setCutoffFrequency(cutoffFreq);
    filterRight.setCutoffFrequency(cutoffFreq);
    filterLeft.setResonance(resonance);
    filterRight.setResonance(resonance);
}

void FilterEngine::process(float* left, float* right, int numSamples)
{
    if (!isEnabled)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = processSampleLeft(left[i]);
        right[i] = processSampleRight(right[i]);
    }
}

float FilterEngine::processSampleLeft(float input)
{
    if (!isEnabled)
        return input;

    float out = filterLeft.processSample(0, input);

    if (currentType == FilterType::BandReject)
        out = -out;

    return out;
}

float FilterEngine::processSampleRight(float input)
{
    if (!isEnabled)
        return input;

    float out = filterRight.processSample(0, input);

    if (currentType == FilterType::BandReject)
        out = -out;

    return out;
}

}