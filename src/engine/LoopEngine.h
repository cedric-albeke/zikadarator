#pragma once

#include "engine/RealtimeRingBuffer.h"

#include <vector>

namespace zikada {

class LoopEngine
{
public:
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

    void captureInput(const float* left, const float* right, int numSamples);
    void process(float* left, float* right, int numSamples);

private:
    double sampleRate{44100.0};
    RealtimeRingBuffer bufferL;
    RealtimeRingBuffer bufferR;
    int historySizeSamples{0};
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
    bool hasSnapshot{false};
    std::vector<float> loopBufferL;
    std::vector<float> loopBufferR;

    void ensureLoopBufferSize();
    void refreshLoopSnapshot();
    float readRawLoopSample(const std::vector<float>& buffer, float phaseIndex) const;
    float readLoopSample(const std::vector<float>& buffer, float phaseIndex) const;
};

} // namespace zikada
