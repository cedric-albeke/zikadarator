#pragma once

#include "../ZikadaLookAndFeel.h"

namespace zikada {

class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay();
    ~WaveformDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void pushSamples(const float* samples, int numSamples);
    void setPlayheadPosition(float normalizedPosition);

    static constexpr int numSlices = 16;

private:
    void timerCallback() override;

    juce::AbstractFifo fifo{1024};
    std::array<float, 1024> audioData{};
    std::array<float, 256> displayPeaks{};
    int numDisplayBins{256};
    std::atomic<float> playheadPos{0.0f};
    std::atomic<bool> needsRepaint{false};
};

}
