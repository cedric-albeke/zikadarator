#pragma once

#include <algorithm>
#include <cmath>
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

        Segment stackSegments[64]{};
        const auto count = makeHostSegments(block, stackSegments, 64);
        result.assign(stackSegments, stackSegments + count);
        return result;
    }

    int makeHostSegments(const HostBlock& block, Segment* segments, int maxSegments) const
    {
        if (segments == nullptr || maxSegments <= 0 || block.numSamples <= 0)
            return 0;

        const auto safeBpm = block.bpm > 0.0 ? block.bpm : 120.0;
        const auto safeSampleRate = block.sampleRate > 0.0 ? block.sampleRate : 44100.0;
        const auto safePpqPerStep = block.ppqPerStep > 0.0 ? block.ppqPerStep : 0.5;
        const auto ppqPerSample = (safeBpm / 60.0) / safeSampleRate;
        const auto samplesPerStep = safePpqPerStep / ppqPerSample;
        const auto phaseDelta = samplesPerStep > 0.0 ? 1.0 / samplesPerStep : 0.0;

        if (!block.playing || ppqPerSample <= 0.0)
        {
            segments[0] = {0,
                           block.numSamples,
                           stepFromPpq(block.startPpq, safePpqPerStep),
                           phaseFromPpq(block.startPpq, safePpqPerStep),
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
            const auto cursorPpq = block.startPpq + static_cast<double>(cursor) * ppqPerSample;
            const auto phase = phaseFromPpq(cursorPpq, safePpqPerStep);
            const auto nextBoundaryPpq = (std::floor(cursorPpq / safePpqPerStep) + 1.0) * safePpqPerStep;
            auto samplesToBoundary = static_cast<int>(std::ceil((nextBoundaryPpq - cursorPpq) / ppqPerSample));
            samplesToBoundary = std::clamp(samplesToBoundary, 1, block.numSamples - cursor);

            if (count == maxSegments - 1)
                samplesToBoundary = block.numSamples - cursor;

            segments[count] = {cursor,
                               samplesToBoundary,
                               stepFromPpq(cursorPpq, safePpqPerStep),
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

private:
    static int stepFromPpq(double ppq, double ppqPerStep)
    {
        const auto rawStep = static_cast<int>(std::floor(ppq / ppqPerStep));
        return ((rawStep % 16) + 16) % 16;
    }

    static double phaseFromPpq(double ppq, double ppqPerStep)
    {
        const auto raw = ppq / ppqPerStep;
        return raw - std::floor(raw);
    }
};

} // namespace zikada
