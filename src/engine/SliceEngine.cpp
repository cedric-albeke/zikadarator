#include "engine/SliceEngine.h"
#include <juce_core/juce_core.h>

namespace zikada {

namespace {
    constexpr double kMinPreparedTempoBpm = 20.0;
    constexpr double kMaxPreparedTempoBpm = 300.0;
}

SliceEngine::SliceEngine() = default;

void SliceEngine::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;
    
    const auto maxSliceDuration = (60.0 / kMinPreparedTempoBpm) / 4.0;
    const auto playbackCapacity = juce::jmax(maxBlockSize,
                                             static_cast<int>(std::ceil(maxSliceDuration * sampleRate)));
    const auto historyCapacity = playbackCapacity * 16 + juce::jmax(1, maxBlockSize);
    leftBuffer.prepare(historyCapacity);
    rightBuffer.prepare(historyCapacity);
    playbackBufferLeft.assign(static_cast<size_t>(playbackCapacity), 0.0f);
    playbackBufferRight.assign(static_cast<size_t>(playbackCapacity), 0.0f);
    
    setTempo(tempoBPM);
    reset();
}

void SliceEngine::reset()
{
    leftBuffer.reset();
    rightBuffer.reset();
    playbackPosition = 0;
    playbackLength = 0;
    edgeFadeSamples = 0;
    isPlayingSlice = false;
    capturePending = false;
    captureProgress = 0;
    captureFramesThisChunk = 0;
}

void SliceEngine::setTempo(double bpm)
{
    tempoBPM = juce::jlimit(kMinPreparedTempoBpm, kMaxPreparedTempoBpm, bpm);
    auto beatDuration = 60.0 / tempoBPM;
    auto sliceDuration = beatDuration / 4.0;
    samplesPerSlice = sliceDuration * sampleRate;
    maxSliceSamples = static_cast<int>(std::ceil(samplesPerSlice));

    if (! playbackBufferLeft.empty())
        maxSliceSamples = juce::jmin(maxSliceSamples, static_cast<int>(playbackBufferLeft.size()));
}

void SliceEngine::setPlaybackMode(PlaybackMode mode, int repeatCount)
{
    playbackMode = mode;
    playbackRepeatCount = juce::jlimit(1, 16, repeatCount);
}

void SliceEngine::writeToBuffer(const float* inputLeft, const float* inputRight, int numSamples)
{
    servicePendingCapture();
    leftBuffer.write(inputLeft, numSamples);
    rightBuffer.write(inputRight, numSamples);
}

void SliceEngine::triggerSlice(int sliceIndex)
{
    sliceIndex = juce::jlimit(0, 15, sliceIndex);
    
    auto sliceStartSamples = static_cast<int>(sliceIndex * samplesPerSlice);
    auto sliceEndSamples = static_cast<int>((sliceIndex + 1) * samplesPerSlice);
    playbackLength = sliceEndSamples - sliceStartSamples;
    playbackLength = juce::jmin(playbackLength, maxSliceSamples);
    playbackLength = juce::jmin(playbackLength, static_cast<int>(playbackBufferLeft.size()));
    playbackLength = juce::jmin(playbackLength, static_cast<int>(playbackBufferRight.size()));
    
    captureStartAbsolute = leftBuffer.getTotalSamplesWritten() - sliceStartSamples - playbackLength;
    captureProgress = 0;
    capturePending = playbackLength > 0;
    playbackPosition = 0;
    edgeFadeSamples = playbackLength > 8 ? juce::jlimit(1, 64, playbackLength / 16) : 0;
    isPlayingSlice = true;
    servicePendingCapture();
}

void SliceEngine::beginProcessChunk(int numSamples)
{
    captureBudgetFrames = juce::jmax(2048, numSamples);
    captureFramesThisChunk = 0;
}

void SliceEngine::servicePendingCapture()
{
    if (!capturePending)
        return;

    const int remainingBudget = juce::jmax(0, captureBudgetFrames - captureFramesThisChunk);
    const int frames = juce::jmin(remainingBudget, playbackLength - captureProgress);
    for (int i = 0; i < frames; ++i)
    {
        const int destination = captureProgress + i;
        const auto source = captureStartAbsolute + destination;
        playbackBufferLeft[static_cast<size_t>(destination)] = leftBuffer.getSampleAtAbsolute(source);
        playbackBufferRight[static_cast<size_t>(destination)] = rightBuffer.getSampleAtAbsolute(source);
    }

    captureProgress += frames;
    captureFramesThisChunk += frames;
    if (captureProgress >= playbackLength)
        capturePending = false;
}

void SliceEngine::process(float* outputLeft, float* outputRight, int numSamples)
{
    if (!isPlayingSlice)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            outputLeft[i] = 0.0f;
            outputRight[i] = 0.0f;
        }
        return;
    }
    
    int samplesToPlay = juce::jmin(numSamples, playbackLength - playbackPosition);
    
    for (int i = 0; i < samplesToPlay; ++i)
    {
        const int position = playbackPosition + i;
        const float gain = getEdgeFadeGain(position);
        outputLeft[i] = getPlaybackSample(playbackBufferLeft, leftBuffer, position) * gain;
        outputRight[i] = getPlaybackSample(playbackBufferRight, rightBuffer, position) * gain;
    }
    
    for (int i = samplesToPlay; i < numSamples; ++i)
    {
        outputLeft[i] = 0.0f;
        outputRight[i] = 0.0f;
    }
    
    playbackPosition += samplesToPlay;
    if (playbackPosition >= playbackLength)
        isPlayingSlice = false;
}

float SliceEngine::getEdgeFadeGain(int position) const
{
    if (edgeFadeSamples <= 0)
        return 1.0f;

    float gain = 1.0f;
    if (position < edgeFadeSamples)
        gain = juce::jlimit(0.0f, 1.0f, static_cast<float>(position) / static_cast<float>(edgeFadeSamples));

    const int samplesUntilEnd = playbackLength - 1 - position;
    if (samplesUntilEnd < edgeFadeSamples)
    {
        const float fadeOut = juce::jlimit(0.0f, 1.0f,
                                           static_cast<float>(samplesUntilEnd) / static_cast<float>(edgeFadeSamples));
        gain = juce::jmin(gain, fadeOut);
    }

    return gain;
}

int SliceEngine::getCanonicalIndex(int position) const
{
    if (playbackLength <= 0)
        return 0;

    switch (playbackMode)
    {
        case PlaybackMode::Reverse:
            return playbackLength - 1 - juce::jlimit(0, playbackLength - 1, position);

        case PlaybackMode::Repeat:
        case PlaybackMode::Stutter:
        {
            const int grainLength = juce::jmax(1, playbackLength / playbackRepeatCount);
            const int grainPosition = position % grainLength;
            return grainPosition;
        }

        case PlaybackMode::Forward:
        default:
            return juce::jlimit(0, playbackLength - 1, position);
    }
}

float SliceEngine::getPlaybackSample(const std::vector<float>& buffer,
                                     const RealtimeRingBuffer& history,
                                     int position) const
{
    const int canonicalIndex = getCanonicalIndex(position);
    if (!capturePending || canonicalIndex < captureProgress)
        return buffer[static_cast<size_t>(canonicalIndex)];

    return history.getSampleAtAbsolute(captureStartAbsolute + canonicalIndex);
}

}
