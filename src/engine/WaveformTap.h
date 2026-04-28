#pragma once

#include <algorithm>
#include <vector>

#include <juce_core/juce_core.h>

namespace zikada {

class WaveformTap
{
public:
    void prepare(int capacity)
    {
        const auto safeCapacity = std::max(1, capacity);
        buffer.assign(static_cast<size_t>(safeCapacity), 0.0f);
        fifo.setTotalSize(safeCapacity);
        fifo.reset();
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        fifo.reset();
    }

    void pushFromAudioThread(const float* samples, int numSamples)
    {
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

private:
    juce::AbstractFifo fifo{1};
    std::vector<float> buffer;
};

} // namespace zikada
