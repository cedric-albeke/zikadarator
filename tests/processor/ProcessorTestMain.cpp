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
    tests.push_back({"host-state roundtrip preserves parameters and sequencer state", []
    {
        PluginProcessor processor;
        processor.prepareToPlay(48000.0, 128);

        setParameter(processor, ParameterIDs::dryWet, 75.0f);
        setParameter(processor, ParameterIDs::outputGain, -6.0f);
        setParameter(processor, ParameterIDs::bypass, 0.0f);
        setParameter(processor, ParameterIDs::clockSource, 1.0f);
        setParameter(processor, ParameterIDs::tempo, 120.0f);
        setParameter(processor, ParameterIDs::stepResolution, 2.0f);

        auto step = processor.getSequencerState().getStepData(0, 0);
        step.active = true;
        step.presetIndex = 3;
        step.chainLength = 2;
        processor.getSequencerState().setStepData(0, 0, step);

        auto slot = processor.getSequencerState().getUserSlot(0, 0);
        slot.volume = 0.75f;
        slot.pan = 0.25f;
        processor.getSequencerState().setUserSlot(0, 0, slot);

        auto snapshot = processor.exportFullState();

        setParameter(processor, ParameterIDs::dryWet, 0.0f);
        setParameter(processor, ParameterIDs::outputGain, 0.0f);
        setParameter(processor, ParameterIDs::bypass, 1.0f);
        setParameter(processor, ParameterIDs::clockSource, 0.0f);
        setParameter(processor, ParameterIDs::tempo, 60.0f);
        setParameter(processor, ParameterIDs::stepResolution, 0.0f);

        auto clearedStep = processor.getSequencerState().getStepData(0, 0);
        clearedStep.active = false;
        clearedStep.presetIndex = 0;
        clearedStep.chainLength = 1;
        processor.getSequencerState().setStepData(0, 0, clearedStep);

        processor.applyFullState(snapshot);

        auto* apvts = &processor.getPluginState().getValueTreeState();
        auto* dryWetParam = apvts->getParameter(ParameterIDs::dryWet);
        auto* outputGainParam = apvts->getParameter(ParameterIDs::outputGain);
        auto* bypassParam = apvts->getParameter(ParameterIDs::bypass);
        auto* clockSourceParam = apvts->getParameter(ParameterIDs::clockSource);
        auto* tempoParam = apvts->getParameter(ParameterIDs::tempo);
        auto* stepResolutionParam = apvts->getParameter(ParameterIDs::stepResolution);

        requireNear(dryWetParam->getValue(), dryWetParam->convertTo0to1(75.0f), 0.001f, "dryWet after roundtrip");
        requireNear(outputGainParam->getValue(), outputGainParam->convertTo0to1(-6.0f), 0.001f, "outputGain after roundtrip");
        requireNear(bypassParam->getValue(), bypassParam->convertTo0to1(0.0f), 0.001f, "bypass after roundtrip");
        requireNear(clockSourceParam->getValue(), clockSourceParam->convertTo0to1(1.0f), 0.001f, "clockSource after roundtrip");
        requireNear(tempoParam->getValue(), tempoParam->convertTo0to1(120.0f), 0.001f, "tempo after roundtrip");
        requireNear(stepResolutionParam->getValue(), stepResolutionParam->convertTo0to1(2.0f), 0.001f, "stepResolution after roundtrip");

        auto restoredStep = processor.getSequencerState().getStepData(0, 0);
        if (!restoredStep.active) throw std::runtime_error("step active not restored after roundtrip");
        if (restoredStep.presetIndex != 3) throw std::runtime_error("step presetIndex not restored after roundtrip");
        if (restoredStep.chainLength != 2) throw std::runtime_error("step chainLength not restored after roundtrip");

        auto restoredSlot = processor.getSequencerState().getUserSlot(0, 0);
        requireNear(restoredSlot.volume, 0.75f, 0.001f, "slot volume not restored after roundtrip");
        requireNear(restoredSlot.pan, 0.25f, 0.001f, "slot pan not restored after roundtrip");
    }});

    tests.push_back({"host layout change does not crash or corrupt state", []
    {
        PluginProcessor processor;
        processor.prepareToPlay(48000.0, 256);

        juce::AudioBuffer<float> stereoBuffer(2, 256);
        fillBuffer(stereoBuffer, 0.5f);
        juce::MidiBuffer midi;
        processor.processBlock(stereoBuffer, midi);

        juce::AudioBuffer<float> monoBuffer(1, 256);
        fillBuffer(monoBuffer, 0.5f);
        midi.clear();
        processor.processBlock(monoBuffer, midi);

        juce::AudioBuffer<float> stereoBuffer2(2, 256);
        fillBuffer(stereoBuffer2, 0.5f);
        midi.clear();
        processor.processBlock(stereoBuffer2, midi);
    }});

    tests.push_back({"invalid host state is ignored without corrupting current state", []
    {
        PluginProcessor processor;
        processor.prepareToPlay(48000.0, 128);

        setParameter(processor, ParameterIDs::dryWet, 64.0f);

        StepData step;
        step.active = true;
        step.presetIndex = 8;
        step.chainLength = 3;
        processor.getSequencerState().setStepData(3, 4, step);

        processor.applyFullState(juce::ValueTree("NotZikadaratorState"));

        auto* dryWetParam = processor.getPluginState().getValueTreeState().getParameter(ParameterIDs::dryWet);
        requireNear(dryWetParam->getValue(), dryWetParam->convertTo0to1(64.0f), 0.001f,
                    "dryWet changed after invalid host state");

        const auto restoredStep = processor.getSequencerState().getStepData(3, 4);
        if (!restoredStep.active) throw std::runtime_error("step active changed after invalid host state");
        if (restoredStep.presetIndex != 8) throw std::runtime_error("step preset changed after invalid host state");
        if (restoredStep.chainLength != 3) throw std::runtime_error("step chain changed after invalid host state");
    }});

    tests.push_back({"malformed binary host state is ignored without corrupting current state", []
    {
        PluginProcessor processor;
        processor.prepareToPlay(48000.0, 128);

        setParameter(processor, ParameterIDs::outputGain, -9.0f);

        StepData step;
        step.active = true;
        step.presetIndex = 5;
        processor.getSequencerState().setStepData(1, 7, step);

        const char malformedState[] = "not valid plugin XML";
        processor.setStateInformation(malformedState, static_cast<int>(sizeof(malformedState)));

        auto* outputGainParam = processor.getPluginState().getValueTreeState().getParameter(ParameterIDs::outputGain);
        requireNear(outputGainParam->getValue(), outputGainParam->convertTo0to1(-9.0f), 0.001f,
                    "outputGain changed after malformed host state");

        const auto restoredStep = processor.getSequencerState().getStepData(1, 7);
        if (!restoredStep.active) throw std::runtime_error("step active changed after malformed host state");
        if (restoredStep.presetIndex != 5) throw std::runtime_error("step preset changed after malformed host state");
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
