#include "engine/RandomEngine.h"
#include <cmath>
#include <algorithm>

namespace zikada {

RandomEngine::RandomEngine() = default;

void RandomEngine::prepare(double sr)
{
    sampleRate = sr;
}

void RandomEngine::trigger(int rateSubdivisions, bool smoothGlide, double stepDurationInSeconds)
{
    rate = std::clamp (rateSubdivisions, 1, 16);
    smooth = smoothGlide;
    stepDuration = stepDurationInSeconds;
    subdivisionDuration = stepDuration / static_cast<double> (rate);
    sampleCounter = 0.0;
    pickNextValue();
    currentValue = nextValue;
    pickNextValue();
}

float RandomEngine::getNextSample()
{
    if (smooth)
    {
        float alpha = static_cast<float> (sampleCounter / subdivisionDuration);
        alpha = std::clamp (alpha, 0.0f, 1.0f);
        float value = currentValue + (nextValue - currentValue) * alpha;

        sampleCounter += 1.0 / sampleRate;
        if (sampleCounter >= subdivisionDuration)
        {
            sampleCounter -= subdivisionDuration;
            currentValue = nextValue;
            pickNextValue();
        }
        return value;
    }
    else
    {
        sampleCounter += 1.0 / sampleRate;
        if (sampleCounter >= subdivisionDuration)
        {
            sampleCounter -= subdivisionDuration;
            currentValue = nextValue;
            pickNextValue();
        }
        return currentValue;
    }
}

void RandomEngine::reset()
{
    sampleCounter = 0.0;
    currentValue  = 0.0f;
    nextValue     = 0.0f;
}

void RandomEngine::pickNextValue()
{
    static thread_local unsigned int seed = 0;
    seed = seed * 1664525u + 1013904223u;
    nextValue = static_cast<float> (seed % 10001u) / 10000.0f;
}

}
