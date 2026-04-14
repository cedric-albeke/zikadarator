#pragma once

#include <juce_dsp/juce_dsp.h>

namespace zikada {

class ReverbEngine
{
public:
    ReverbEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setRoomSize(float roomSize);
    void setDamping(float damping);
    void setWidth(float width);
    void setMix(float mix);
    void setEnabled(bool enabled);

    void process(float* left, float* right, int numSamples);

private:
    bool isEnabled{true};
    float mix{0.5f};

    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
};

}
