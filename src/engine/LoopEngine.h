#pragma once

#include "engine/RealtimeRingBuffer.h"

namespace zikada {

class LoopEngine
{
public:
    LoopEngine() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setLoopParameters(float loopLengthSeconds, float playbackRate, bool reversePlayback, float wetMix);
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

    float readLoopSample(const RealtimeRingBuffer& buffer, float phaseIndex) const;
};

} // namespace zikada
