#pragma once

#include "engine/RealtimeRingBuffer.h"

#include <vector>
#include <cstdint>

namespace zikada {

class LoopEngine
{
public:
    static constexpr double MaximumLoopSeconds = 12.0;

    LoopEngine() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setLoopParameters(float loopLengthSeconds,
                           float playbackRate,
                           bool reversePlayback,
                           float wetMix,
                           float smoothingAmount = 0.5f);
    void setEnabled(bool enabled);
    void trigger();
    void beginProcessChunk(int numSamples);

    void captureInput(const float* left, const float* right, int numSamples);
    void process(float* left, float* right, int numSamples);

#if defined(ZIKADA_ENABLE_TEST_HOOKS)
    [[nodiscard]] int getCaptureFramesThisChunkForTesting() const { return captureFramesThisChunk; }
    [[nodiscard]] int getCaptureBudgetForTesting() const { return captureBudgetFrames; }
    [[nodiscard]] int getPendingCaptureFramesForTesting() const { return snapshotPending ? loopLengthSamples - captureProgress : 0; }
    [[nodiscard]] int getLoopLengthSamplesForTesting() const { return loopLengthSamples; }
#endif

private:
    double sampleRate{44100.0};
    RealtimeRingBuffer bufferL;
    RealtimeRingBuffer bufferR;
    int historySizeSamples{0};
    int maxLoopSamples{1};
    int loopLengthSamples{1};
    float rate{1.0f};
    float mix{1.0f};
    bool reverse{false};
    bool enabled{false};
    float phase{0.0f};
    int edgeFadeSamples{0};
    int triggerFadeSamples{0};
    int triggerFadePosition{0};
    bool snapshotPending{false};
    bool captureAnchored{false};
    bool hasSnapshot{false};
    std::int64_t captureStartAbsolute{0};
    int captureProgress{0};
    int captureBudgetFrames{2048};
    int captureFramesThisChunk{0};
    std::vector<float> loopBufferL;
    std::vector<float> loopBufferR;

    void ensureLoopBufferSize();
    void beginPendingCapture();
    void servicePendingCapture();
    float getCanonicalSample(const std::vector<float>& buffer, const RealtimeRingBuffer& history, int index) const;
    float readRawLoopSample(const std::vector<float>& buffer, const RealtimeRingBuffer& history, float phaseIndex) const;
    float readLoopSample(const std::vector<float>& buffer, const RealtimeRingBuffer& history, float phaseIndex) const;
};

} // namespace zikada
