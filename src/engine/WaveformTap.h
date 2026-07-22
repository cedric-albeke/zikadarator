#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

#include <juce_core/juce_core.h>

namespace zikada {

class WaveformTap
{
public:
    void prepare(int capacity)
    {
        {
            ReconfigurationGuard reconfiguration(*this);
            const auto safeCapacity = std::max(1, capacity);
            buffer.assign(static_cast<size_t>(safeCapacity), 0.0f);
            fifo.setTotalSize(safeCapacity);
            fifo.reset();
            generation.fetch_add(1, std::memory_order_release);
        }
    }

    void reset()
    {
        {
            ReconfigurationGuard reconfiguration(*this);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            fifo.reset();
            generation.fetch_add(1, std::memory_order_release);
        }
    }

    void pushFromAudioThread(const float* samples, int numSamples)
    {
        OperationGuard operation(*this);
        if (!operation)
            return;

        if (samples == nullptr || numSamples <= 0 || buffer.empty())
            return;

        const auto writable = std::min(numSamples, fifo.getFreeSpace());
        if (writable <= 0)
            return;

        const auto writeHandle = fifo.write(writable);

        if (writeHandle.blockSize1 > 0)
        {
            std::copy(samples,
                      samples + writeHandle.blockSize1,
                      buffer.begin() + writeHandle.startIndex1);
        }

        if (writeHandle.blockSize2 > 0)
        {
            std::copy(samples + writeHandle.blockSize1,
                      samples + writeHandle.blockSize1 + writeHandle.blockSize2,
                      buffer.begin() + writeHandle.startIndex2);
        }
    }

    int popForUi(float* destination, int maxSamples)
    {
        OperationGuard operation(*this);
        if (!operation)
            return 0;

        if (destination == nullptr || maxSamples <= 0 || buffer.empty())
            return 0;

        const auto readable = std::min(maxSamples, fifo.getNumReady());
        if (readable <= 0)
            return 0;

        const auto readHandle = fifo.read(readable);

        if (readHandle.blockSize1 > 0)
        {
            std::copy(buffer.begin() + readHandle.startIndex1,
                      buffer.begin() + readHandle.startIndex1 + readHandle.blockSize1,
                      destination);
        }

        if (readHandle.blockSize2 > 0)
        {
            std::copy(buffer.begin() + readHandle.startIndex2,
                      buffer.begin() + readHandle.startIndex2 + readHandle.blockSize2,
                      destination + readHandle.blockSize1);
        }

        return readHandle.blockSize1 + readHandle.blockSize2;
    }

    int discardAllForUi()
    {
        OperationGuard operation(*this);
        if (!operation)
            return 0;

        if (buffer.empty())
            return 0;

        const auto readable = fifo.getNumReady();
        if (readable <= 0)
            return 0;

        const auto readHandle = fifo.read(readable);
        return readHandle.blockSize1 + readHandle.blockSize2;
    }

    [[nodiscard]] std::uint64_t getGeneration() const noexcept
    {
        return generation.load(std::memory_order_acquire);
    }

private:
    static_assert(std::atomic<int>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);

    class ReconfigurationGuard
    {
    public:
        explicit ReconfigurationGuard(WaveformTap& ownerIn)
            : owner(ownerIn)
        {
            owner.beginReconfiguration();
        }

        ~ReconfigurationGuard()
        {
            owner.endReconfiguration();
        }

        ReconfigurationGuard(const ReconfigurationGuard&) = delete;
        ReconfigurationGuard& operator=(const ReconfigurationGuard&) = delete;

    private:
        WaveformTap& owner;
    };

    class OperationGuard
    {
    public:
        explicit OperationGuard(WaveformTap& ownerIn) noexcept
            : owner(ownerIn), active(owner.tryBeginOperation())
        {
        }

        ~OperationGuard()
        {
            if (active)
                owner.endOperation();
        }

        explicit operator bool() const noexcept { return active; }

        OperationGuard(const OperationGuard&) = delete;
        OperationGuard& operator=(const OperationGuard&) = delete;

    private:
        WaveformTap& owner;
        bool active;
    };

    bool tryBeginOperation() noexcept
    {
        if (reconfiguring.load(std::memory_order_acquire))
            return false;

        activeOperations.fetch_add(1, std::memory_order_acq_rel);
        if (!reconfiguring.load(std::memory_order_acquire))
            return true;

        activeOperations.fetch_sub(1, std::memory_order_release);
        return false;
    }

    void endOperation() noexcept
    {
        activeOperations.fetch_sub(1, std::memory_order_release);
    }

    void beginReconfiguration()
    {
        while (reconfiguring.exchange(true, std::memory_order_acq_rel))
            std::this_thread::yield();

        while (activeOperations.load(std::memory_order_acquire) != 0)
            std::this_thread::yield();
    }

    void endReconfiguration() noexcept
    {
        reconfiguring.store(false, std::memory_order_release);
    }

    juce::AbstractFifo fifo{1};
    std::vector<float> buffer;
    std::atomic<int> activeOperations{0};
    std::atomic<bool> reconfiguring{false};
    std::atomic<std::uint64_t> generation{0};
};

} // namespace zikada
