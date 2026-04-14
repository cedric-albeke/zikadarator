#pragma once

namespace zikada {

class GainPanEngine
{
public:
    GainPanEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setVolume(float vol);
    void setPan(float panValue);

    void process(float* left, float* right, int numSamples);

private:
    float volume{1.0f};
    float pan{0.0f};
    float targetLeftGain{1.0f};
    float targetRightGain{1.0f};
    float currentLeftGain{1.0f};
    float currentRightGain{1.0f};

    void updateGains();
};

}
