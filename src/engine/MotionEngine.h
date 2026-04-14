#pragma once

namespace zikada {

class MotionEngine
{
public:
    MotionEngine();

    void prepare(double sr);

    void trigger(int shapeIndex, double speedMultiplier, double stepDurationInSeconds);

    float getNextSample();

    void reset();

private:
    double sampleRate{44100.0};
    int    shape{0};
    double speed{1.0};
    double stepDuration{0.25};
    double phase{0.0};
    double phaseDelta{0.0};

    float computeShape(float ph) const;
};

}
