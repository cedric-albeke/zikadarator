#include "engine/FilterEngine.h"

#include <cmath>

namespace zikada {

FilterEngine::FilterEngine() = default;

void FilterEngine::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;

    const juce::dsp::ProcessSpec monoSpec{sampleRate,
                                          static_cast<juce::uint32>(juce::jmax(1, maxBlockSize)),
                                          1};
    filterLeftA.prepare(monoSpec);
    filterRightA.prepare(monoSpec);
    filterLeftB.prepare(monoSpec);
    filterRightB.prepare(monoSpec);

    maxCombDelaySamples = static_cast<float>(juce::jmax(1, static_cast<int>(sampleRate * 0.1)));
    combDelayLine.prepare(static_cast<int>(maxCombDelaySamples));

    reset();
    updateFilterType();
    updateFilterParameters();
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
        updateFilterType();
        updateFilterParameters();
    }
}

void FilterEngine::setParameters(float frequency, float q)
{
    const float newCutoff = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), frequency);
    const float newResonance = juce::jlimit(0.1f, 10.0f, q);
    if (newCutoff == cutoffFreq && newResonance == resonance)
        return;

    cutoffFreq = newCutoff;
    resonance = newResonance;
    updateFilterParameters();
}

void FilterEngine::setCutoff(float frequency)
{
    setParameters(frequency, resonance);
}

void FilterEngine::setResonance(float q)
{
    setParameters(cutoffFreq, q);
}

void FilterEngine::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

void FilterEngine::updateFilterType()
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

    if (isCascadedType())
    {
        filterLeftB.setType(type);
        filterRightB.setType(type);
    }
}

void FilterEngine::updateFilterParameters()
{
#if defined(ZIKADA_ENABLE_TEST_HOOKS)
    ++parameterUpdateCount;
#endif

    filterLeftA.setCutoffFrequency(cutoffFreq);
    filterRightA.setCutoffFrequency(cutoffFreq);
    filterLeftA.setResonance(resonance);
    filterRightA.setResonance(resonance);

    if (isCascadedType())
    {
        filterLeftB.setCutoffFrequency(cutoffFreq);
        filterRightB.setCutoffFrequency(cutoffFreq);
        filterLeftB.setResonance(resonance);
        filterRightB.setResonance(resonance);
    }

    combDelaySamples = juce::jlimit(1.0f, maxCombDelaySamples,
                                    static_cast<float>(sampleRate) / cutoffFreq);
    combFeedback = juce::jlimit(0.0f, 0.85f, resonance / (resonance + 3.0f));
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
    const float delayed = combDelayLine.popSample(channel, combDelaySamples);
    combDelayLine.pushSample(channel, input + delayed * combFeedback);

    return input * 0.55f + delayed * 0.45f;
}

bool FilterEngine::isCascadedType() const
{
    return currentType == FilterType::LowPass24 || currentType == FilterType::HighPass24;
}

void FilterEngine::CombDelay::prepare(int maximumDelaySamples)
{
    capacity = juce::jmax(2, maximumDelaySamples + 2);
    for (int channel = 0; channel < 2; ++channel)
    {
        data[static_cast<size_t>(channel)].assign(static_cast<size_t>(capacity), 0.0f);
        generations[static_cast<size_t>(channel)].assign(static_cast<size_t>(capacity), 0);
    }
    writePositions = {};
    currentGenerations = {1, 1};
}

void FilterEngine::CombDelay::reset()
{
    ++currentGenerations[0];
    ++currentGenerations[1];
    writePositions = {};
}

float FilterEngine::CombDelay::popSample(int channel, float delaySamples) const
{
    const auto channelIndex = static_cast<size_t>(juce::jlimit(0, 1, channel));
    float readPosition = static_cast<float>(writePositions[channelIndex]) - delaySamples;
    while (readPosition < 0.0f)
        readPosition += static_cast<float>(capacity);

    const int index0 = static_cast<int>(std::floor(readPosition)) % capacity;
    const int index1 = (index0 + 1) % capacity;
    const float fraction = readPosition - static_cast<float>(index0);
    const auto generation = currentGenerations[channelIndex];
    const float first = generations[channelIndex][static_cast<size_t>(index0)] == generation
        ? data[channelIndex][static_cast<size_t>(index0)] : 0.0f;
    const float second = generations[channelIndex][static_cast<size_t>(index1)] == generation
        ? data[channelIndex][static_cast<size_t>(index1)] : 0.0f;
    return first + (second - first) * fraction;
}

void FilterEngine::CombDelay::pushSample(int channel, float sample)
{
    const auto channelIndex = static_cast<size_t>(juce::jlimit(0, 1, channel));
    const auto position = static_cast<size_t>(writePositions[channelIndex]);
    data[channelIndex][position] = sample;
    generations[channelIndex][position] = currentGenerations[channelIndex];
    writePositions[channelIndex] = (writePositions[channelIndex] + 1) % capacity;
}

}
