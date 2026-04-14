#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace zikada {

struct StepData
{
    bool active = false;
    int  presetIndex = 0;
    int  chainLength = 1;

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree ("Step");
        tree.setProperty ("active",      active,      nullptr);
        tree.setProperty ("presetIndex", presetIndex, nullptr);
        tree.setProperty ("chainLength", chainLength, nullptr);
        return tree;
    }

    static StepData fromValueTree (const juce::ValueTree& tree)
    {
        StepData d;
        d.active      = static_cast<bool> (tree.getProperty ("active",      false));
        d.presetIndex = static_cast<int>  (tree.getProperty ("presetIndex", 0));
        d.chainLength = static_cast<int>  (tree.getProperty ("chainLength", 1));
        return d;
    }
};

}
