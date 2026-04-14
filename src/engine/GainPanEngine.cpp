#include "engine/GainPanEngine.h"
#include <cmath>
#include <algorithm>

namespace zikada {

GainPanEngine::GainPanEngine() = default;

void GainPanEngine::prepare(double /*sampleRate*/, int /*maxBlockSize*/)
{
    reset();
}

void GainPanEngine::reset()
{
    currentLeftGain  = 1.0f;
    currentRightGain = 1.0f;
    targetLeftGain   = 1.0f;
    targetRightGain  = 1.0f;
}

void GainPanEngine::setVolume(float vol)
{
    volume = std::clamp (vol, 0.0f, 2.0f);
    updateGains();
}

void GainPanEngine::setPan(float panValue)
{
    pan = std::clamp (panValue, -1.0f, 1.0f);
    updateGains();
}

void GainPanEngine::updateGains()
{
    float panLaw = 0.70710678f;
    float angle  = (pan + 1.0f) * 0.25f * 3.14159265f;
    targetLeftGain  = volume * panLaw * std::cos (angle) * 2.0f;
    targetRightGain = volume * panLaw * std::sin (angle) * 2.0f;
}

void GainPanEngine::process(float* left, float* right, int numSamples)
{
    if (left == nullptr || right == nullptr)
        return;

    float stepLeft  = (targetLeftGain  - currentLeftGain)  / static_cast<float> (numSamples);
    float stepRight = (targetRightGain - currentRightGain) / static_cast<float> (numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        currentLeftGain  += stepLeft;
        currentRightGain += stepRight;
        left[i]  *= currentLeftGain;
        right[i] *= currentRightGain;
    }

    currentLeftGain  = targetLeftGain;
    currentRightGain = targetRightGain;
}

}
