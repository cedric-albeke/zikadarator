#include "engine/LoopEngine.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>

namespace zikada {

void LoopEngine::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    maxLoopSamples = juce::jmax(1, static_cast<int>(std::ceil(sampleRate * MaximumLoopSeconds)));
    historySizeSamples = maxLoopSamples + juce::jmax(1, maxBlockSize);
    bufferL.prepare(historySizeSamples);
    bufferR.prepare(historySizeSamples);
    loopBufferL.assign(static_cast<size_t>(maxLoopSamples), 0.0f);
    loopBufferR.assign(static_cast<size_t>(maxLoopSamples), 0.0f);
    reset();
}

void LoopEngine::reset()
{
    bufferL.reset();
    bufferR.reset();
    phase = 0.0f;
    snapshotPending = false;
    captureAnchored = false;
    captureProgress = 0;
    captureFramesThisChunk = 0;
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
                                     maxLoopSamples,
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
    captureAnchored = false;
    captureProgress = 0;
}

void LoopEngine::beginProcessChunk(int numSamples)
{
    captureBudgetFrames = juce::jmax(2048, numSamples);
    captureFramesThisChunk = 0;
}

void LoopEngine::captureInput(const float* left, const float* right, int numSamples)
{
    if (snapshotPending && captureAnchored)
        servicePendingCapture();

    bufferL.write(left, numSamples);
    bufferR.write(right, numSamples);

    if (snapshotPending && !captureAnchored)
        beginPendingCapture();

    servicePendingCapture();
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

void LoopEngine::beginPendingCapture()
{
    ensureLoopBufferSize();

    if (loopBufferL.empty() || loopBufferR.empty())
    {
        snapshotPending = false;
        captureAnchored = false;
        hasSnapshot = false;
        return;
    }

    captureStartAbsolute = bufferL.getTotalSamplesWritten() - loopLengthSamples;
    captureProgress = 0;
    captureAnchored = true;
    hasSnapshot = true;
}

void LoopEngine::servicePendingCapture()
{
    if (!snapshotPending || !captureAnchored)
        return;

    const int remainingBudget = juce::jmax(0, captureBudgetFrames - captureFramesThisChunk);
    const int frames = juce::jmin(remainingBudget, loopLengthSamples - captureProgress);

    for (int i = 0; i < frames; ++i)
    {
        const int destination = captureProgress + i;
        const auto source = captureStartAbsolute + destination;
        loopBufferL[static_cast<size_t>(destination)] = bufferL.getSampleAtAbsolute(source);
        loopBufferR[static_cast<size_t>(destination)] = bufferR.getSampleAtAbsolute(source);
    }

    captureProgress += frames;
    captureFramesThisChunk += frames;

    if (captureProgress >= loopLengthSamples)
    {
        snapshotPending = false;
        captureAnchored = false;
    }
}

float LoopEngine::getCanonicalSample(const std::vector<float>& buffer,
                                     const RealtimeRingBuffer& history,
                                     int index) const
{
    const int wrapped = (index % loopLengthSamples + loopLengthSamples) % loopLengthSamples;
    if (!snapshotPending || wrapped < captureProgress)
        return buffer[static_cast<size_t>(wrapped)];

    return history.getSampleAtAbsolute(captureStartAbsolute + wrapped);
}

float LoopEngine::readRawLoopSample(const std::vector<float>& buffer,
                                    const RealtimeRingBuffer& history,
                                    float phaseIndex) const
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

    const float a = getCanonicalSample(buffer, history, index0);
    const float b = getCanonicalSample(buffer, history, index1);
    return a + (b - a) * fraction;
}

float LoopEngine::readLoopSample(const std::vector<float>& buffer,
                                 const RealtimeRingBuffer& history,
                                 float phaseIndex) const
{
    float sample = readRawLoopSample(buffer, history, phaseIndex);

    if (edgeFadeSamples > 1 && loopLengthSamples > edgeFadeSamples * 2)
    {
        const float fadeStart = static_cast<float>(loopLengthSamples - edgeFadeSamples);
        if (phaseIndex >= fadeStart)
        {
            const float amount = juce::jlimit(0.0f, 1.0f, (phaseIndex - fadeStart) / static_cast<float>(edgeFadeSamples));
            const float wrapSample = readRawLoopSample(buffer, history, 0.0f);
            sample = sample * (1.0f - amount) + wrapSample * amount;
        }
    }

    return sample;
}

void LoopEngine::process(float* left, float* right, int numSamples)
{
    if (!enabled)
        return;

    if (snapshotPending && !captureAnchored)
        beginPendingCapture();

    servicePendingCapture();

    if (!hasSnapshot)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float phaseIndex = juce::jlimit(0.0f, static_cast<float>(loopLengthSamples - 1), phase);
        const float loopL = readLoopSample(loopBufferL, bufferL, phaseIndex);
        const float loopR = readLoopSample(loopBufferR, bufferR, phaseIndex);
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
