#pragma once

#include <algorithm>

namespace zikada {

inline float blendLaneMixSample(float laneInput, float laneOutput, float mix) noexcept
{
    const float wet = std::clamp(mix, 0.0f, 1.0f);
    return laneInput + (laneOutput - laneInput) * wet;
}

inline void applyLaneMix(float* left,
                         float* right,
                         const float* laneInputLeft,
                         const float* laneInputRight,
                         int numSamples,
                         float mix) noexcept
{
    const float wet = std::clamp(mix, 0.0f, 1.0f);
    if (wet >= 0.999f)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = blendLaneMixSample(laneInputLeft[i], left[i], wet);
        right[i] = blendLaneMixSample(laneInputRight[i], right[i], wet);
    }
}

} // namespace zikada
