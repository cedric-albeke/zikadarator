#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "state/PluginState.h"
#include "state/SequencerState.h"
#include "engine/SequencerEngine.h"
#include "engine/SliceEngine.h"
#include "engine/FilterEngine.h"
#include "engine/ModulationEngine.h"
#include "engine/GainPanEngine.h"

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

    bool isPlaying() const { return isPlayingFlag; }
    double getCurrentBPM() const { return currentBPM; }
    int getCurrentStep() const { return currentStep; }

private:
    PluginState state;
    SequencerState sequencerState;
    SequencerEngine sequencerEngine;
    SliceEngine sliceEngine;
    FilterEngine filterEngine;
    ModulationEngine modulationEngine;
    GainPanEngine gainPanEngine;
    std::atomic<bool> isPlayingFlag{false};
    std::atomic<double> currentBPM{120.0};
    std::atomic<int> currentStep{0};
    int lastStep{-1};
    double sampleRate{44100.0};
    juce::AudioPlayHead::CurrentPositionInfo lastPosInfo;
    double ppqPosition{0.0};
    double ppqPerStep{0.25};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

}

