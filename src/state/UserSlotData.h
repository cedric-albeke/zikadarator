#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "ModulationData.h"

namespace zikada {

struct UserSlotData
{
    float filterCutoff    = 2000.0f;
    float filterResonance = 0.707f;
    float delayTime       = 0.25f;
    float delayFeedback   = 0.3f;
    float delayMix        = 0.5f;
    float volume          = 1.0f;
    float pan             = 0.0f;

    ModulationData modulation;

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree ("UserSlot");
        tree.setProperty ("filterCutoff",    filterCutoff,    nullptr);
        tree.setProperty ("filterResonance", filterResonance, nullptr);
        tree.setProperty ("delayTime",       delayTime,       nullptr);
        tree.setProperty ("delayFeedback",   delayFeedback,   nullptr);
        tree.setProperty ("delayMix",        delayMix,        nullptr);
        tree.setProperty ("volume",          volume,          nullptr);
        tree.setProperty ("pan",             pan,             nullptr);
        tree.addChild (modulation.toValueTree(), -1, nullptr);
        return tree;
    }

    static UserSlotData fromValueTree (const juce::ValueTree& tree)
    {
        UserSlotData d;
        d.filterCutoff    = static_cast<float> (tree.getProperty ("filterCutoff",    2000.0f));
        d.filterResonance = static_cast<float> (tree.getProperty ("filterResonance", 0.707f));
        d.delayTime       = static_cast<float> (tree.getProperty ("delayTime",       0.25f));
        d.delayFeedback   = static_cast<float> (tree.getProperty ("delayFeedback",   0.3f));
        d.delayMix        = static_cast<float> (tree.getProperty ("delayMix",        0.5f));
        d.volume          = static_cast<float> (tree.getProperty ("volume",          1.0f));
        d.pan             = static_cast<float> (tree.getProperty ("pan",             0.0f));

        auto modChild = tree.getChildWithName ("Modulation");
        if (modChild.isValid())
            d.modulation = ModulationData::fromValueTree (modChild);

        return d;
    }
};

} // namespace zikada
