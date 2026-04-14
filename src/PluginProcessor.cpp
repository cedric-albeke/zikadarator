#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    
    if (sequencerStep != lastStep)
    {
        currentStep = sequencerStep;
        lastStep = sequencerStep;
        sliceEngine.triggerSlice(currentStep);

        static constexpr int kFilterLane = 4;
        const auto& stepData = sequencerState.getStepData(kFilterLane, currentStep);
        filterEngine.setCutoff(stepData.filterCutoff);
        filterEngine.setResonance(stepData.filterResonance);
    }
    
    sliceEngine.writeToBuffer(leftChannel, rightChannel, numSamples);

    std::vector<float> sliceLeft(static_cast<size_t>(numSamples), 0.0f);
    std::vector<float> sliceRight(static_cast<size_t>(numSamples), 0.0f);
    sliceEngine.process(sliceLeft.data(), sliceRight.data(), numSamples);

    filterEngine.process(sliceLeft.data(), sliceRight.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        leftChannel[i] = sliceLeft[i];
        rightChannel[i] = sliceRight[i];
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
