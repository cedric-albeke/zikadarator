#pragma once

#include <array>
#include "StepData.h"

namespace zikada {

class SequencerState
{
public:
    static constexpr int NumLanes = 6;
    static constexpr int NumSteps = 16;

    const StepData& getStepData (int lane, int step) const
    {
        return grid[static_cast<size_t> (lane)][static_cast<size_t> (step)];
    }

    void setStepData (int lane, int step, const StepData& data)
    {
        grid[static_cast<size_t> (lane)][static_cast<size_t> (step)] = data;
    }

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree root ("SequencerState");

        for (int lane = 0; lane < NumLanes; ++lane)
        {
            juce::ValueTree laneTree ("Lane");
            laneTree.setProperty ("index", lane, nullptr);

            for (int step = 0; step < NumSteps; ++step)
            {
                auto stepTree = grid[static_cast<size_t> (lane)][static_cast<size_t> (step)].toValueTree();
                stepTree.setProperty ("index", step, nullptr);
                laneTree.addChild (stepTree, -1, nullptr);
            }

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

            for (int si = 0; si < laneTree.getNumChildren(); ++si)
            {
                auto stepTree = laneTree.getChild (si);
                int step = static_cast<int> (stepTree.getProperty ("index", si));

                if (step < 0 || step >= NumSteps)
                    continue;

                grid[static_cast<size_t> (lane)][static_cast<size_t> (step)] =
                    StepData::fromValueTree (stepTree);
            }
        }
    }

private:
    std::array<std::array<StepData, NumSteps>, NumLanes> grid{};
};

} // namespace zikada
