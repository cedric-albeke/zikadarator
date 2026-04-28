#include "engine/SliceEngine.h"
#include <juce_core/juce_core.h>

namespace zikada {

SliceEngine::SliceEngine() = default;

void SliceEngine::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;
    
    auto maxSeconds = 10.0;
    auto maxSamples = static_cast<int>(maxSeconds * sampleRate);
    
    leftBuffer.prepare(maxSamples);
    rightBuffer.prepare(maxSamples);
    
    playbackBufferLeft.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    playbackBufferRight.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    
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
    tempoBPM = bpm;
    auto beatDuration = 60.0 / tempoBPM;
    auto sliceDuration = beatDuration / 4.0;
    samplesPerSlice = sliceDuration * sampleRate;
    maxSliceSamples = static_cast<int>(std::ceil(samplesPerSlice));
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
    
    if (playbackBufferLeft.size() < static_cast<size_t>(playbackLength))
        playbackBufferLeft.resize(static_cast<size_t>(playbackLength));
    if (playbackBufferRight.size() < static_cast<size_t>(playbackLength))
        playbackBufferRight.resize(static_cast<size_t>(playbackLength));
    
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
