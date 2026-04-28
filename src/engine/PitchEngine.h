#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace zikada {

// Experimental pitch-color processor. This is not a production time-stretch,
// granular, vinyl, or formant algorithm.
class PitchEngine
{
public:
    PitchEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setSemitones(float semitones);
    void setMix(float mix);
    void setEnabled(bool enabled);

    void process(float* left, float* right, int numSamples);

private:
    double sampleRate{44100.0};
    bool isEnabled{true};

    float semitones{0.0f};
    float mix{1.0f};
    float playbackRate{1.0f};

    std::vector<float> ringL;
    std::vector<float> ringR;
    int ringSize{0};
    int writePos{0};
    float readPosL{0.0f};
    float readPosR{0.0f};

    float readSampleLinear(const std::vector<float>& ring, float pos) const;
};

}
