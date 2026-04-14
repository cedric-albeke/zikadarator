#pragma once

#include <array>
#include <juce_data_structures/juce_data_structures.h>

namespace zikada {

enum class ModulationSource : int
{
    Static = 0,
    Motion,
    EnvFollower,
    Random,
    NumSources
};

enum class ModulationTarget : int
{
    None = -1,
    FilterCutoff = 0,
    FilterResonance,
    DelayTime,
    DelayFeedback,
    DelayMix,
    Volume,
    Pan,
    NumTargets
};

struct ModulationSlot
{
    ModulationTarget target = ModulationTarget::None;
    ModulationSource source = ModulationSource::Static;
    float amount = 0.0f;

    int   motionShape  = 0;
    float motionSpeed  = 1.0f;

    float envAttack    = 10.0f;
    float envRelease   = 100.0f;
    bool  envBipolar   = false;

    int  randomRate    = 1;
    bool randomSmooth  = false;

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree ("Slot");
        tree.setProperty ("target",        static_cast<int> (target),       nullptr);
        tree.setProperty ("source",        static_cast<int> (source),       nullptr);
        tree.setProperty ("amount",        amount,                          nullptr);
        tree.setProperty ("motionShape",   motionShape,                     nullptr);
        tree.setProperty ("motionSpeed",   motionSpeed,                     nullptr);
        tree.setProperty ("envAttack",     envAttack,                       nullptr);
        tree.setProperty ("envRelease",    envRelease,                      nullptr);
        tree.setProperty ("envBipolar",    envBipolar,                      nullptr);
        tree.setProperty ("randomRate",    randomRate,                      nullptr);
        tree.setProperty ("randomSmooth",  randomSmooth,                    nullptr);
        return tree;
    }

    static ModulationSlot fromValueTree (const juce::ValueTree& tree)
    {
        ModulationSlot s;
        s.target       = static_cast<ModulationTarget> (static_cast<int> (tree.getProperty ("target",       -1)));
        s.source       = static_cast<ModulationSource> (static_cast<int> (tree.getProperty ("source",        0)));
        s.amount       = static_cast<float>            (tree.getProperty ("amount",        0.0f));
        s.motionShape  = static_cast<int>              (tree.getProperty ("motionShape",   0));
        s.motionSpeed  = static_cast<float>            (tree.getProperty ("motionSpeed",   1.0f));
        s.envAttack    = static_cast<float>            (tree.getProperty ("envAttack",     10.0f));
        s.envRelease   = static_cast<float>            (tree.getProperty ("envRelease",    100.0f));
        s.envBipolar   = static_cast<bool>             (tree.getProperty ("envBipolar",    false));
        s.randomRate   = static_cast<int>              (tree.getProperty ("randomRate",    1));
        s.randomSmooth = static_cast<bool>             (tree.getProperty ("randomSmooth",  false));
        return s;
    }
};

struct ModulationData
{
    static constexpr int NumSlots = 3;
    std::array<ModulationSlot, NumSlots> slots;

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree ("Modulation");
        for (const auto& slot : slots)
            tree.addChild (slot.toValueTree(), -1, nullptr);
        return tree;
    }

    static ModulationData fromValueTree (const juce::ValueTree& tree)
    {
        ModulationData d;
        int i = 0;
        for (auto child : tree)
        {
            if (i >= NumSlots)
                break;
            d.slots[i] = ModulationSlot::fromValueTree (child);
            ++i;
        }
        return d;
    }
};

} // namespace zikada
