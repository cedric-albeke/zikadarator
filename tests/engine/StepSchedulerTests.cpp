#include "engine/StepScheduler.h"

#include <functional>
#include <limits>
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
        require(segments[0].absoluteStepIndex == 0, "first segment keeps absolute step 0");
        require(segments[1].startSample == segments[0].numSamples, "second segment follows first");
        require(segments[1].stepIndex == 1, "second segment belongs to step 1");
        require(segments[1].absoluteStepIndex == 1, "second segment keeps absolute step 1");
    }});

    tests.push_back({"StepScheduler supports sixteenth-note step boundaries", []
    {
        StepScheduler scheduler;
        const auto segments = scheduler.makeHostSegments({
            48000.0,
            120.0,
            0.25,
            0.249,
            128,
            true
        });

        require(segments.size() == 2, "1/16 block crossing a step boundary should produce two segments");
        require(segments[0].stepIndex == 0, "first 1/16 segment belongs to step 0");
        require(segments[1].stepIndex == 1, "second 1/16 segment belongs to step 1");
        require(segments[0].numSamples < 40, "1/16 boundary should be close to the block start");
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

    tests.push_back({"StepScheduler preserves negative absolute steps", []
    {
        StepScheduler scheduler;
        const auto segments = scheduler.makeHostSegments({
            48000.0,
            120.0,
            0.5,
            -0.25,
            64,
            true
        });

        require(segments.size() == 1, "negative PPQ block should stay inside one step");
        require(segments[0].stepIndex == 15, "negative absolute step should wrap to sequencer step 15");
        require(segments[0].absoluteStepIndex == -1, "negative PPQ should retain absolute step -1");
    }});

    tests.push_back({"StepScheduler distinguishes complete pattern cycles", []
    {
        StepScheduler scheduler;
        const auto firstCycle = scheduler.makeHostSegments({48000.0, 120.0, 0.5, 0.0, 64, true});
        const auto secondCycle = scheduler.makeHostSegments({48000.0, 120.0, 0.5, 8.0, 64, true});

        require(firstCycle.size() == 1 && secondCycle.size() == 1,
                "pattern-cycle probes should each produce one segment");
        require(firstCycle[0].stepIndex == 0 && secondCycle[0].stepIndex == 0,
                "complete pattern cycle should keep modulo step zero");
        require(firstCycle[0].absoluteStepIndex == 0 && secondCycle[0].absoluteStepIndex == 16,
                "complete pattern cycle must retain distinct absolute steps");
    }});

    tests.push_back({"StepScheduler batches more than 64 boundaries without collapsing steps", []
    {
        StepScheduler scheduler;
        const auto segments = scheduler.makeHostSegments({64.0, 300.0, 0.25, 0.0, 4097, true});

        require(segments.size() > 64, "pathological offline block should exercise multiple scheduler batches");
        int coveredSamples = 0;
        for (const auto& segment : segments)
        {
            require(segment.startSample == coveredSamples, "scheduler batches must remain contiguous");
            require(segment.numSamples > 0, "scheduler batch emitted an empty segment");
            coveredSamples += segment.numSamples;
        }

        require(coveredSamples == 4097, "scheduler batches must cover the complete offline block");
        require(segments.back().absoluteStepIndex == 1280,
                "scheduler collapsed the final absolute step after its fixed-stack batch");
    }});

    tests.push_back({"StepScheduler bounds extreme finite host positions", []
    {
        StepScheduler scheduler;
        const auto segments = scheduler.makeHostSegments({
            48000.0,
            120.0,
            0.5,
            std::numeric_limits<double>::max(),
            64,
            true
        });

        require(!segments.empty(), "extreme finite host position produced no scheduler output");
        int coveredSamples = 0;
        for (const auto& segment : segments)
        {
            require(segment.stepIndex >= 0 && segment.stepIndex < 16,
                    "extreme finite host position produced an invalid modulo step");
            require(std::isfinite(segment.phaseStart) && std::isfinite(segment.phaseDelta),
                    "extreme finite host position produced non-finite phase data");
            coveredSamples += segment.numSamples;
        }
        require(coveredSamples == 64, "extreme finite host position lost scheduled samples");
    }});
}

} // namespace zikada::tests
