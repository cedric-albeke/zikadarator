#include "engine/SliceEngine.h"

#include <cmath>
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

void requireNear(float actual, float expected, float tolerance, const char* message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(message);
}

void writeRamp(zikada::SliceEngine& engine, int numSamples)
{
    std::vector<float> left(static_cast<size_t>(numSamples), 0.0f);
    std::vector<float> right(static_cast<size_t>(numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        left[static_cast<size_t>(i)] = static_cast<float>(i + 1);
        right[static_cast<size_t>(i)] = static_cast<float>(100 + i + 1);
    }

    engine.writeToBuffer(left.data(), right.data(), numSamples);
}

} // namespace

namespace zikada::tests {

void addSliceEngineTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"SliceEngine reverse mode plays a frozen slice backwards", []
    {
        constexpr double sampleRate = 64.0;
        constexpr int blockSize = 16;
        SliceEngine engine;
        engine.prepare(sampleRate, blockSize);
        engine.setTempo(60.0);
        engine.setPlaybackMode(SliceEngine::PlaybackMode::Reverse, 1);

        writeRamp(engine, blockSize);
        engine.triggerSlice(0);

        std::vector<float> left(16, 0.0f);
        std::vector<float> right(16, 0.0f);
        engine.process(left.data(), right.data(), 16);

        requireNear(left[1], 15.0f, 0.0001f, "reverse playback should continue from the newest slice sample after the fade-in edge");
        requireNear(left[14], 2.0f, 0.0001f, "reverse playback should approach the oldest slice sample before the fade-out edge");
        requireNear(right[1], 115.0f, 0.0001f, "reverse playback should preserve stereo right data");
    }});

    tests.push_back({"SliceEngine stutter mode repeats a short grain without chasing new input", []
    {
        constexpr double sampleRate = 64.0;
        constexpr int blockSize = 16;
        SliceEngine engine;
        engine.prepare(sampleRate, blockSize);
        engine.setTempo(60.0);
        engine.setPlaybackMode(SliceEngine::PlaybackMode::Stutter, 4);

        writeRamp(engine, blockSize);
        engine.triggerSlice(0);

        std::vector<float> zeros(16, 0.0f);
        engine.writeToBuffer(zeros.data(), zeros.data(), 16);

        std::vector<float> left(16, 0.0f);
        std::vector<float> right(16, 0.0f);
        engine.process(left.data(), right.data(), 16);

        for (int repeat = 1; repeat < 3; ++repeat)
        {
            const int offset = repeat * 4;
            requireNear(left[static_cast<size_t>(offset + 0)], 1.0f, 0.0001f, "stutter should repeat the frozen grain start");
            requireNear(left[static_cast<size_t>(offset + 3)], 4.0f, 0.0001f, "stutter should repeat the frozen grain end");
        }
    }});

    tests.push_back({"SliceEngine applies de-click fades to slice edges", []
    {
        constexpr double sampleRate = 64.0;
        constexpr int blockSize = 16;
        SliceEngine engine;
        engine.prepare(sampleRate, blockSize);
        engine.setTempo(60.0);
        engine.setPlaybackMode(SliceEngine::PlaybackMode::Forward, 1);

        std::vector<float> ones(blockSize, 1.0f);
        engine.writeToBuffer(ones.data(), ones.data(), blockSize);
        engine.triggerSlice(0);

        std::vector<float> left(blockSize, 0.0f);
        std::vector<float> right(blockSize, 0.0f);
        engine.process(left.data(), right.data(), blockSize);

        requireNear(left.front(), 0.0f, 0.0001f, "slice fade-in should suppress the first sample discontinuity");
        requireNear(right.front(), 0.0f, 0.0001f, "slice fade-in should suppress the first right-channel sample discontinuity");
        requireNear(left[static_cast<size_t>(blockSize / 2)], 1.0f, 0.0001f, "slice fade should leave the body of the slice untouched");
        requireNear(left.back(), 0.0f, 0.0001f, "slice fade-out should suppress the last sample discontinuity");
    }});

    tests.push_back({"SliceEngine bounds maximum capture work per process chunk", []
    {
        constexpr double sampleRate = 192000.0;
        constexpr int blockSize = 32;
        SliceEngine engine;
        engine.prepare(sampleRate, blockSize);
        engine.setTempo(20.0);
        engine.setPlaybackMode(SliceEngine::PlaybackMode::Forward, 1);

        const int historySamples = static_cast<int>(sampleRate * 12.0);
        std::vector<float> history(static_cast<size_t>(historySamples), 0.5f);
        engine.writeToBuffer(history.data(), history.data(), historySamples);
        engine.beginProcessChunk(blockSize);
        engine.triggerSlice(15);

        std::vector<float> left(blockSize, 0.0f);
        std::vector<float> right(blockSize, 0.0f);
        engine.process(left.data(), right.data(), blockSize);

        require(engine.getCaptureFramesThisChunkForTesting() <= engine.getCaptureBudgetForTesting(),
                "slice capture must stay inside its per-chunk frame budget");
        require(engine.getPendingCaptureFramesForTesting() > 0,
                "maximum slice capture should be incremental instead of completing synchronously");
        require(left[1] > 0.0f, "pending slice capture must still render immediately from pinned history");
    }});
}

} // namespace zikada::tests
