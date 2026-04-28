#include "engine/DelayEngine.h"

namespace zikada {

DelayEngine::DelayEngine() = default;

void DelayEngine::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;
    maxDelaySamples = static_cast<float>(juce::jmax(1, static_cast<int>(sampleRate * 2.0)));

    const juce::dsp::ProcessSpec spec{sampleRate,
                                      static_cast<juce::uint32>(juce::jmax(1, maxBlockSize)),
                                      2};

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(static_cast<int>(maxDelaySamples));

    delaySamplesSmoothed.reset(sampleRate, 0.02);
    feedbackSmoothed.reset(sampleRate, 0.01);
    mixSmoothed.reset(sampleRate, 0.01);
    reset();
}

void DelayEngine::reset()
{
    delayLine.reset();
    delaySamplesSmoothed.setCurrentAndTargetValue(juce::jlimit(0.0f, maxDelaySamples,
                                                              delayTimeSec * static_cast<float>(sampleRate)));
    feedbackSmoothed.setCurrentAndTargetValue(feedback);
    mixSmoothed.setCurrentAndTargetValue(mix);
    hasProcessed = false;
}

void DelayEngine::setDelayTime(float seconds)
{
    delayTimeSec = juce::jlimit(0.0f, 2.0f, seconds);
    const float targetDelaySamples = juce::jlimit(0.0f, maxDelaySamples,
                                                  delayTimeSec * static_cast<float>(sampleRate));
    if (hasProcessed)
        delaySamplesSmoothed.setTargetValue(targetDelaySamples);
    else
        delaySamplesSmoothed.setCurrentAndTargetValue(targetDelaySamples);
}

void DelayEngine::setFeedback(float fb)
{
    feedback = juce::jlimit(0.0f, 0.95f, fb);
    if (hasProcessed)
        feedbackSmoothed.setTargetValue(feedback);
    else
        feedbackSmoothed.setCurrentAndTargetValue(feedback);
}

void DelayEngine::setMix(float wetMix)
{
    mix = juce::jlimit(0.0f, 1.0f, wetMix);
    if (hasProcessed)
        mixSmoothed.setTargetValue(mix);
    else
        mixSmoothed.setCurrentAndTargetValue(mix);
}

void DelayEngine::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

void DelayEngine::process(float* left, float* right, int numSamples)
{
    if (!isEnabled || left == nullptr || numSamples <= 0)
        return;

    const bool hasSeparateRight = right != nullptr && right != left;
    hasProcessed = true;

    for (int i = 0; i < numSamples; ++i)
    {
        const float delaySamples = delaySamplesSmoothed.getNextValue();
        const float feedbackAmount = feedbackSmoothed.getNextValue();
        const float wetMix = mixSmoothed.getNextValue();

        const float inL = left[i];
        const float delayedL = delayLine.popSample(0, delaySamples);
        delayLine.pushSample(0, inL + delayedL * feedbackAmount);
        left[i] = inL * (1.0f - wetMix) + delayedL * wetMix;

        if (hasSeparateRight)
        {
            const float inR = right[i];
            const float delayedR = delayLine.popSample(1, delaySamples);
            delayLine.pushSample(1, inR + delayedR * feedbackAmount);
            right[i] = inR * (1.0f - wetMix) + delayedR * wetMix;
        }
    }
}

}
