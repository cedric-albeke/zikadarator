#include "engine/MotionEngine.h"
#include <cmath>
#include <algorithm>

namespace zikada {

MotionEngine::MotionEngine() = default;

void MotionEngine::prepare(double sr)
{
    sampleRate = sr;
}

void MotionEngine::trigger(int shapeIndex, double speedMultiplier, double stepDurationInSeconds)
{
    shape        = shapeIndex;
    speed        = std::clamp (speedMultiplier, 0.25, 4.0);
    stepDuration = stepDurationInSeconds;
    phase        = 0.0;

    double cyclesPerStep = speed;
    double cyclesPerSecond = cyclesPerStep / stepDuration;
    phaseDelta = cyclesPerSecond / sampleRate;
}

float MotionEngine::getNextSample()
{
    float value = computeShape (static_cast<float> (phase));
    phase += phaseDelta;
    phase -= std::floor (phase);
    return value;
}

void MotionEngine::reset()
{
    phase = 0.0;
}

float MotionEngine::computeShape(float ph) const
{
    switch (shape)
    {
        default:
        case 0: // sine, bipolar -1..1
            return std::sin (ph * 2.0f * 3.14159265f);

        case 1: // triangle, bipolar -1..1
            return 1.0f - std::abs (ph * 4.0f - 2.0f);

        case 2: // saw up, bipolar -1..1
            return ph * 2.0f - 1.0f;

        case 3: // saw down, bipolar -1..1
            return 1.0f - ph * 2.0f;

        case 4: // square, bipolar -1..1
            return ph < 0.5f ? 1.0f : -1.0f;

        case 5: // ramp up, unipolar 0..1
            return ph;

        case 6: // ramp down, unipolar 0..1
            return 1.0f - ph;

        case 7: // random step (changes each cycle)
        {
            static thread_local unsigned int seed = 0;
            seed = seed * 1664525u + 1013904223u;
            return (static_cast<float> (seed % 10001u) / 10000.0f);
        }
    }
}

}
