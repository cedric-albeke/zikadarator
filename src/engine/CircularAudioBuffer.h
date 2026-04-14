#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace zikada {

class CircularAudioBuffer
{
public:
    CircularAudioBuffer();

    void prepare(int maxSamples);
    void reset();

    void write(const float* samples, int numSamples);
    void read(float* destination, int numSamples);

    int getNumSamplesAvailable() const { return fifo.getNumReady(); }
    float getSample(int samplesAgo) const;

private:
    juce::AbstractFifo fifo{480000};
    std::vector<float> buffer;
    std::atomic<int> writePosition{0};
};

}
