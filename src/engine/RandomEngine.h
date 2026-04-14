#pragma once

namespace zikada {

class RandomEngine
{
public:
    RandomEngine();

    void prepare(double sr);

    void trigger(int rateSubdivisions, bool smoothGlide, double stepDurationInSeconds);

    float getNextSample();

    void reset();

private:
    double sampleRate{44100.0};
    int    rate{1};
    bool   smooth{false};
    double stepDuration{0.25};
    double sampleCounter{0.0};
    double subdivisionDuration{0.25};
    float  currentValue{0.0f};
    float  nextValue{0.0f};

    void pickNextValue();
};

}
