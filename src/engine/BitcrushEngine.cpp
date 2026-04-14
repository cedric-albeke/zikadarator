#include "engine/BitcrushEngine.h"
#include <cmath>

namespace zikada {

BitcrushEngine::BitcrushEngine() = default;

void BitcrushEngine::prepare(double sr, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);
    sampleRate = sr;
    reset();
}

void BitcrushEngine::reset()
{
    holdL = 0.0f;
    holdR = 0.0f;
    holdCounter = 0;
}

void BitcrushEngine::setBitDepth(float bits)
{
    bitDepth = juce::jlimit(1.0f, 16.0f, bits);
}

void BitcrushEngine::setSampleRateReduction(float sr)
{
    reducedSampleRate = juce::jlimit(100.0f, static_cast<float>(sampleRate), sr);
}

void BitcrushEngine::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

void BitcrushEngine::process(float* left, float* right, int numSamples)
{
    if (!isEnabled)
        return;

    const float levels = std::pow(2.0f, bitDepth) - 1.0f;
    const int holdSamples = juce::jmax(1, static_cast<int>(sampleRate / reducedSampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        if (holdCounter <= 0)
        {
            holdL = std::round(left[i]  * levels) / levels;
            holdR = std::round(right[i] * levels) / levels;
            holdCounter = holdSamples;
        }

        left[i]  = holdL;
        right[i] = holdR;
        --holdCounter;
    }
}

}
