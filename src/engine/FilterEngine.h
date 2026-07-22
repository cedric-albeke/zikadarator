#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cstdint>
#include <vector>

namespace zikada {

class FilterEngine
{
public:
    FilterEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    enum class FilterType
    {
        LowPass12,
        LowPass24,
        HighPass12,
        HighPass24,
        BandPass,
        BandReject,
        Comb,
        NumTypes
    };

    void setFilterType(FilterType type);
    void setParameters(float frequency, float q);
    void setCutoff(float frequency);
    void setResonance(float q);
    void setEnabled(bool enabled);

    void process(float* left, float* right, int numSamples);

    float processSampleLeft(float input);
    float processSampleRight(float input);

#if defined(ZIKADA_ENABLE_TEST_HOOKS)
    [[nodiscard]] std::uint64_t getParameterUpdateCountForTesting() const { return parameterUpdateCount; }
#endif

private:
    double sampleRate{44100.0};
    bool isEnabled{true};
    FilterType currentType{FilterType::LowPass24};
    float cutoffFreq{2000.0f};
    float resonance{0.707f};

    juce::dsp::StateVariableTPTFilter<float> filterLeftA;
    juce::dsp::StateVariableTPTFilter<float> filterRightA;
    juce::dsp::StateVariableTPTFilter<float> filterLeftB;
    juce::dsp::StateVariableTPTFilter<float> filterRightB;
    struct CombDelay
    {
        void prepare(int maximumDelaySamples);
        void reset();
        float popSample(int channel, float delaySamples) const;
        void pushSample(int channel, float sample);

        int capacity{1};
        std::array<std::vector<float>, 2> data;
        std::array<std::vector<std::uint64_t>, 2> generations;
        std::array<int, 2> writePositions{};
        std::array<std::uint64_t, 2> currentGenerations{1, 1};
    };

    CombDelay combDelayLine;
    float maxCombDelaySamples{1.0f};
    float combDelaySamples{1.0f};
    float combFeedback{0.0f};

#if defined(ZIKADA_ENABLE_TEST_HOOKS)
    std::uint64_t parameterUpdateCount{0};
#endif

    void updateFilterType();
    void updateFilterParameters();
    float processSample(int channel, float input);
    float processCombSample(int channel, float input);
    bool isCascadedType() const;
};

}
