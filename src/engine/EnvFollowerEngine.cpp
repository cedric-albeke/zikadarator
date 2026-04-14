#include "engine/EnvFollowerEngine.h"
#include <cmath>
#include <algorithm>

namespace zikada {

EnvFollowerEngine::EnvFollowerEngine() = default;

void EnvFollowerEngine::prepare(double sr)
{
    sampleRate = sr;
    envelope   = 0.0f;
}

void EnvFollowerEngine::setParameters(float attackMs, float releaseMs, bool isBipolar)
{
    attackCoeff  = coeffFromMs (attackMs);
    releaseCoeff = coeffFromMs (releaseMs);
    isBipolar    = isBipolar;
}

float EnvFollowerEngine::processSample(float left, float right)
{
    float input = std::max (std::abs (left), std::abs (right));

    if (input > envelope)
        envelope += attackCoeff * (input - envelope);
    else
        envelope += releaseCoeff * (input - envelope);

    if (isBipolar)
        return envelope * 2.0f - 1.0f;

    return envelope;
}

void EnvFollowerEngine::reset()
{
    envelope = 0.0f;
}

float EnvFollowerEngine::coeffFromMs(float ms) const
{
    ms = std::clamp (ms, 1.0f, 5000.0f);
    return 1.0f - std::exp (-1.0f / (static_cast<float> (sampleRate) * ms * 0.001f));
}

}
