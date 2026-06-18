#include "engine/EnvelopeShape.h"

#include <juce_core/juce_core.h>

#include <cmath>

namespace zikada {

float computeEnvelopeShape(int presetIndex, float phase)
{
    phase = juce::jlimit(0.0f, 1.0f, phase);

    switch (presetIndex)
    {
        case 5:  return phase;
        case 6:  return 1.0f - phase;
        case 7:  return 0.75f;
        case 8:  return phase < 0.65f ? 1.0f : 0.0f;
        case 9:  return std::exp(-6.0f * phase);
        case 10: return 0.35f + 0.65f * std::sin(phase * juce::MathConstants<float>::pi);
        case 11: return phase * phase;
        case 12: return std::sin(phase * juce::MathConstants<float>::twoPi) > 0.0f ? 1.0f : 0.25f;
        case 13: return phase * phase * (3.0f - 2.0f * phase);
        case 14: return phase < 0.5f ? phase * 2.0f : (1.0f - phase) * 2.0f;
        case 15: return 0.25f + 0.75f * (std::sin(phase * juce::MathConstants<float>::twoPi * 4.0f) > 0.0f ? 1.0f : 0.0f);
        case 16: return 0.18f + 0.82f * (0.5f + 0.5f * std::sin(phase * juce::MathConstants<float>::twoPi * 2.0f));
        case 17: return 0.22f + 0.78f * std::exp(-5.5f * phase);
        case 18: return std::exp(-14.0f * phase);
        case 19: return std::sqrt(phase);
        case 20: return phase < 0.70f ? 1.0f : juce::jlimit(0.0f, 1.0f, (1.0f - phase) / 0.30f);
        default: return 1.0f;
    }
}

} // namespace zikada
