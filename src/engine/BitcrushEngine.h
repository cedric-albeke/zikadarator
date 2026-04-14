#pragma once

#include <juce_dsp/juce_dsp.h>

namespace zikada {

class BitcrushEngine
{
public:
    BitcrushEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setBitDepth(float bits);
    void setSampleRateReduction(float reducedSampleRate);
    void setEnabled(bool enabled);

    void process(float* left, float* right, int numSamples);

private:
    double sampleRate{44100.0};
    bool isEnabled{true};

    float bitDepth{16.0f};
    float reducedSampleRate{44100.0f};

    float holdL{0.0f};
    float holdR{0.0f};
    int holdCounter{0};
};

}
