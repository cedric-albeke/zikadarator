#pragma once

#include "state/UserSlotData.h"

#include <juce_core/juce_core.h>

namespace zikada {

struct FilterPresetTuning
{
    float cutoff{2000.0f};
    float resonance{0.707f};
};

inline FilterPresetTuning getFilterPresetTuning(int presetIndex, const UserSlotData& slot)
{
    float cutoffScale = 1.0f;
    float resonanceScale = 1.0f;

    switch (presetIndex)
    {
        case 12: cutoffScale = 0.55f; resonanceScale = 1.40f; break;
        case 13: cutoffScale = 1.50f; resonanceScale = 0.70f; break;
        case 14: cutoffScale = 0.65f; resonanceScale = 0.80f; break;
        case 15: cutoffScale = 1.25f; resonanceScale = 1.60f; break;
        case 16: cutoffScale = 0.80f; resonanceScale = 2.00f; break;
        case 17: cutoffScale = 1.80f; resonanceScale = 1.80f; break;
        case 18: cutoffScale = 0.45f; resonanceScale = 1.20f; break;
        case 19: cutoffScale = 0.65f; resonanceScale = 2.20f; break;
        case 20: cutoffScale = 0.70f; resonanceScale = 0.65f; break;
        default: break;
    }

    return {
        juce::jlimit(20.0f, 20000.0f, slot.filterCutoff * cutoffScale),
        juce::jlimit(0.1f, 10.0f, slot.filterResonance * resonanceScale)
    };
}

} // namespace zikada
