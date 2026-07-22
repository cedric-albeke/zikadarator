#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace zikada {

class StepScheduler
{
public:
    struct HostBlock
    {
        double sampleRate{44100.0};
        double bpm{120.0};
        double ppqPerStep{0.5};
        double startPpq{0.0};
        int numSamples{0};
        bool playing{false};
    };

    struct Segment
    {
        int startSample{0};
        int numSamples{0};
        int stepIndex{0};
        std::int64_t absoluteStepIndex{0};
        double phaseStart{0.0};
        double phaseDelta{0.0};
        bool playing{false};
        bool startsNewStep{false};
    };

    [[nodiscard]] std::vector<Segment> makeHostSegments(const HostBlock& block) const
    {
        std::vector<Segment> result;
        if (block.numSamples <= 0)
            return result;

        constexpr int maximumSegments = 64;
        const auto safeBpm = sanitizeBpm(block.bpm);
        const auto safeSampleRate = sanitizeSampleRate(block.sampleRate);
        const auto ppqPerSample = (safeBpm / 60.0) / safeSampleRate;
        result.reserve(static_cast<size_t>(std::min(block.numSamples, maximumSegments)));

        for (int blockOffset = 0; blockOffset < block.numSamples;)
        {
            auto chunk = block;
            chunk.startPpq = block.startPpq + static_cast<double>(blockOffset) * ppqPerSample;
            chunk.numSamples = getMaximumSafeBlockSize(chunk,
                                                       block.numSamples - blockOffset,
                                                       maximumSegments);

            Segment stackSegments[maximumSegments]{};
            const auto count = makeHostSegments(chunk, stackSegments, maximumSegments);
            for (int index = 0; index < count; ++index)
            {
                stackSegments[index].startSample += blockOffset;
                result.push_back(stackSegments[index]);
            }

            blockOffset += chunk.numSamples;
        }

        return result;
    }

    int makeHostSegments(const HostBlock& block, Segment* segments, int maxSegments) const
    {
        if (segments == nullptr || maxSegments <= 0 || block.numSamples <= 0)
            return 0;

        const auto safeBpm = sanitizeBpm(block.bpm);
        const auto safeSampleRate = sanitizeSampleRate(block.sampleRate);
        const auto safePpqPerStep = sanitizePpqPerStep(block.ppqPerStep);
        const auto safeStartPpq = sanitizePpq(block.startPpq, safePpqPerStep);
        const auto ppqPerSample = (safeBpm / 60.0) / safeSampleRate;
        const auto samplesPerStep = safePpqPerStep / ppqPerSample;
        const auto phaseDelta = samplesPerStep > 0.0 ? 1.0 / samplesPerStep : 0.0;

        if (!block.playing || ppqPerSample <= 0.0)
        {
            const auto absoluteStep = absoluteStepFromPpq(safeStartPpq, safePpqPerStep);
            segments[0] = {0,
                           block.numSamples,
                           stepFromAbsolute(absoluteStep),
                           absoluteStep,
                           phaseFromPpq(safeStartPpq, safePpqPerStep),
                           phaseDelta,
                           false,
                           false};
            return 1;
        }

        auto cursor = 0;
        auto first = true;
        auto count = 0;
        while (cursor < block.numSamples)
        {
            const auto cursorPpq = safeStartPpq + static_cast<double>(cursor) * ppqPerSample;
            const auto absoluteStep = absoluteStepFromPpq(cursorPpq, safePpqPerStep);
            const auto phase = phaseFromPpq(cursorPpq, safePpqPerStep);
            const auto nextBoundaryPpq = (static_cast<double>(absoluteStep) + 1.0) * safePpqPerStep;
            const int remainingSamples = block.numSamples - cursor;
            const auto exactSamplesToBoundary = std::ceil((nextBoundaryPpq - cursorPpq) / ppqPerSample);
            int samplesToBoundary = remainingSamples;
            if (std::isfinite(exactSamplesToBoundary) && exactSamplesToBoundary < remainingSamples)
                samplesToBoundary = exactSamplesToBoundary <= 1.0
                                  ? 1
                                  : static_cast<int>(exactSamplesToBoundary);

            if (count == maxSegments - 1)
                samplesToBoundary = block.numSamples - cursor;

            segments[count] = {cursor,
                               samplesToBoundary,
                               stepFromAbsolute(absoluteStep),
                               absoluteStep,
                               phase,
                               phaseDelta,
                               true,
                               !first && phase <= phaseDelta * 1.5};
            cursor += samplesToBoundary;
            first = false;
            ++count;

            if (count >= maxSegments)
                break;
        }

        if (count > 0)
            segments[0].startsNewStep = segments[0].phaseStart <= phaseDelta * 1.5;

        return count;
    }

    int getMaximumSafeBlockSize(const HostBlock& block, int requestedSamples, int maxSegments) const
    {
        if (requestedSamples <= 0 || maxSegments <= 0 || !block.playing)
            return requestedSamples;

        const auto safeBpm = sanitizeBpm(block.bpm);
        const auto safeSampleRate = sanitizeSampleRate(block.sampleRate);
        const auto safePpqPerStep = sanitizePpqPerStep(block.ppqPerStep);
        const auto ppqPerSample = (safeBpm / 60.0) / safeSampleRate;
        const auto samplesPerStep = safePpqPerStep / ppqPerSample;
        const auto usableSegments = std::max(1, maxSegments - 2);
        const auto safeSamples = std::floor(samplesPerStep * static_cast<double>(usableSegments));

        if (!std::isfinite(safeSamples) || safeSamples >= static_cast<double>(requestedSamples))
            return requestedSamples;

        return safeSamples <= 1.0 ? 1 : static_cast<int>(safeSamples);
    }

private:
    static constexpr std::int64_t maximumAbsoluteStep = std::int64_t{1} << 40;

    static double sanitizeBpm(double bpm)
    {
        return std::isfinite(bpm) && bpm > 0.0 ? std::clamp(bpm, 1.0, 100000.0) : 120.0;
    }

    static double sanitizeSampleRate(double sampleRate)
    {
        return std::isfinite(sampleRate) && sampleRate > 0.0
             ? std::clamp(sampleRate, 1.0, 10000000.0)
             : 44100.0;
    }

    static double sanitizePpqPerStep(double ppqPerStep)
    {
        return std::isfinite(ppqPerStep) && ppqPerStep > 0.0
             ? std::clamp(ppqPerStep, 1.0e-9, 1000000.0)
             : 0.5;
    }

    static double sanitizePpq(double ppq, double ppqPerStep)
    {
        if (!std::isfinite(ppq))
            return 0.0;

        const auto limit = static_cast<double>(maximumAbsoluteStep) * ppqPerStep;
        return std::clamp(ppq, -limit, limit);
    }

    static std::int64_t absoluteStepFromPpq(double ppq, double ppqPerStep)
    {
        const auto rawStep = std::floor(ppq / ppqPerStep);
        const auto bounded = std::clamp(rawStep,
                                        -static_cast<double>(maximumAbsoluteStep),
                                        static_cast<double>(maximumAbsoluteStep));
        return static_cast<std::int64_t>(bounded);
    }

    static int stepFromAbsolute(std::int64_t absoluteStep)
    {
        const auto wrapped = absoluteStep % 16;
        return static_cast<int>(wrapped < 0 ? wrapped + 16 : wrapped);
    }

    static double phaseFromPpq(double ppq, double ppqPerStep)
    {
        const auto raw = ppq / ppqPerStep;
        return raw - std::floor(raw);
    }
};

} // namespace zikada
