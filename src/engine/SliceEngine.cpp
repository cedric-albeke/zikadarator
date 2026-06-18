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
        playbackBufferLeft[i] = leftBuffer.getSampleAgo(sliceStartSamples + (playbackLength - 1 - i));
        playbackBufferRight[i] = rightBuffer.getSampleAgo(sliceStartSamples + (playbackLength - 1 - i));
    }
    
    playbackPosition = 0;
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
        outputLeft[i] = playbackBufferLeft[static_cast<size_t>(playbackPosition + i)];
        outputRight[i] = playbackBufferRight[static_cast<size_t>(playbackPosition + i)];
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

}
