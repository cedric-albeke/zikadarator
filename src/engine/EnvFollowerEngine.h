#pragma once

namespace zikada {

class EnvFollowerEngine
{
public:
    EnvFollowerEngine();

    void prepare(double sr);

    void setParameters(float attackMs, float releaseMs, bool isBipolar);

    float processSample(float left, float right);

    void reset();

private:
    double sampleRate{44100.0};
    float  attackCoeff{0.0f};
    float  releaseCoeff{0.0f};
    bool   isBipolar{false};
    float  envelope{0.0f};

    float coeffFromMs(float ms) const;
};

}
