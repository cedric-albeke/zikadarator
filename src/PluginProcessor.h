#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "state/PluginState.h"
#include "state/PresetManager.h"
#include "state/SequencerState.h"
#include "engine/SequencerEngine.h"
#include "engine/SliceEngine.h"
#include "engine/LoopEngine.h"
#include "engine/FilterEngine.h"
#include "engine/DelayEngine.h"
#include "engine/ReverbEngine.h"
#include "engine/BitcrushEngine.h"
#include "engine/PitchEngine.h"
#include "engine/ModulationEngine.h"
#include "engine/GainPanEngine.h"
#include "engine/WaveformTap.h"
#include "engine/StepScheduler.h"

#include <array>
#include <cstdint>
#include <vector>

namespace zikada {

class PluginEditor;

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    PluginState& getPluginState() { return state; }
    const PluginState& getPluginState() const { return state; }

    SequencerState& getSequencerState() { return sequencerState; }
    const SequencerState& getSequencerState() const { return sequencerState; }
    PresetManager& getPresetManager() { return presetManager; }
    const PresetManager& getPresetManager() const { return presetManager; }
    WaveformTap& getWaveformTap() { return waveformTap; }
    WaveformTap& getProcessedWaveformTap() { return processedWaveformTap; }

    juce::ValueTree exportFullState();
    void applyFullState(const juce::ValueTree& stateTree);
    bool synchronizeSequencerActiveStateFromParameters();

    bool isPlaying() const { return isPlayingFlag; }
    double getCurrentBPM() const { return currentBPM; }
    double getCurrentPpqPerStep() const { return currentPpqPerStep; }
    double getCurrentSampleRate() const { return sampleRate; }
    int getCurrentStep() const { return currentStep; }
#if defined(ZIKADA_ENABLE_TEST_HOOKS)
    int getScratchCapacityForTesting() const { return scratchCapacitySamples; }
    int getLastProcessChunkCountForTesting() const { return lastProcessChunkCountForTesting; }
    std::uint64_t getLaneOnsetCountForTesting(int lane) const
    {
        return lane >= 0 && lane < SequencerState::NumLanes
             ? laneOnsetCountsForTesting[static_cast<size_t>(lane)]
             : 0;
    }
    std::uint64_t getProcessedSequencerSampleCountForTesting() const
    {
        return processedSequencerSamplesForTesting;
    }
#endif

private:
    struct TriggerIdentity
    {
        std::uint64_t transportGeneration{0};
        std::int64_t absoluteStep{0};
        int rootGridStep{-1};
        int presetIndex{0};
        bool valid{false};
    };

    void prepareScratchBuffers(int maxSamples);
    void resetTransportTracking();
    void updateTransportForBlock(bool useFreeClock,
                                 bool playing,
                                 bool hasHostSamplePosition,
                                 std::int64_t hostSamplePosition,
                                 bool hasHostPpqPosition,
                                 double hostPpqPosition,
                                 double ppqPerSample,
                                 int numSamples,
                                 int stepResolutionIndex);
    void processSegment(float* leftChannel,
                        float* rightChannel,
                        const float* dryLeft,
                        const float* dryRight,
                        int numSamples,
                        bool hasRightChannel,
                        const StepScheduler::Segment& segment,
                        const SequencerState::Snapshot& sequencerSnapshot,
                        double bpm,
                        double blockPpqPerStep,
                        const float* laneMix,
                        const bool* laneMuted,
                        const bool* laneSoloed,
                        bool anySolo,
                        int globalMixMode,
                        float globalDryWet,
                        float outputGain);

    PluginState state;
    PresetManager presetManager;
    SequencerState sequencerState;
    std::atomic<float>* dryWetParameter{nullptr};
    std::atomic<float>* outputGainParameter{nullptr};
    std::atomic<float>* mixModeParameter{nullptr};
    std::atomic<float>* bypassParameter{nullptr};
    std::atomic<float>* clockSourceParameter{nullptr};
    std::atomic<float>* tempoParameter{nullptr};
    std::atomic<float>* stepResolutionParameter{nullptr};
    std::array<std::array<std::atomic<float>*, SequencerState::NumSteps>, SequencerState::NumLanes> stepActiveParameters{};
    std::array<std::atomic<float>*, SequencerState::NumLanes> laneMixParameters{};
    std::array<std::atomic<float>*, SequencerState::NumLanes> laneMuteParameters{};
    std::array<std::atomic<float>*, SequencerState::NumLanes> laneSoloParameters{};
    SequencerEngine sequencerEngine;
    SliceEngine sliceEngine;
    LoopEngine loopEngine;
    FilterEngine filterEngine;
    DelayEngine fx1DelayEngine;
    ReverbEngine fx1ReverbEngine;
    BitcrushEngine fx1BitcrushEngine;
    PitchEngine fx1PitchEngine;
    FilterEngine fx1ToneFilter;
    DelayEngine fx2DelayEngine;
    ReverbEngine fx2ReverbEngine;
    BitcrushEngine fx2BitcrushEngine;
    PitchEngine fx2PitchEngine;
    FilterEngine fx2ToneFilter;
    ModulationEngine modulationEngine;
    GainPanEngine gainPanEngine;
    WaveformTap waveformTap;
    WaveformTap processedWaveformTap;
    StepScheduler stepScheduler;
    std::atomic<bool> isPlayingFlag{false};
    std::atomic<double> currentBPM{120.0};
    std::atomic<double> currentPpqPerStep{0.5};
    std::atomic<int> currentStep{0};
    std::array<TriggerIdentity, SequencerState::NumLanes> lastTriggerIdentities{};
    std::atomic<bool> triggerIdentityInvalidationRequested{false};
    std::uint64_t transportGeneration{0};
    bool transportTrackingInitialized{false};
    bool previousUseFreeClock{false};
    bool previousHostPlaying{false};
    int previousStepResolutionIndex{1};
    bool expectedHostSamplePositionValid{false};
    std::int64_t expectedHostSamplePosition{0};
    bool expectedHostPpqPositionValid{false};
    double expectedHostPpqPosition{0.0};
    double expectedHostPpqTolerance{0.0};
    double sampleRate{44100.0};
    double ppqPosition{0.0};
    double ppqPerStep{0.5};
    int scratchCapacitySamples{0};
#if defined(ZIKADA_ENABLE_TEST_HOOKS)
    int lastProcessChunkCountForTesting{0};
    std::array<std::uint64_t, SequencerState::NumLanes> laneOnsetCountsForTesting{};
    std::uint64_t processedSequencerSamplesForTesting{0};
#endif
    std::vector<float> dryLeftBuffer;
    std::vector<float> dryRightBuffer;
    std::vector<float> monoRightBuffer;
    std::vector<float> wetLeftBuffer;
    std::vector<float> wetRightBuffer;
    std::vector<float> sliceLeftBuffer;
    std::vector<float> sliceRightBuffer;
    std::vector<float> laneInputLeftBuffer;
    std::vector<float> laneInputRightBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

}
