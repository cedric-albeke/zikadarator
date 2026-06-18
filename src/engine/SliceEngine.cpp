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
    
    auto maxSeconds = 10.0;
    auto maxSamples = static_cast<int>(maxSeconds * sampleRate);
    
    leftBuffer.prepare(maxSamples);
    rightBuffer.prepare(maxSamples);
    
    const auto maxSliceDuration = (60.0 / kMinPreparedTempoBpm) / 4.0;
    const auto playbackCapacity = juce::jmax(maxBlockSize,
                                             static_cast<int>(std::ceil(maxSliceDuration * sampleRate)));
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
    
    for (int i = 0; i < playbackLength; ++i)
    {
        const int sampleAge = getPlaybackSampleAge(sliceStartSamples, i);
        playbackBufferLeft[i] = leftBuffer.getSampleAgo(sampleAge);
        playbackBufferRight[i] = rightBuffer.getSampleAgo(sampleAge);
    }
    
    playbackPosition = 0;
    edgeFadeSamples = playbackLength > 8 ? juce::jlimit(1, 64, playbackLength / 16) : 0;
    isPlayingSlice = true;
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
        outputLeft[i] = playbackBufferLeft[static_cast<size_t>(position)] * gain;
        outputRight[i] = playbackBufferRight[static_cast<size_t>(position)] * gain;
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

int SliceEngine::getPlaybackSampleAge(int sliceStartSamples, int position) const
{
    if (playbackLength <= 0)
        return sliceStartSamples;

    switch (playbackMode)
    {
        case PlaybackMode::Reverse:
            return sliceStartSamples + juce::jlimit(0, playbackLength - 1, position);

        case PlaybackMode::Repeat:
        case PlaybackMode::Stutter:
        {
            const int grainLength = juce::jmax(1, playbackLength / playbackRepeatCount);
            const int grainPosition = position % grainLength;
            return sliceStartSamples + (playbackLength - 1 - grainPosition);
        }

        case PlaybackMode::Forward:
        default:
            return sliceStartSamples + (playbackLength - 1 - position);
    }
}

}
