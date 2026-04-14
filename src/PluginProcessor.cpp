#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace zikada {

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this)
{
}

PluginProcessor::~PluginProcessor() = default;

void PluginProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    sequencerEngine.prepare(newSampleRate, samplesPerBlock);
    sliceEngine.prepare(newSampleRate, samplesPerBlock);
    filterEngine.prepare(newSampleRate, samplesPerBlock);
    modulationEngine.prepare(newSampleRate);
    gainPanEngine.prepare(newSampleRate, samplesPerBlock);
    lastStep = -1;
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto numSamples = buffer.getNumSamples();
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : leftChannel;

    auto currentPlayHead = getPlayHead();
    if (currentPlayHead != nullptr)
    {
        juce::AudioPlayHead::CurrentPositionInfo posInfo;
        currentPlayHead->getCurrentPosition(posInfo);

        isPlayingFlag = posInfo.isPlaying;
        currentBPM = posInfo.bpm;

        if (posInfo.isPlaying && posInfo.ppqPosition >= 0.0)
        {
            ppqPosition = posInfo.ppqPosition;
            currentStep = static_cast<int>(ppqPosition / ppqPerStep) % 16;
        }

        sequencerEngine.setTempo(currentBPM);
        sequencerEngine.setPlaying(isPlayingFlag);
    }

    sequencerEngine.advance(numSamples);
    auto sequencerStep = sequencerEngine.getCurrentStep();

    static constexpr int kFilterLane = 4;

    if (sequencerStep != lastStep)
    {
        currentStep = sequencerStep;
        lastStep = currentStep;
        sliceEngine.triggerSlice(currentStep);

        const auto& stepData = sequencerState.getStepData(kFilterLane, currentStep);
        if (stepData.presetIndex > 0)
        {
            UserSlotData slotData;
            if (stepData.presetIndex >= 1 && stepData.presetIndex <= 4)
                slotData = sequencerState.getUserSlot(kFilterLane, stepData.presetIndex - 1);

            filterEngine.setCutoff(slotData.filterCutoff);
            filterEngine.setResonance(slotData.filterResonance);

            double stepDuration = (currentBPM > 0.0) ? (60.0 / currentBPM * ppqPerStep) : 0.125;
            modulationEngine.setStepData(slotData.modulation, currentBPM, stepDuration);
        }
    }

    sliceEngine.writeToBuffer(leftChannel, rightChannel, numSamples);

    std::vector<float> sliceLeft(static_cast<size_t>(numSamples), 0.0f);
    std::vector<float> sliceRight(static_cast<size_t>(numSamples), 0.0f);
    sliceEngine.process(sliceLeft.data(), sliceRight.data(), numSamples);

    const auto& stepData = sequencerState.getStepData(kFilterLane, currentStep);
    UserSlotData currentSlotData;
    if (stepData.presetIndex >= 1 && stepData.presetIndex <= 4)
        currentSlotData = sequencerState.getUserSlot(kFilterLane, stepData.presetIndex - 1);

    for (int i = 0; i < numSamples; ++i)
    {
        float modValues[ModulationEngine::NumTargets];
        modulationEngine.processSample(sliceLeft[i], sliceRight[i], modValues);

        float cutoffMod = modValues[static_cast<int>(ModulationTarget::FilterCutoff)];
        float modCutoff = currentSlotData.filterCutoff * std::pow(2.0f, cutoffMod * 3.0f);
        filterEngine.setCutoff(modCutoff);

        float left  = filterEngine.processSampleLeft(sliceLeft[i]);
        float right = filterEngine.processSampleRight(sliceRight[i]);

        float volMod = modValues[static_cast<int>(ModulationTarget::Volume)];
        float panMod = modValues[static_cast<int>(ModulationTarget::Pan)];

        float vol = currentSlotData.volume * (1.0f + volMod);
        float pan = juce::jlimit(-1.0f, 1.0f, currentSlotData.pan + panMod);

        constexpr float panLaw = 0.70710678f;
        float angle = (pan + 1.0f) * 0.25f * 3.14159265f;
        float leftGain  = vol * panLaw * std::cos(angle) * 2.0f;
        float rightGain = vol * panLaw * std::sin(angle) * 2.0f;

        leftChannel[i]  = left  * leftGain;
        rightChannel[i] = right * rightGain;
    }

    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor()))
    {
        if (auto* waveform = editor->getWaveformDisplay())
        {
            waveform->pushSamples(buffer.getReadPointer(0), buffer.getNumSamples());
            waveform->setPlayheadPosition(static_cast<float>(currentStep) / 16.0f);
        }
    }
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

bool PluginProcessor::hasEditor() const
{
    return true;
}

const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
    return true;
}

bool PluginProcessor::producesMidi() const
{
    return false;
}

bool PluginProcessor::isMidiEffect() const
{
    return false;
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index)
{
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto stateTree = state.getValueTreeState().copyState();
    stateTree.addChild (sequencerState.toValueTree(), -1, nullptr);
    std::unique_ptr<juce::XmlElement> xml (stateTree.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState == nullptr)
        return;

    auto fullTree = juce::ValueTree::fromXml (*xmlState);

    if (! fullTree.isValid())
        return;

    auto seqChild = fullTree.getChildWithName ("SequencerState");

    if (seqChild.isValid())
        fullTree.removeChild (seqChild, nullptr);

    if (fullTree.hasType (state.getValueTreeState().state.getType()))
        state.getValueTreeState().replaceState (fullTree);

    if (seqChild.isValid())
        sequencerState.fromValueTree (seqChild);
}

}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new zikada::PluginProcessor();
}
