#include "engine/DelayEngine.h"
#include "engine/FilterEngine.h"
#include "engine/LoopEngine.h"

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
}

} // namespace zikada::tests
