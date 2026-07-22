#include "engine/DelayEngine.h"
#include "engine/FilterEngine.h"
#include "engine/LoopEngine.h"
#include "engine/MixUtils.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
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

float rms(const std::vector<float>& samples)
{
    const auto energy = std::accumulate(samples.begin(), samples.end(), 0.0f,
                                        [](float total, float sample) { return total + sample * sample; });
    return std::sqrt(energy / static_cast<float>(samples.size()));
}

} // namespace

namespace zikada::tests {

void addLaneTransitionTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"Lane mix blends lane output without attenuating lane input", []
    {
        requireNear(blendLaneMixSample(1.0f, 0.0f, 0.0f), 1.0f, 0.0001f,
                    "zero lane mix should preserve the lane input");
        requireNear(blendLaneMixSample(1.0f, 0.0f, 0.5f), 0.5f, 0.0001f,
                    "half lane mix should crossfade between lane input and lane output");
        requireNear(blendLaneMixSample(1.0f, 0.0f, 1.0f), 0.0f, 0.0001f,
                    "full lane mix should use the lane output");
    }});

    tests.push_back({"LoopEngine replays newest audio after history wraps", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 128;
        LoopEngine loop;
        loop.prepare(sampleRate, blockSize);

        std::vector<float> zeros(static_cast<size_t>(sampleRate * 4.0), 0.0f);
        loop.captureInput(zeros.data(), zeros.data(), static_cast<int>(zeros.size()));

        std::vector<float> ones(512, 1.0f);
        loop.captureInput(ones.data(), ones.data(), static_cast<int>(ones.size()));

        std::vector<float> left(blockSize, 0.0f);
        std::vector<float> right(blockSize, 0.0f);
        loop.setLoopParameters(static_cast<float>(blockSize / sampleRate), 1.0f, false, 1.0f);
        loop.setEnabled(true);
        loop.trigger();
        loop.process(left.data(), right.data(), blockSize);

        const auto sum = std::accumulate(left.begin(), left.end(), 0.0f,
                                         [](float total, float sample) { return total + std::abs(sample); });
        require(sum > 100.0f, "loop output should contain recently captured ones after history wrap");
    }});

    tests.push_back({"LoopEngine smooths playback discontinuities at loop wrap", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int loopSamples = 96;
        LoopEngine loop;
        loop.prepare(sampleRate, loopSamples);

        std::vector<float> ramp(static_cast<size_t>(loopSamples), 0.0f);
        for (int i = 0; i < loopSamples; ++i)
            ramp[static_cast<size_t>(i)] = -1.0f + 2.0f * static_cast<float>(i) / static_cast<float>(loopSamples - 1);

        loop.captureInput(ramp.data(), ramp.data(), loopSamples);
        loop.setLoopParameters(static_cast<float>(loopSamples / sampleRate), 1.0f, false, 1.0f);
        loop.setEnabled(true);
        loop.trigger();

        std::vector<float> left(static_cast<size_t>(loopSamples * 2), 0.0f);
        std::vector<float> right(static_cast<size_t>(loopSamples * 2), 0.0f);
        loop.process(left.data(), right.data(), static_cast<int>(left.size()));

        float largestJump = 0.0f;
        for (size_t i = 1; i < left.size(); ++i)
            largestJump = std::max(largestJump, std::abs(left[i] - left[i - 1]));

        require(largestJump < 1.25f, "loop wrap should be crossfaded instead of hard jumping between unrelated endpoints");
    }});

    tests.push_back({"LoopEngine freezes triggered audio instead of chasing rolling input", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int loopSamples = 64;
        LoopEngine loop;
        loop.prepare(sampleRate, loopSamples);

        std::vector<float> pattern(loopSamples, 0.0f);
        for (int i = 0; i < loopSamples; ++i)
            pattern[static_cast<size_t>(i)] = static_cast<float>(i + 1) / static_cast<float>(loopSamples);

        loop.captureInput(pattern.data(), pattern.data(), loopSamples);
        loop.setLoopParameters(static_cast<float>(loopSamples / sampleRate), 1.0f, false, 1.0f, 0.0f);
        loop.setEnabled(true);
        loop.trigger();

        std::vector<float> first(loopSamples, 0.0f);
        std::vector<float> firstRight(loopSamples, 0.0f);
        loop.process(first.data(), firstRight.data(), loopSamples);

        std::vector<float> zeros(loopSamples, 0.0f);
        loop.captureInput(zeros.data(), zeros.data(), loopSamples);

        std::vector<float> second(loopSamples, 0.0f);
        std::vector<float> secondRight(loopSamples, 0.0f);
        loop.process(second.data(), secondRight.data(), loopSamples);

        float totalDifference = 0.0f;
        for (int i = 0; i < loopSamples; ++i)
            totalDifference += std::abs(first[static_cast<size_t>(i)] - second[static_cast<size_t>(i)]);

        require(rms(first) > 0.5f, "triggered loop should contain the captured pattern");
        require(totalDifference < 0.001f, "triggered loop should keep repeating the frozen pattern after later input changes");
    }});

    tests.push_back({"LoopEngine bounds maximum capture work per process chunk", []
    {
        constexpr double sampleRate = 192000.0;
        constexpr int blockSize = 32;
        LoopEngine loop;
        loop.prepare(sampleRate, blockSize);

        std::vector<float> history(static_cast<size_t>(sampleRate * 4.0), 0.25f);
        loop.captureInput(history.data(), history.data(), static_cast<int>(history.size()));
        loop.setLoopParameters(4.0f, 1.0f, false, 1.0f, 0.0f);
        loop.setEnabled(true);
        loop.beginProcessChunk(blockSize);
        loop.trigger();

        std::vector<float> left(blockSize, 0.0f);
        std::vector<float> right(blockSize, 0.0f);
        loop.process(left.data(), right.data(), blockSize);

        require(loop.getCaptureFramesThisChunkForTesting() <= loop.getCaptureBudgetForTesting(),
                "loop capture must stay inside its per-chunk frame budget");
        require(loop.getPendingCaptureFramesForTesting() > 0,
                "maximum loop capture should be incremental instead of completing synchronously");
        require(rms(left) > 0.01f, "pending loop capture must still render immediately from pinned history");
    }});

    tests.push_back({"DelayEngine interpolates fractional delay times", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int numSamples = 16;
        DelayEngine delay;
        delay.prepare(sampleRate, numSamples);
        delay.setEnabled(true);
        delay.setMix(1.0f);
        delay.setFeedback(0.0f);
        delay.setDelayTime(1.5f / static_cast<float>(sampleRate));

        float left[numSamples]{};
        float right[numSamples]{};
        left[0] = 1.0f;
        right[0] = 1.0f;

        delay.process(left, right, numSamples);

        int nonzeroSamples = 0;
        for (float sample : left)
        {
            require(std::isfinite(sample), "delay output should stay finite");
            if (std::abs(sample) > 0.001f)
                ++nonzeroSamples;
        }

        require(nonzeroSamples >= 2, "fractional delay should distribute an impulse across adjacent samples");
        requireNear(std::accumulate(left, left + numSamples, 0.0f), 1.0f, 0.01f, "fractional delay should preserve impulse energy");
    }});

    tests.push_back({"DelayEngine smooths delay-time changes", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int numSamples = 2048;
        DelayEngine delay;
        delay.prepare(sampleRate, 128);
        delay.setEnabled(true);
        delay.setMix(1.0f);
        delay.setFeedback(0.35f);
        delay.setDelayTime(0.010f);

        std::vector<float> left(numSamples, 0.0f);
        std::vector<float> right(numSamples, 0.0f);
        for (int i = 0; i < numSamples; ++i)
        {
            const float sample = std::sin(static_cast<float>(i) * 0.071f);
            left[static_cast<size_t>(i)] = sample;
            right[static_cast<size_t>(i)] = sample;
        }

        delay.process(left.data(), right.data(), 512);
        delay.setDelayTime(0.120f);
        delay.process(left.data() + 512, right.data() + 512, numSamples - 512);

        float largestJump = 0.0f;
        for (int i = 1; i < numSamples; ++i)
        {
            require(std::isfinite(left[static_cast<size_t>(i)]), "delay output should stay finite after modulation");
            largestJump = std::max(largestJump, std::abs(left[static_cast<size_t>(i)] - left[static_cast<size_t>(i - 1)]));
        }

        require(largestJump < 1.5f, "delay modulation should not create a hard single-sample jump");
    }});

    tests.push_back({"FilterEngine band reject keeps DC instead of negating bandpass", []
    {
        constexpr int numSamples = 4096;
        FilterEngine filter;
        filter.prepare(48000.0, 128);
        filter.setEnabled(true);
        filter.setFilterType(FilterEngine::FilterType::BandReject);
        filter.setCutoff(1000.0f);
        filter.setResonance(0.707f);

        float output = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            output = filter.processSampleLeft(1.0f);

        require(output > 0.75f, "band reject should pass steady DC content");
    }});

    tests.push_back({"FilterEngine lowpass 24 attenuates more than lowpass 12", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int numSamples = 4096;
        constexpr float frequency = 8000.0f;
        std::vector<float> input(numSamples, 0.0f);
        for (int i = 0; i < numSamples; ++i)
            input[static_cast<size_t>(i)] = std::sin(2.0f * juce::MathConstants<float>::pi * frequency
                                                     * static_cast<float>(i) / static_cast<float>(sampleRate));

        FilterEngine lp12;
        lp12.prepare(sampleRate, 128);
        lp12.setFilterType(FilterEngine::FilterType::LowPass12);
        lp12.setCutoff(1000.0f);
        lp12.setResonance(0.707f);

        FilterEngine lp24;
        lp24.prepare(sampleRate, 128);
        lp24.setFilterType(FilterEngine::FilterType::LowPass24);
        lp24.setCutoff(1000.0f);
        lp24.setResonance(0.707f);

        std::vector<float> out12(numSamples, 0.0f);
        std::vector<float> out24(numSamples, 0.0f);
        for (int i = 0; i < numSamples; ++i)
        {
            out12[static_cast<size_t>(i)] = lp12.processSampleLeft(input[static_cast<size_t>(i)]);
            out24[static_cast<size_t>(i)] = lp24.processSampleLeft(input[static_cast<size_t>(i)]);
        }

        require(rms(out24) < rms(out12) * 0.55f, "lowpass 24 should be a steeper cascaded filter");
    }});

    tests.push_back({"FilterEngine combines and deduplicates parameter updates", []
    {
        FilterEngine filter;
        filter.prepare(48000.0, 128);
        const auto initialUpdates = filter.getParameterUpdateCountForTesting();

        filter.setParameters(1200.0f, 2.0f);
        require(filter.getParameterUpdateCountForTesting() == initialUpdates + 1,
                "cutoff and resonance should share one coefficient update");

        filter.setParameters(1200.0f, 2.0f);
        require(filter.getParameterUpdateCountForTesting() == initialUpdates + 1,
                "unchanged filter parameters should not rebuild coefficients");
    }});

    tests.push_back({"FilterEngine comb reset does not expose stale delay history", []
    {
        FilterEngine filter;
        filter.prepare(1000.0, 32);
        filter.setFilterType(FilterEngine::FilterType::Comb);
        filter.setParameters(100.0f, 2.0f);

        filter.processSampleLeft(1.0f);
        for (int i = 0; i < 10; ++i)
            filter.processSampleLeft(0.0f);

        filter.setFilterType(FilterEngine::FilterType::LowPass12);
        filter.setFilterType(FilterEngine::FilterType::Comb);

        for (int i = 0; i < 16; ++i)
            requireNear(filter.processSampleLeft(0.0f), 0.0f, 0.0001f,
                        "logical comb reset should hide samples from the previous generation");
    }});
}

} // namespace zikada::tests
