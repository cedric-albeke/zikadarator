#pragma once

namespace zikada {

class SequencerEngine
{
public:
    SequencerEngine();

    void prepare(double sampleRate, int blockSize);
    void reset();

    void setTempo(double bpm);
    void setStepResolution(int resolutionIndex);
    void setPlaying(bool playing);

    int getCurrentStep() const { return currentStep; }
    double getStepPhase() const { return samplesPerStep > 0.0 ? sampleCounter / samplesPerStep : 0.0; }
    void advance(int numSamples);

private:
    double sampleRate{44100.0};
    double tempoBPM{120.0};
    double samplesPerStep{0.0};
    double sampleCounter{0.0};
    int currentStep{0};
    bool isPlaying{false};
    int stepResolution{1};
};

}
