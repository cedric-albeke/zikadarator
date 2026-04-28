#include "engine/RealtimeRingBuffer.h"

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void requireNear(float actual, float expected, float tolerance, const char* message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(message);
}

} // namespace

namespace zikada::tests {

void addRealtimeRingBufferTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"RealtimeRingBuffer overwrites oldest samples", []
    {
        RealtimeRingBuffer buffer;
        buffer.prepare(4);

        const float first[] = {1.0f, 2.0f, 3.0f, 4.0f};
        const float second[] = {5.0f, 6.0f};
        buffer.write(first, 4);
        buffer.write(second, 2);

        requireNear(buffer.getSampleAgo(0), 6.0f, 0.0001f, "latest sample should be 6");
        requireNear(buffer.getSampleAgo(1), 5.0f, 0.0001f, "one sample ago should be 5");
        requireNear(buffer.getSampleAgo(2), 4.0f, 0.0001f, "two samples ago should be 4");
        requireNear(buffer.getSampleAgo(3), 3.0f, 0.0001f, "three samples ago should be 3");
        requireNear(buffer.getSampleAgo(4), 0.0f, 0.0001f, "out-of-history reads should return silence");
    }});

    tests.push_back({"RealtimeRingBuffer interpolates fractional history", []
    {
        RealtimeRingBuffer buffer;
        buffer.prepare(8);

        const float samples[] = {0.0f, 10.0f, 20.0f, 30.0f};
        buffer.write(samples, 4);

        requireNear(buffer.getSampleAgoLinear(0.5f), 25.0f, 0.0001f, "0.5 sample ago should interpolate");
        requireNear(buffer.getSampleAgoLinear(1.25f), 17.5f, 0.0001f, "1.25 samples ago should interpolate");
    }});
}

} // namespace zikada::tests
