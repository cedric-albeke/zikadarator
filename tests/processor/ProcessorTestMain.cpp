#include "PluginProcessor.h"
#include "state/ParameterIDs.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void requireNear(float actual, float expected, float tolerance, const char* message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(std::string(message) + ": expected " + std::to_string(expected)
                                 + ", got " + std::to_string(actual));
}

void setParameter(zikada::PluginProcessor& processor, const juce::String& id, float plainValue)
{
    auto* parameter = processor.getPluginState().getValueTreeState().getParameter(id);
    if (parameter == nullptr)
        throw std::runtime_error("missing parameter: " + id.toStdString());

    parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
}

void activateHardRightEnvelope(zikada::PluginProcessor& processor)
{
    zikada::StepData step;
    step.active = true;
    step.presetIndex = 7;
    processor.getSequencerState().setStepData(2, 0, step);
    setParameter(processor, zikada::getStepActiveID(2, 0), 1.0f);

    zikada::UserSlotData slot;
    slot.volume = 1.0f;
    slot.pan = 1.0f;
    processor.getSequencerState().setUserSlot(2, 0, slot);
}

void configureHardRightEnvelopeProcessor(zikada::PluginProcessor& processor)
{
    setParameter(processor, zikada::ParameterIDs::clockSource, 1.0f);
    setParameter(processor, zikada::ParameterIDs::dryWet, 100.0f);
    activateHardRightEnvelope(processor);
}

void fillBuffer(juce::AudioBuffer<float>& buffer, float sampleValue)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(channel, i, sampleValue);
}

} // namespace

namespace zikada::tests {

void addProcessorTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"mono output folds hard-right processed signal instead of dropping it", []
    {
        PluginProcessor monoProcessor;
        configureHardRightEnvelopeProcessor(monoProcessor);

        PluginProcessor stereoProcessor;
        configureHardRightEnvelopeProcessor(stereoProcessor);

        juce::AudioBuffer<float> monoBuffer(1, 8);
        fillBuffer(monoBuffer, 1.0f);

        juce::AudioBuffer<float> stereoBuffer(2, 8);
        fillBuffer(stereoBuffer, 1.0f);

        juce::MidiBuffer midi;
        monoProcessor.prepareToPlay(48000.0, monoBuffer.getNumSamples());
        monoProcessor.processBlock(monoBuffer, midi);

        midi.clear();
        stereoProcessor.prepareToPlay(48000.0, stereoBuffer.getNumSamples());
        stereoProcessor.processBlock(stereoBuffer, midi);

        const float foldedStereo = 0.5f * (stereoBuffer.getSample(0, 0) + stereoBuffer.getSample(1, 0));

        requireNear(monoBuffer.getSample(0, 0), foldedStereo, 0.001f,
                    "mono output must match folded stereo output");
    }});
}

} // namespace zikada::tests

int main()
{
    std::vector<std::pair<std::string, std::function<void()>>> tests;
    zikada::tests::addProcessorTests(tests);

    int failures = 0;
    for (const auto& [name, test] : tests)
    {
        try
        {
            test();
        }
        catch (const std::exception& ex)
        {
            ++failures;
            std::fprintf(stderr, "FAIL %s: %s\n", name.c_str(), ex.what());
        }
    }

    return failures == 0 ? 0 : 1;
}
