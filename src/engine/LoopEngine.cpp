#include "engine/LoopEngine.h"

#include <juce_core/juce_core.h>

namespace zikada {

void LoopEngine::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    historySizeSamples = juce::jmax(maxBlockSize * 32, static_cast<int>(sampleRate * 4.0));
    bufferL.prepare(historySizeSamples);
    bufferR.prepare(historySizeSamples);
    reset();
}

void LoopEngine::reset()
{
    bufferL.reset();
    bufferR.reset();
    phase = 0.0f;
}

void LoopEngine::setLoopParameters(float loopLengthSeconds, float playbackRate, bool reversePlayback, float wetMix)
{
    loopLengthSamples = juce::jlimit(1, juce::jmax(1, historySizeSamples - 1), static_cast<int>(loopLengthSeconds * sampleRate));
    rate = juce::jlimit(0.25f, 4.0f, playbackRate);
    reverse = reversePlayback;
    mix = juce::jlimit(0.0f, 1.0f, wetMix);
}

void LoopEngine::setEnabled(bool shouldEnable)
{
    enabled = shouldEnable;
}

void LoopEngine::trigger()
{
    phase = 0.0f;
}

void LoopEngine::captureInput(const float* left, const float* right, int numSamples)
{
    bufferL.write(left, numSamples);
    bufferR.write(right, numSamples);
}

float LoopEngine::readLoopSample(const RealtimeRingBuffer& buffer, float phaseIndex) const
{
    if (loopLengthSamples <= 1)
        return 0.0f;

    const float safeIndex = reverse
        ? phaseIndex
        : static_cast<float>(loopLengthSamples - 1) - phaseIndex;

    return buffer.getSampleAgoLinear(juce::jlimit(0.0f, static_cast<float>(loopLengthSamples - 1), safeIndex));
}

void LoopEngine::process(float* left, float* right, int numSamples)
{
    if (!enabled)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float phaseIndex = juce::jlimit(0.0f, static_cast<float>(loopLengthSamples - 1), phase);
        const float loopL = readLoopSample(bufferL, phaseIndex);
        const float loopR = readLoopSample(bufferR, phaseIndex);

        left[i] = left[i] * (1.0f - mix) + loopL * mix;
        right[i] = right[i] * (1.0f - mix) + loopR * mix;

        phase += rate;
        while (phase >= static_cast<float>(loopLengthSamples))
            phase -= static_cast<float>(loopLengthSamples);
    }
}

} // namespace zikada
