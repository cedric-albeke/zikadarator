#include "engine/FilterEngine.h"

namespace zikada {

FilterEngine::FilterEngine() = default;

void FilterEngine::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;

    const juce::dsp::ProcessSpec monoSpec{sampleRate,
                                          static_cast<juce::uint32>(juce::jmax(1, maxBlockSize)),
                                          1};
    const juce::dsp::ProcessSpec stereoSpec{sampleRate,
                                            static_cast<juce::uint32>(juce::jmax(1, maxBlockSize)),
                                            2};

    filterLeftA.prepare(monoSpec);
    filterRightA.prepare(monoSpec);
    filterLeftB.prepare(monoSpec);
    filterRightB.prepare(monoSpec);

    maxCombDelaySamples = static_cast<float>(juce::jmax(1, static_cast<int>(sampleRate * 0.1)));
    combDelayLine.prepare(stereoSpec);
    combDelayLine.setMaximumDelayInSamples(static_cast<int>(maxCombDelaySamples));

    reset();
    updateFilter();
}

void FilterEngine::reset()
{
    filterLeftA.reset();
    filterRightA.reset();
    filterLeftB.reset();
    filterRightB.reset();
    combDelayLine.reset();
}

void FilterEngine::setFilterType(FilterType type)
{
    if (currentType != type)
    {
        const bool combStateChanged = currentType == FilterType::Comb || type == FilterType::Comb;
        currentType = type;
        if (combStateChanged)
            combDelayLine.reset();
        updateFilter();
    }
}

void FilterEngine::setCutoff(float frequency)
{
    cutoffFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), frequency);
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

    filterLeftA.setType(type);
    filterRightA.setType(type);
    filterLeftB.setType(type);
    filterRightB.setType(type);

    filterLeftA.setCutoffFrequency(cutoffFreq);
    filterRightA.setCutoffFrequency(cutoffFreq);
    filterLeftB.setCutoffFrequency(cutoffFreq);
    filterRightB.setCutoffFrequency(cutoffFreq);

    filterLeftA.setResonance(resonance);
    filterRightA.setResonance(resonance);
    filterLeftB.setResonance(resonance);
    filterRightB.setResonance(resonance);
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
    return processSample(0, input);
}

float FilterEngine::processSampleRight(float input)
{
    if (!isEnabled)
        return input;

    return processSample(1, input);
}

float FilterEngine::processSample(int channel, float input)
{
    if (!isEnabled)
        return input;

    if (currentType == FilterType::Comb)
        return processCombSample(channel, input);

    auto& first = channel == 0 ? filterLeftA : filterRightA;
    auto& second = channel == 0 ? filterLeftB : filterRightB;

    if (currentType == FilterType::BandReject)
        return input - first.processSample(0, input);

    float output = first.processSample(0, input);
    if (isCascadedType())
        output = second.processSample(0, output);

    return output;
}

float FilterEngine::processCombSample(int channel, float input)
{
    const float safeCutoff = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), cutoffFreq);
    const float delaySamples = juce::jlimit(1.0f, maxCombDelaySamples,
                                            static_cast<float>(sampleRate) / safeCutoff);
    const float feedback = juce::jlimit(0.0f, 0.85f, resonance / (resonance + 3.0f));
    const float delayed = combDelayLine.popSample(channel, delaySamples);
    combDelayLine.pushSample(channel, input + delayed * feedback);

    return input * 0.55f + delayed * 0.45f;
}

bool FilterEngine::isCascadedType() const
{
    return currentType == FilterType::LowPass24 || currentType == FilterType::HighPass24;
}

}
