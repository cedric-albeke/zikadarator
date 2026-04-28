#pragma once

#include "engine/RealtimeRingBuffer.h"

#include <vector>

namespace zikada {

class SliceEngine
{
public:
    SliceEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setTempo(double bpm);
    void triggerSlice(int sliceIndex);

    void process(float* outputLeft, float* outputRight, int numSamples);

    void writeToBuffer(const float* inputLeft, const float* inputRight, int numSamples);

private:
    double sampleRate{44100.0};
    double tempoBPM{120.0};
    double samplesPerSlice{0.0};
    int maxSliceSamples{0};

    RealtimeRingBuffer leftBuffer;
    RealtimeRingBuffer rightBuffer;

    std::vector<float> playbackBufferLeft;
    std::vector<float> playbackBufferRight;
    int playbackPosition{0};
    int playbackLength{0};
    bool isPlayingSlice{false};
};

}
