#include "engine/ModulationEngine.h"
#include <cmath>

namespace zikada {

ModulationEngine::ModulationEngine() = default;

void ModulationEngine::prepare(double sr)
{
    sampleRate = sr;
    for (auto& slot : slots)
    {
        slot.motion.prepare (sr);
        slot.env.prepare (sr);
        slot.random.prepare (sr);
    }
}

void ModulationEngine::setStepData(const ModulationData& data, double tempoBPM, double stepDurationSeconds)
{
    juce::ignoreUnused (tempoBPM);

    for (int i = 0; i < ModulationData::NumSlots; ++i)
    {
        auto& slot = slots[static_cast<size_t> (i)];
        const auto& src = data.slots[static_cast<size_t> (i)];

        slot.target  = src.target;
        slot.source  = src.source;
        slot.amount  = juce::jlimit (0.0f, 1.0f, src.amount);
        slot.active  = (slot.target != ModulationTarget::None && slot.source != ModulationSource::Static && slot.amount > 0.0f);

        if (slot.active)
        {
            slot.motion.trigger (src.motionShape, src.motionSpeed, stepDurationSeconds);
            slot.env.setParameters (src.envAttack, src.envRelease, src.envBipolar);
            slot.random.trigger (src.randomRate, src.randomSmooth, stepDurationSeconds);
        }
    }
}

void ModulationEngine::processSample(float left, float right, float* targetValues)
{
    for (int t = 0; t < NumTargets; ++t)
        targetValues[t] = 0.0f;

    for (auto& slot : slots)
    {
        if (!slot.active)
            continue;

        int targetIdx = static_cast<int> (slot.target);
        if (targetIdx < 0 || targetIdx >= NumTargets)
            continue;

        float sourceValue = 0.0f;
        switch (slot.source)
        {
            case ModulationSource::Motion:
                sourceValue = slot.motion.getNextSample();
                break;
            case ModulationSource::EnvFollower:
                sourceValue = slot.env.processSample (left, right);
                break;
            case ModulationSource::Random:
                sourceValue = slot.random.getNextSample();
                break;
            default:
                break;
        }

        targetValues[targetIdx] += sourceValue * slot.amount;
    }
}

void ModulationEngine::reset()
{
    for (auto& slot : slots)
    {
        slot.motion.reset();
        slot.env.reset();
        slot.random.reset();
    }
}

}
