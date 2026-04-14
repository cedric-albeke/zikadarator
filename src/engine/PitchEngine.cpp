#include "engine/PitchEngine.h"
#include <cmath>

namespace zikada {

PitchEngine::PitchEngine() = default;

void PitchEngine::prepare(double sr, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);
    sampleRate = sr;

    ringSize = static_cast<int>(sampleRate * 0.1);
    ringL.assign(ringSize, 0.0f);
    ringR.assign(ringSize, 0.0f);

    reset();
}

void PitchEngine::reset()
{
    std::fill(ringL.begin(), ringL.end(), 0.0f);
    std::fill(ringR.begin(), ringR.end(), 0.0f);
    writePos = 0;
    readPosL = 0.0f;
    readPosR = 0.0f;
}

void PitchEngine::setSemitones(float st)
{
    semitones    = juce::jlimit(-24.0f, 24.0f, st);
    playbackRate = std::pow(2.0f, semitones / 12.0f);
}

void PitchEngine::setMix(float wetMix)
{
    mix = juce::jlimit(0.0f, 1.0f, wetMix);
}

void PitchEngine::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

float PitchEngine::readSampleLinear(const std::vector<float>& ring, float pos) const
{
    const int i0 = static_cast<int>(pos) % ringSize;
    const int i1 = (i0 + 1) % ringSize;
    const float frac = pos - std::floor(pos);
    return ring[i0] * (1.0f - frac) + ring[i1] * frac;
}

void PitchEngine::process(float* left, float* right, int numSamples)
{
    if (!isEnabled)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        ringL[writePos] = left[i];
        ringR[writePos] = right[i];

        const float shiftedL = readSampleLinear(ringL, readPosL);
        const float shiftedR = readSampleLinear(ringR, readPosR);

        left[i]  = left[i]  * (1.0f - mix) + shiftedL * mix;
        right[i] = right[i] * (1.0f - mix) + shiftedR * mix;

        writePos = (writePos + 1) % ringSize;
        readPosL = std::fmod(readPosL + playbackRate, static_cast<float>(ringSize));
        readPosR = std::fmod(readPosR + playbackRate, static_cast<float>(ringSize));
    }
}

}
