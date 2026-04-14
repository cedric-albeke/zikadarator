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
    juce::ignoreUnused(samplesPerBlock);
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

    auto playHead = getPlayHead();
    if (playHead != nullptr)
    {
        juce::AudioPlayHead::CurrentPositionInfo posInfo;
        playHead->getCurrentPosition(posInfo);
        
        isPlayingFlag = posInfo.isPlaying;
        currentBPM = posInfo.bpm;
        
        if (posInfo.isPlaying)
        {
            auto ppq = posInfo.ppqPosition;
            if (ppq.hasValue())
            {
                ppqPosition = *ppq;
                currentStep = static_cast<int>(ppqPosition / ppqPerStep) % 16;
            }
        }
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        juce::ignoreUnused(channelData);
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
    std::unique_ptr<juce::XmlElement> xml(stateTree.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(state.getValueTreeState().state.getType()))
            state.getValueTreeState().replaceState(juce::ValueTree::fromXml(*xmlState));
}

}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new zikada::PluginProcessor();
}
