#pragma once

#include "MotionEngine.h"
#include "EnvFollowerEngine.h"
#include "RandomEngine.h"
#include "../state/ModulationData.h"
#include <array>

namespace zikada {

class ModulationEngine
{
public:
    ModulationEngine();

    void prepare(double sr);

    void setStepData(const ModulationData& data, double tempoBPM, double stepDurationSeconds);

    void processSample(float left, float right, float* targetValues);

    void reset();

    static constexpr int NumTargets = static_cast<int> (ModulationTarget::NumTargets);

private:
    struct SlotState
    {
        bool             active{false};
        ModulationTarget target{ModulationTarget::None};
        ModulationSource source{ModulationSource::Static};
        float            amount{0.0f};
        MotionEngine     motion;
        EnvFollowerEngine env;
        RandomEngine     random;
    };

    std::array<SlotState, ModulationData::NumSlots> slots;
    double sampleRate{44100.0};
};

}
