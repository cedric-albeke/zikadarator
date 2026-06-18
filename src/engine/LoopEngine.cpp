#include "engine/LoopEngine.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>

namespace zikada {

void LoopEngine::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    historySizeSamples = juce::jmax(maxBlockSize * 32, static_cast<int>(sampleRate * 4.0));
    bufferL.prepare(historySizeSamples);
    bufferR.prepare(historySizeSamples);
    loopBufferL.assign(static_cast<size_t>(historySizeSamples), 0.0f);
    loopBufferR.assign(static_cast<size_t>(historySizeSamples), 0.0f);
    reset();
}

void LoopEngine::reset()
{
    bufferL.reset();
    bufferR.reset();
    phase = 0.0f;
    snapshotPending = false;
    hasSnapshot = false;
    std::fill(loopBufferL.begin(), loopBufferL.end(), 0.0f);
    std::fill(loopBufferR.begin(), loopBufferR.end(), 0.0f);
}

void LoopEngine::setLoopParameters(float loopLengthSeconds,
                                   float playbackRate,
                                   bool reversePlayback,
                                   float wetMix,
                                   float smoothingAmount)
{
    loopLengthSamples = juce::jlimit(1,
                                     juce::jmax(1, historySizeSamples - 1),
                                     static_cast<int>(std::round(loopLengthSeconds * sampleRate)));
    rate = juce::jlimit(0.25f, 4.0f, playbackRate);
    reverse = reversePlayback;
    mix = juce::jlimit(0.0f, 1.0f, wetMix);
    const float smooth = juce::jlimit(0.0f, 1.0f, smoothingAmount);
    edgeFadeSamples = juce::jlimit(0, 512, static_cast<int>(static_cast<float>(loopLengthSamples) * (0.02f + smooth * 0.14f)));
    triggerFadeSamples = juce::jlimit(0, 128, static_cast<int>(static_cast<float>(loopLengthSamples) * (0.01f + smooth * 0.06f)));
    ensureLoopBufferSize();
}

void LoopEngine::setEnabled(bool shouldEnable)
{
    enabled = shouldEnable;
}

void LoopEngine::trigger()
{
    phase = 0.0f;
    triggerFadePosition = 0;
    snapshotPending = true;
}

void LoopEngine::captureInput(const float* left, const float* right, int numSamples)
{
    bufferL.write(left, numSamples);
    bufferR.write(right, numSamples);

    if (snapshotPending)
        refreshLoopSnapshot();
}

void LoopEngine::ensureLoopBufferSize()
{
    const int preparedCapacity = static_cast<int>(juce::jmin(loopBufferL.size(), loopBufferR.size()));
    if (preparedCapacity <= 0)
    {
        loopLengthSamples = 1;
        return;
    }

    loopLengthSamples = juce::jlimit(1, preparedCapacity, loopLengthSamples);
}

void LoopEngine::refreshLoopSnapshot()
{
    ensureLoopBufferSize();

    if (loopBufferL.empty() || loopBufferR.empty())
    {
        snapshotPending = false;
        hasSnapshot = false;
        return;
    }

    for (int i = 0; i < loopLengthSamples; ++i)
    {
        const int samplesAgo = loopLengthSamples - 1 - i;
        loopBufferL[static_cast<size_t>(i)] = bufferL.getSampleAgo(samplesAgo);
        loopBufferR[static_cast<size_t>(i)] = bufferR.getSampleAgo(samplesAgo);
    }

    snapshotPending = false;
    hasSnapshot = true;
    phase = 0.0f;
    triggerFadePosition = 0;
}

float LoopEngine::readRawLoopSample(const std::vector<float>& buffer, float phaseIndex) const
{
    if (loopLengthSamples <= 1 || buffer.empty())
        return 0.0f;

    const float playbackIndex = reverse
        ? static_cast<float>(loopLengthSamples - 1) - phaseIndex
        : phaseIndex;
    const float wrappedIndex = std::fmod(playbackIndex + static_cast<float>(loopLengthSamples),
                                         static_cast<float>(loopLengthSamples));
    const auto index0 = static_cast<int>(std::floor(wrappedIndex)) % loopLengthSamples;
    const auto index1 = (index0 + 1) % loopLengthSamples;
    const auto fraction = wrappedIndex - static_cast<float>(index0);

    const float a = buffer[static_cast<size_t>(index0)];
    const float b = buffer[static_cast<size_t>(index1)];
    return a + (b - a) * fraction;
}

float LoopEngine::readLoopSample(const std::vector<float>& buffer, float phaseIndex) const
{
    float sample = readRawLoopSample(buffer, phaseIndex);

    if (edgeFadeSamples > 1 && loopLengthSamples > edgeFadeSamples * 2)
    {
        const float fadeStart = static_cast<float>(loopLengthSamples - edgeFadeSamples);
        if (phaseIndex >= fadeStart)
        {
            const float amount = juce::jlimit(0.0f, 1.0f, (phaseIndex - fadeStart) / static_cast<float>(edgeFadeSamples));
            const float wrapSample = readRawLoopSample(buffer, 0.0f);
            sample = sample * (1.0f - amount) + wrapSample * amount;
        }
    }

    return sample;
}

void LoopEngine::process(float* left, float* right, int numSamples)
{
    if (!enabled)
        return;

    if (snapshotPending)
        refreshLoopSnapshot();

    if (!hasSnapshot)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float phaseIndex = juce::jlimit(0.0f, static_cast<float>(loopLengthSamples - 1), phase);
        const float loopL = readLoopSample(loopBufferL, phaseIndex);
        const float loopR = readLoopSample(loopBufferR, phaseIndex);
        float effectiveMix = mix;

        if (triggerFadeSamples > 1 && triggerFadePosition < triggerFadeSamples)
        {
            effectiveMix *= static_cast<float>(triggerFadePosition) / static_cast<float>(triggerFadeSamples);
            ++triggerFadePosition;
        }

        left[i] = left[i] * (1.0f - effectiveMix) + loopL * effectiveMix;
        right[i] = right[i] * (1.0f - effectiveMix) + loopR * effectiveMix;

        phase += rate;
        while (phase >= static_cast<float>(loopLengthSamples))
            phase -= static_cast<float>(loopLengthSamples);
    }
}

} // namespace zikada
