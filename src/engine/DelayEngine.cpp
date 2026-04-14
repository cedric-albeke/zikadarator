#include "engine/DelayEngine.h"

namespace zikada {

DelayEngine::DelayEngine() = default;

void DelayEngine::prepare(double sr, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);
    sampleRate = sr;

    const int maxDelaySamples = static_cast<int>(sampleRate * 2.0) + 1;
    bufferSize = maxDelaySamples;

    bufferL.assign(bufferSize, 0.0f);
    bufferR.assign(bufferSize, 0.0f);

    reset();
}

void DelayEngine::reset()
{
    std::fill(bufferL.begin(), bufferL.end(), 0.0f);
    std::fill(bufferR.begin(), bufferR.end(), 0.0f);
    writeIndexL = 0;
    writeIndexR = 0;
}

void DelayEngine::setDelayTime(float seconds)
{
    delayTimeSec = juce::jlimit(0.0f, 2.0f, seconds);
}

void DelayEngine::setFeedback(float fb)
{
    feedback = juce::jlimit(0.0f, 0.95f, fb);
}

void DelayEngine::setMix(float wetMix)
{
    mix = juce::jlimit(0.0f, 1.0f, wetMix);
}

void DelayEngine::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

int DelayEngine::getDelaySamples() const
{
    return juce::jlimit(1, bufferSize - 1, static_cast<int>(delayTimeSec * sampleRate));
}

void DelayEngine::process(float* left, float* right, int numSamples)
{
    if (!isEnabled)
        return;

    const int delaySamples = getDelaySamples();

    for (int i = 0; i < numSamples; ++i)
    {
        const int readIndexL = (writeIndexL - delaySamples + bufferSize) % bufferSize;
        const int readIndexR = (writeIndexR - delaySamples + bufferSize) % bufferSize;

        const float delayedL = bufferL[readIndexL];
        const float delayedR = bufferR[readIndexR];

        bufferL[writeIndexL] = left[i] + delayedL * feedback;
        bufferR[writeIndexR] = right[i] + delayedR * feedback;

        writeIndexL = (writeIndexL + 1) % bufferSize;
        writeIndexR = (writeIndexR + 1) % bufferSize;

        left[i]  = left[i]  * (1.0f - mix) + delayedL * mix;
        right[i] = right[i] * (1.0f - mix) + delayedR * mix;
    }
}

}
