#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include "StepData.h"
#include "UserSlotData.h"

namespace zikada {

class SequencerState
{
public:
    static constexpr int NumLanes    = 6;
    static constexpr int NumSteps    = 16;
    static constexpr int NumUserSlots = 4;

    using StepGrid = std::array<std::array<StepData, NumSteps>, NumLanes>;
    using UserSlotGrid = std::array<std::array<UserSlotData, NumUserSlots>, NumLanes>;

    struct Snapshot
    {
        StepGrid grid{};
        UserSlotGrid userSlots{};

        const StepData& getStepData (int lane, int step) const
        {
            return grid[static_cast<size_t> (lane)][static_cast<size_t> (step)];
        }

        const UserSlotData& getUserSlot (int lane, int slot) const
        {
            return userSlots[static_cast<size_t> (lane)][static_cast<size_t> (slot)];
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
                    auto stepTree = getStepData (lane, step).toValueTree();
                    stepTree.setProperty ("index", step, nullptr);
                    stepsTree.addChild (stepTree, -1, nullptr);
                }
                laneTree.addChild (stepsTree, -1, nullptr);

                juce::ValueTree slotsTree ("UserSlots");
                for (int slot = 0; slot < NumUserSlots; ++slot)
                {
                    auto slotTree = getUserSlot (lane, slot).toValueTree();
                    slotTree.setProperty ("index", slot, nullptr);
                    slotsTree.addChild (slotTree, -1, nullptr);
                }
                laneTree.addChild (slotsTree, -1, nullptr);

                root.addChild (laneTree, -1, nullptr);
            }

            return root;
        }
    };

    SequencerState()
    {
        publishSnapshot();
    }

    StepData getStepData (int lane, int step) const
    {
        const std::lock_guard<std::mutex> lock (liveStateMutex);
        return grid[static_cast<size_t> (lane)][static_cast<size_t> (step)];
    }

    void setStepData (int lane, int step, const StepData& data)
    {
        const std::lock_guard<std::mutex> lock (liveStateMutex);
        grid[static_cast<size_t> (lane)][static_cast<size_t> (step)] = data;
        publishSnapshot();
    }

    UserSlotData getUserSlot (int lane, int slot) const
    {
        const std::lock_guard<std::mutex> lock (liveStateMutex);
        return userSlots[static_cast<size_t> (lane)][static_cast<size_t> (slot)];
    }

    void setUserSlot (int lane, int slot, const UserSlotData& data)
    {
        const std::lock_guard<std::mutex> lock (liveStateMutex);
        userSlots[static_cast<size_t> (lane)][static_cast<size_t> (slot)] = data;
        publishSnapshot();
    }

    [[nodiscard]] Snapshot getSnapshot() const
    {
        for (;;)
        {
            const int index = publishedSnapshotIndex.load (std::memory_order_acquire);
            snapshotReaders[static_cast<size_t> (index)].fetch_add (1, std::memory_order_acq_rel);

            if (publishedSnapshotIndex.load (std::memory_order_acquire) == index)
            {
                auto snapshotCopy = snapshots[static_cast<size_t> (index)];
                snapshotReaders[static_cast<size_t> (index)].fetch_sub (1, std::memory_order_release);
                return snapshotCopy;
            }

            snapshotReaders[static_cast<size_t> (index)].fetch_sub (1, std::memory_order_release);
        }
    }

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        return getSnapshot().toValueTree();
    }

    void fromValueTree (const juce::ValueTree& root)
    {
        if (! root.hasType ("SequencerState"))
            return;

        StepGrid loadedGrid{};
        UserSlotGrid loadedUserSlots{};

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
                        loadedGrid[static_cast<size_t> (lane)][static_cast<size_t> (step)] = StepData::fromValueTree (stepTree);
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
                            loadedGrid[static_cast<size_t> (lane)][static_cast<size_t> (step)] = StepData::fromValueTree (stepTree);
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
                        loadedUserSlots[static_cast<size_t> (lane)][static_cast<size_t> (slot)] = UserSlotData::fromValueTree (slotTree);
                }
            }
        }

        const std::lock_guard<std::mutex> lock (liveStateMutex);
        grid = loadedGrid;
        userSlots = loadedUserSlots;
        publishSnapshot();
    }

private:
    static constexpr int NumSnapshotBuffers = 3;

    StepGrid grid{};
    UserSlotGrid userSlots{};
    mutable std::mutex liveStateMutex;
    std::array<Snapshot, NumSnapshotBuffers> snapshots{};
    mutable std::array<std::atomic<int>, NumSnapshotBuffers> snapshotReaders{};
    std::atomic<int> publishedSnapshotIndex{0};

    void publishSnapshot()
    {
        Snapshot nextSnapshot;
        nextSnapshot.grid = grid;
        nextSnapshot.userSlots = userSlots;

        const int currentIndex = publishedSnapshotIndex.load (std::memory_order_acquire);

        for (;;)
        {
            for (int offset = 1; offset < NumSnapshotBuffers; ++offset)
            {
                const int candidate = (currentIndex + offset) % NumSnapshotBuffers;
                if (snapshotReaders[static_cast<size_t> (candidate)].load (std::memory_order_acquire) != 0)
                    continue;

                snapshots[static_cast<size_t> (candidate)] = nextSnapshot;
                publishedSnapshotIndex.store (candidate, std::memory_order_release);
                return;
            }
        }
    }
};

} // namespace zikada
