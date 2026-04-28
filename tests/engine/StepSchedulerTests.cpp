#include "engine/StepScheduler.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

} // namespace

namespace zikada::tests {

void addStepSchedulerTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"StepScheduler splits blocks at host step boundaries", []
    {
        StepScheduler scheduler;
        const auto segments = scheduler.makeHostSegments({
            48000.0,
            120.0,
            0.5,
            0.499,
            128,
            true
        });

        require(segments.size() == 2, "block crossing a step boundary should produce two segments");
        require(segments[0].startSample == 0, "first segment starts at sample 0");
        require(segments[0].numSamples > 0, "first segment has samples");
        require(segments[0].stepIndex == 0, "first segment belongs to step 0");
        require(segments[1].startSample == segments[0].numSamples, "second segment follows first");
        require(segments[1].stepIndex == 1, "second segment belongs to step 1");
    }});

    tests.push_back({"StepScheduler returns one stopped segment when host is stopped", []
    {
        StepScheduler scheduler;
        const auto segments = scheduler.makeHostSegments({
            48000.0,
            120.0,
            0.5,
            12.0,
            256,
            false
        });

        require(segments.size() == 1, "stopped host should produce one segment");
        require(!segments[0].playing, "segment should be marked stopped");
        require(segments[0].numSamples == 256, "segment should preserve full block length");
    }});
}

} // namespace zikada::tests
