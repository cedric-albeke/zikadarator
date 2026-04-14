#include "engine/CircularAudioBuffer.h"

namespace zikada {

CircularAudioBuffer::CircularAudioBuffer() = default;

void CircularAudioBuffer::prepare(int maxSamples)
{
    buffer.resize(static_cast<size_t>(maxSamples), 0.0f);
    fifo.setTotalSize(maxSamples);
    reset();
}

void CircularAudioBuffer::reset()
{
    fifo.reset();
    writePosition = 0;
    std::fill(buffer.begin(), buffer.end(), 0.0f);
}

void CircularAudioBuffer::write(const float* samples, int numSamples)
{
    auto writeHandle = fifo.write(numSamples);

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

    writePosition = (writeHandle.startIndex1 + writeHandle.blockSize1 + writeHandle.blockSize2) % fifo.getTotalSize();
}

void CircularAudioBuffer::read(float* destination, int numSamples)
{
    auto readHandle = fifo.read(numSamples);

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
}

float CircularAudioBuffer::getSample(int samplesAgo) const
{
    auto available = fifo.getNumReady();
    if (samplesAgo >= available)
        return 0.0f;

    auto totalSize = fifo.getTotalSize();
    auto targetIndex = writePosition.load() - 1 - samplesAgo;
    targetIndex = ((targetIndex % totalSize) + totalSize) % totalSize;

    return buffer[static_cast<size_t>(targetIndex)];
}

}
