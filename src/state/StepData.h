#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace zikada {

struct StepData
{
    bool active = false;
    int  presetIndex = 0;

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree ("Step");
        tree.setProperty ("active",      active,      nullptr);
        tree.setProperty ("presetIndex", presetIndex, nullptr);
        return tree;
    }

    static StepData fromValueTree (const juce::ValueTree& tree)
    {
        StepData d;
        d.active      = static_cast<bool> (tree.getProperty ("active",      false));
        d.presetIndex = static_cast<int>  (tree.getProperty ("presetIndex", 0));
        return d;
    }
};

}
