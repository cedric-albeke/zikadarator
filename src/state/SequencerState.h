#pragma once

#include <array>
#include "StepData.h"
#include "UserSlotData.h"

namespace zikada {

class SequencerState
{
public:
    static constexpr int NumLanes    = 6;
    static constexpr int NumSteps    = 16;
    static constexpr int NumUserSlots = 4;

    const StepData& getStepData (int lane, int step) const
    {
        return grid[static_cast<size_t> (lane)][static_cast<size_t> (step)];
    }

    void setStepData (int lane, int step, const StepData& data)
    {
        grid[static_cast<size_t> (lane)][static_cast<size_t> (step)] = data;
    }

    const UserSlotData& getUserSlot (int lane, int slot) const
    {
        return userSlots[static_cast<size_t> (lane)][static_cast<size_t> (slot)];
    }

    void setUserSlot (int lane, int slot, const UserSlotData& data)
    {
        userSlots[static_cast<size_t> (lane)][static_cast<size_t> (slot)] = data;
    }

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree root ("SequencerState");

        for (int lane = 0; lane < NumLanes; ++lane)
        {
            juce::ValueTree laneTree ("Lane");
            laneTree.setProperty ("index", lane, nullptr);

            juce::ValueTree stepsTree ("Steps");
            for (int step = 0; step < NumSteps; ++step)
            {
                auto stepTree = grid[static_cast<size_t> (lane)][static_cast<size_t> (step)].toValueTree();
                stepTree.setProperty ("index", step, nullptr);
                stepsTree.addChild (stepTree, -1, nullptr);
            }
            laneTree.addChild (stepsTree, -1, nullptr);

            juce::ValueTree slotsTree ("UserSlots");
            for (int slot = 0; slot < NumUserSlots; ++slot)
            {
                auto slotTree = userSlots[static_cast<size_t> (lane)][static_cast<size_t> (slot)].toValueTree();
                slotTree.setProperty ("index", slot, nullptr);
                slotsTree.addChild (slotTree, -1, nullptr);
            }
            laneTree.addChild (slotsTree, -1, nullptr);

            root.addChild (laneTree, -1, nullptr);
        }

        return root;
    }

    void fromValueTree (const juce::ValueTree& root)
    {
        if (! root.hasType ("SequencerState"))
            return;

        for (int li = 0; li < root.getNumChildren(); ++li)
        {
            auto laneTree = root.getChild (li);
            int lane = static_cast<int> (laneTree.getProperty ("index", li));

            if (lane < 0 || lane >= NumLanes)
                continue;

            auto stepsTree = laneTree.getChildWithName ("Steps");
            if (stepsTree.isValid())
            {
                for (int si = 0; si < stepsTree.getNumChildren(); ++si)
                {
                    auto stepTree = stepsTree.getChild (si);
                    int step = static_cast<int> (stepTree.getProperty ("index", si));

                    if (step >= 0 && step < NumSteps)
                        grid[static_cast<size_t> (lane)][static_cast<size_t> (step)] = StepData::fromValueTree (stepTree);
                }
            }
            else
            {
                for (int si = 0; si < laneTree.getNumChildren(); ++si)
                {
                    auto stepTree = laneTree.getChild (si);
                    if (stepTree.hasType ("Step"))
                    {
                        int step = static_cast<int> (stepTree.getProperty ("index", si));
                        if (step >= 0 && step < NumSteps)
                            grid[static_cast<size_t> (lane)][static_cast<size_t> (step)] = StepData::fromValueTree (stepTree);
                    }
                }
            }

            auto slotsTree = laneTree.getChildWithName ("UserSlots");
            if (slotsTree.isValid())
            {
                for (int si = 0; si < slotsTree.getNumChildren(); ++si)
                {
                    auto slotTree = slotsTree.getChild (si);
                    int slot = static_cast<int> (slotTree.getProperty ("index", si));

                    if (slot >= 0 && slot < NumUserSlots)
                        userSlots[static_cast<size_t> (lane)][static_cast<size_t> (slot)] = UserSlotData::fromValueTree (slotTree);
                }
            }
        }
    }

private:
    std::array<std::array<StepData, NumSteps>, NumLanes> grid{};
    std::array<std::array<UserSlotData, NumUserSlots>, NumLanes> userSlots{};
};

} // namespace zikada
