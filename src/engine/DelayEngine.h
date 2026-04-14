#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace zikada {

class DelayEngine
{
public:
    DelayEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setDelayTime(float seconds);
    void setFeedback(float feedback);
    void setMix(float mix);
    void setEnabled(bool enabled);

    void process(float* left, float* right, int numSamples);

private:
    double sampleRate{44100.0};
    bool isEnabled{true};

    float delayTimeSec{0.5f};
    float feedback{0.4f};
    float mix{0.5f};

    std::vector<float> bufferL;
    std::vector<float> bufferR;
    int bufferSize{0};
    int writeIndexL{0};
    int writeIndexR{0};

    int getDelaySamples() const;
};

}
