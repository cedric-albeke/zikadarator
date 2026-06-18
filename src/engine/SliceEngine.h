#pragma once

#include "engine/RealtimeRingBuffer.h"

#include <vector>

namespace zikada {

class SliceEngine
{
public:
    enum class PlaybackMode
    {
        Forward = 0,
        Reverse,
        Repeat,
        Stutter
    };

    SliceEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setTempo(double bpm);
    void setPlaybackMode(PlaybackMode mode, int repeatCount);
    void triggerSlice(int sliceIndex);

    void process(float* outputLeft, float* outputRight, int numSamples);

    void writeToBuffer(const float* inputLeft, const float* inputRight, int numSamples);

private:
    double sampleRate{44100.0};
    double tempoBPM{120.0};
    double samplesPerSlice{0.0};
    int maxSliceSamples{0};
    PlaybackMode playbackMode{PlaybackMode::Forward};
    int playbackRepeatCount{1};

    RealtimeRingBuffer leftBuffer;
    RealtimeRingBuffer rightBuffer;

    std::vector<float> playbackBufferLeft;
    std::vector<float> playbackBufferRight;
    int playbackPosition{0};
    int playbackLength{0};
    bool isPlayingSlice{false};

    int getPlaybackSampleAge(int sliceStartSamples, int position) const;
};

}
