#include "state/SequencerState.h"

#include <atomic>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <thread>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(float actual, float expected, float tolerance, const char* message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(message);
}

} // namespace

namespace zikada::tests {

void addSequencerStateTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"SequencerState load replaces stale data", []
    {
        SequencerState state;

        StepData staleStep;
        staleStep.active = true;
        staleStep.presetIndex = 11;
        staleStep.chainLength = 4;
        state.setStepData(2, 3, staleStep);

        UserSlotData staleSlot;
        staleSlot.filterCutoff = 123.0f;
        state.setUserSlot(2, 1, staleSlot);

        juce::ValueTree root("SequencerState");
        juce::ValueTree lane("Lane");
        lane.setProperty("index", 1, nullptr);

        juce::ValueTree steps("Steps");
        StepData loadedStep;
        loadedStep.active = true;
        loadedStep.presetIndex = 5;
        auto stepTree = loadedStep.toValueTree();
        stepTree.setProperty("index", 4, nullptr);
        steps.addChild(stepTree, -1, nullptr);
        lane.addChild(steps, -1, nullptr);

        juce::ValueTree slots("UserSlots");
        UserSlotData loadedSlot;
        loadedSlot.filterCutoff = 4567.0f;
        auto slotTree = loadedSlot.toValueTree();
        slotTree.setProperty("index", 0, nullptr);
        slots.addChild(slotTree, -1, nullptr);
        lane.addChild(slots, -1, nullptr);

        root.addChild(lane, -1, nullptr);
        state.fromValueTree(root);

        const auto& loaded = state.getStepData(1, 4);
        require(loaded.active, "loaded step remains active");
        require(loaded.presetIndex == 5, "loaded step preset is applied");

        const auto& cleared = state.getStepData(2, 3);
        require(!cleared.active, "stale step active flag is cleared");
        require(cleared.presetIndex == 0, "stale step preset is cleared");
        require(cleared.chainLength == 1, "stale step chain length is reset");

        requireNear(state.getUserSlot(1, 0).filterCutoff, 4567.0f, 0.0001f, "loaded user slot is applied");
        requireNear(state.getUserSlot(2, 1).filterCutoff, 2000.0f, 0.0001f, "stale user slot is reset");
    }});

    tests.push_back({"SequencerState snapshot is immutable after live changes", []
    {
        SequencerState state;

        StepData initialStep;
        initialStep.active = true;
        initialStep.presetIndex = 9;
        state.setStepData(1, 2, initialStep);

        UserSlotData initialSlot;
        initialSlot.volume = 0.25f;
        state.setUserSlot(1, 0, initialSlot);

        const auto snapshot = state.getSnapshot();

        StepData changedStep;
        changedStep.active = false;
        changedStep.presetIndex = 0;
        state.setStepData(1, 2, changedStep);

        UserSlotData changedSlot;
        changedSlot.volume = 0.9f;
        state.setUserSlot(1, 0, changedSlot);

        require(snapshot.getStepData(1, 2).active, "snapshot keeps original step active flag");
        require(snapshot.getStepData(1, 2).presetIndex == 9, "snapshot keeps original step preset");
        requireNear(snapshot.getUserSlot(1, 0).volume, 0.25f, 0.0001f, "snapshot keeps original user slot");
        requireNear(state.getUserSlot(1, 0).volume, 0.9f, 0.0001f, "live state still mutates");
    }});

    tests.push_back({"SequencerState snapshot is plain value for realtime use", []
    {
        static_assert(std::is_same_v<decltype(std::declval<const SequencerState&>().getSnapshot()),
                                     SequencerState::Snapshot>,
                      "SequencerState::getSnapshot must return a plain value, not shared ownership");
    }});

    tests.push_back({"SequencerState serializes concurrent publishers", []
    {
        SequencerState state;
        std::atomic<bool> start{false};
        std::atomic<bool> writersDone{false};
        std::atomic<bool> coherent{true};

        StepData initialStep;
        initialStep.chainLength = 1;
        state.setStepData(0, 0, initialStep);
        UserSlotData initialSlot;
        initialSlot.filterCutoff = 0.0f;
        initialSlot.delayTime = 1.0f;
        initialSlot.volume = 2.0f;
        state.setUserSlot(0, 0, initialSlot);

        auto stepWriter = std::thread([&]
        {
            while (!start.load(std::memory_order_acquire)) {}

            for (int id = 1; id <= 20000; ++id)
            {
                StepData step;
                step.active = (id & 1) != 0;
                step.presetIndex = id;
                step.chainLength = id + 1;
                state.setStepData(0, 0, step);
            }
        });

        auto slotWriter = std::thread([&]
        {
            while (!start.load(std::memory_order_acquire)) {}

            for (int id = 1; id <= 20000; ++id)
            {
                UserSlotData slot;
                slot.filterCutoff = static_cast<float>(id);
                slot.delayTime = static_cast<float>(id + 1);
                slot.volume = static_cast<float>(id + 2);
                state.setUserSlot(0, 0, slot);
            }
        });

        auto reader = std::thread([&]
        {
            while (!start.load(std::memory_order_acquire)) {}

            while (!writersDone.load(std::memory_order_acquire))
            {
                const auto snapshot = state.getSnapshot();
                const auto& step = snapshot.getStepData(0, 0);
                const auto& slot = snapshot.getUserSlot(0, 0);

                if (step.chainLength != step.presetIndex + 1
                    || step.active != ((step.presetIndex & 1) != 0)
                    || slot.delayTime != slot.filterCutoff + 1.0f
                    || slot.volume != slot.filterCutoff + 2.0f)
                {
                    coherent.store(false, std::memory_order_release);
                    return;
                }
            }
        });

        start.store(true, std::memory_order_release);
        stepWriter.join();
        slotWriter.join();
        writersDone.store(true, std::memory_order_release);
        reader.join();

        require(coherent.load(std::memory_order_acquire),
                "concurrent publishers must expose field-coherent snapshots");

        static_assert(std::is_same_v<decltype(std::declval<const SequencerState&>().getStepData(0, 0)),
                                     StepData>,
                      "live step reads must return a synchronized value");
        static_assert(std::is_same_v<decltype(std::declval<const SequencerState&>().getUserSlot(0, 0)),
                                     UserSlotData>,
                      "live user-slot reads must return a synchronized value");
    }});
}

} // namespace zikada::tests
