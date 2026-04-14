#include "engine/ReverbEngine.h"

namespace zikada {

ReverbEngine::ReverbEngine()
{
    reverbParams.roomSize   = 0.5f;
    reverbParams.damping    = 0.5f;
    reverbParams.wetLevel   = 0.33f;
    reverbParams.dryLevel   = 0.4f;
    reverbParams.width      = 1.0f;
    reverbParams.freezeMode = 0.0f;
}

void ReverbEngine::prepare(double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);
    reverb.setSampleRate(sampleRate);
    reverb.setParameters(reverbParams);
    reset();
}

void ReverbEngine::reset()
{
    reverb.reset();
}

void ReverbEngine::setRoomSize(float roomSize)
{
    reverbParams.roomSize = juce::jlimit(0.0f, 1.0f, roomSize);
    reverb.setParameters(reverbParams);
}

void ReverbEngine::setDamping(float damping)
{
    reverbParams.damping = juce::jlimit(0.0f, 1.0f, damping);
    reverb.setParameters(reverbParams);
}

void ReverbEngine::setWidth(float width)
{
    reverbParams.width = juce::jlimit(0.0f, 1.0f, width);
    reverb.setParameters(reverbParams);
}

void ReverbEngine::setMix(float wetMix)
{
    mix = juce::jlimit(0.0f, 1.0f, wetMix);
    reverbParams.wetLevel = mix;
    reverbParams.dryLevel = 1.0f - mix;
    reverb.setParameters(reverbParams);
}

void ReverbEngine::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

void ReverbEngine::process(float* left, float* right, int numSamples)
{
    if (!isEnabled)
        return;

    reverb.processStereo(left, right, numSamples);
}

}
