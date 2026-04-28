#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace zikada {

class RealtimeRingBuffer
{
public:
    void prepare(int newCapacity)
    {
        capacity = std::max(1, newCapacity);
        data.assign(static_cast<size_t>(capacity), 0.0f);
        writePosition = 0;
        validSamples = 0;
    }

    void reset()
    {
        std::fill(data.begin(), data.end(), 0.0f);
        writePosition = 0;
        validSamples = 0;
    }

    void write(const float* samples, int numSamples)
    {
        if (samples == nullptr || numSamples <= 0 || capacity <= 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            data[static_cast<size_t>(writePosition)] = samples[i];
            writePosition = (writePosition + 1) % capacity;
            validSamples = std::min(capacity, validSamples + 1);
        }
    }

    [[nodiscard]] float getSampleAgo(int samplesAgo) const
    {
        if (samplesAgo < 0 || samplesAgo >= validSamples || capacity <= 0)
            return 0.0f;

        int index = writePosition - 1 - samplesAgo;
        while (index < 0)
            index += capacity;

        return data[static_cast<size_t>(index % capacity)];
    }

    [[nodiscard]] float getSampleAgoLinear(float samplesAgo) const
    {
        if (samplesAgo < 0.0f)
            return getSampleAgo(0);

        const auto floorAgo = static_cast<int>(std::floor(samplesAgo));
        const auto fraction = samplesAgo - static_cast<float>(floorAgo);
        const auto a = getSampleAgo(floorAgo);
        const auto b = getSampleAgo(floorAgo + 1);
        return a + (b - a) * fraction;
    }

    [[nodiscard]] int getCapacity() const { return capacity; }
    [[nodiscard]] int getValidSamples() const { return validSamples; }

private:
    std::vector<float> data;
    int capacity{0};
    int writePosition{0};
    int validSamples{0};
};

} // namespace zikada
