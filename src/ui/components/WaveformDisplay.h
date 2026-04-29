#pragma once

#include "../ZikadaLookAndFeel.h"

#include <array>
#include <atomic>

namespace zikada {

class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay();
    ~WaveformDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void pushSamples(const float* samples, int numSamples);
    void pushInputSamples(const float* samples, int numSamples);
    void pushOutputSamples(const float* samples, int numSamples);
    void setPlayheadPosition(float normalizedPosition);

    static constexpr int numSlices = 16;

private:
    static constexpr int historySampleCount = 131072;
    static constexpr int displayBinCount = 256;

    struct WaveLane
    {
        std::array<float, historySampleCount> historyData{};
        std::array<float, displayBinCount> displayMins{};
        std::array<float, displayBinCount> displayMaxes{};
        int writePosition{0};
        int samplesAvailable{0};
        float displayGain{1.0f};
    };

    void timerCallback() override;
    void pushLaneSamples(WaveLane& lane, const float* samples, int numSamples);
    void rebuildDisplayBins(WaveLane& lane);
    void drawWaveLane(juce::Graphics& g,
                      juce::Rectangle<float> bounds,
                      const WaveLane& lane,
                      juce::Colour colour,
                      float fillAlpha);

    WaveLane inputLane;
    WaveLane outputLane;
    std::atomic<float> playheadPos{0.0f};
    std::atomic<bool> needsRepaint{false};
};

}
