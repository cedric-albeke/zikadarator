#pragma once

#include "engine/RealtimeRingBuffer.h"

#include <vector>
#include <cstdint>

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
    void beginProcessChunk(int numSamples);

    void process(float* outputLeft, float* outputRight, int numSamples);

    void writeToBuffer(const float* inputLeft, const float* inputRight, int numSamples);

#if defined(ZIKADA_ENABLE_TEST_HOOKS)
    [[nodiscard]] int getCaptureFramesThisChunkForTesting() const { return captureFramesThisChunk; }
    [[nodiscard]] int getCaptureBudgetForTesting() const { return captureBudgetFrames; }
    [[nodiscard]] int getPendingCaptureFramesForTesting() const { return capturePending ? playbackLength - captureProgress : 0; }
#endif

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
    int edgeFadeSamples{0};
    bool isPlayingSlice{false};
    bool capturePending{false};
    std::int64_t captureStartAbsolute{0};
    int captureProgress{0};
    int captureBudgetFrames{2048};
    int captureFramesThisChunk{0};

    void servicePendingCapture();
    int getCanonicalIndex(int position) const;
    float getPlaybackSample(const std::vector<float>& buffer, const RealtimeRingBuffer& history, int position) const;
    float getEdgeFadeGain(int position) const;
};

}
