#include "PluginProcessor.h"
#include "state/ParameterIDs.h"
#include "ui/components/StepCell.h"
#include "ui/panels/WorkspacePanel.h"

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

float sumFiniteAbsoluteSamples(const juce::AudioBuffer<float>& buffer,
                               int channel,
                               const char* context)
{
    float energy = 0.0f;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float value = buffer.getSample(channel, sample);
        if (!std::isfinite(value))
            throw std::runtime_error(std::string(context) + " produced a non-finite sample");
        energy += std::abs(value);
    }
    return energy;
}

class TestPlayHead final : public juce::AudioPlayHead
{
public:
    void setPosition(double newBpm, double newPpqPosition, bool playing)
    {
        bpm = newBpm;
        ppqPosition = newPpqPosition;
        isPlaying = playing;
    }

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo position;
        position.setBpm(bpm);
        position.setPpqPosition(ppqPosition);
        position.setIsPlaying(isPlaying);
        return position;
    }

private:
    double bpm{120.0};
    double ppqPosition{0.0};
    bool isPlaying{true};
};

int countRenderedSliceSamples(float tempo, bool automatedStepActive = true)
{
    constexpr double sampleRate = 64.0;
    constexpr int blockSize = 4;
    constexpr int blocksToRender = 16;

    zikada::PluginProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);
    setParameter(processor, zikada::ParameterIDs::clockSource, 1.0f);
    setParameter(processor, zikada::ParameterIDs::tempo, tempo);
    setParameter(processor, zikada::ParameterIDs::stepResolution, 0.0f);
    setParameter(processor, zikada::ParameterIDs::dryWet, 100.0f);

    zikada::StepData sliceStep;
    sliceStep.active = true;
    sliceStep.presetIndex = 5;
    processor.getSequencerState().setStepData(0, 1, sliceStep);
    setParameter(processor, zikada::getStepActiveID(0, 1), automatedStepActive ? 1.0f : 0.0f);

    juce::MidiBuffer midi;
    int renderedSamples = 0;
    const int samplesPerStep = static_cast<int>(std::round(sampleRate * 60.0 / tempo * 0.25));
    for (int block = 0; block < blocksToRender; ++block)
    {
        juce::AudioBuffer<float> buffer(2, blockSize);
        const bool feedingHistory = block * blockSize < samplesPerStep;
        fillBuffer(buffer, feedingHistory ? 1.0f : 0.0f);
        processor.processBlock(buffer, midi);

        if (!feedingHistory)
            for (int sample = 0; sample < blockSize; ++sample)
                if (std::abs(buffer.getSample(0, sample)) > 0.01f)
                    ++renderedSamples;
    }

    return renderedSamples;
}

int countHostSyncedSliceSamples(float hostTempo)
{
    constexpr double sampleRate = 64.0;
    constexpr int blockSize = 4;
    constexpr int blocksToRender = 16;

    TestPlayHead playHead;
    zikada::PluginProcessor processor;
    processor.setPlayHead(&playHead);
    processor.prepareToPlay(sampleRate, blockSize);
    setParameter(processor, zikada::ParameterIDs::clockSource, 0.0f);
    setParameter(processor, zikada::ParameterIDs::tempo, 300.0f);
    setParameter(processor, zikada::ParameterIDs::stepResolution, 0.0f);
    setParameter(processor, zikada::ParameterIDs::dryWet, 100.0f);

    zikada::StepData sliceStep;
    sliceStep.active = true;
    sliceStep.presetIndex = 5;
    processor.getSequencerState().setStepData(0, 1, sliceStep);
    setParameter(processor, zikada::getStepActiveID(0, 1), 1.0f);

    juce::MidiBuffer midi;
    int renderedSamples = 0;
    const int samplesPerStep = static_cast<int>(std::round(sampleRate * 60.0 / hostTempo * 0.25));
    const double ppqPerSample = (hostTempo / 60.0) / sampleRate;

    for (int block = 0; block < blocksToRender; ++block)
    {
        playHead.setPosition(hostTempo,
                             static_cast<double>(block * blockSize) * ppqPerSample,
                             true);

        juce::AudioBuffer<float> buffer(2, blockSize);
        const bool feedingHistory = block * blockSize < samplesPerStep;
        fillBuffer(buffer, feedingHistory ? 1.0f : 0.0f);
        processor.processBlock(buffer, midi);

        if (!feedingHistory)
            for (int sample = 0; sample < blockSize; ++sample)
                if (std::abs(buffer.getSample(0, sample)) > 0.01f)
                    ++renderedSamples;
    }

    requireNear(static_cast<float>(processor.getCurrentBPM()), hostTempo, 0.001f,
                "processor did not retain host BPM");
    return renderedSamples;
}

bool isInteractiveWorkspaceControl(const juce::Component& component)
{
    return dynamic_cast<const juce::Button*>(&component) != nullptr
        || dynamic_cast<const juce::Slider*>(&component) != nullptr
        || dynamic_cast<const juce::TextEditor*>(&component) != nullptr
        || dynamic_cast<const juce::ListBox*>(&component) != nullptr;
}

void requireVisibleWorkspaceControlsContained(const zikada::WorkspacePanel& panel,
                                              const char* modeName,
                                              int width,
                                              int height)
{
    const auto panelBounds = panel.getLocalBounds();

    for (int index = 0; index < panel.getNumChildComponents(); ++index)
    {
        const auto* child = panel.getChildComponent(index);
        if (child == nullptr || !child->isVisible() || !isInteractiveWorkspaceControl(*child))
            continue;

        const auto childBounds = child->getBounds();
        if (childBounds.isEmpty())
            throw std::runtime_error(std::string(modeName) + " control " + std::to_string(index)
                                     + " has empty bounds at " + std::to_string(width) + "x"
                                     + std::to_string(height));

        if (!panelBounds.contains(childBounds))
            throw std::runtime_error(std::string(modeName) + " control " + std::to_string(index)
                                     + " escapes panel bounds at " + std::to_string(width) + "x"
                                     + std::to_string(height));
    }
}

} // namespace

namespace zikada::tests {

void addProcessorTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"slice duration follows resolved free-clock tempo", []
    {
        const int samplesAt60Bpm = countRenderedSliceSamples(60.0f);
        const int samplesAt120Bpm = countRenderedSliceSamples(120.0f);

        if (samplesAt120Bpm < 4)
            throw std::runtime_error("tempo test did not render the reference slice: "
                                     + std::to_string(samplesAt60Bpm) + " vs "
                                     + std::to_string(samplesAt120Bpm));

        if (samplesAt60Bpm <= samplesAt120Bpm + 4)
            throw std::runtime_error("60 BPM slice was not materially longer than 120 BPM after edge fades: "
                                     + std::to_string(samplesAt60Bpm) + " vs "
                                     + std::to_string(samplesAt120Bpm));
    }});

    tests.push_back({"slice duration follows resolved host tempo", []
    {
        const int samplesAt60Bpm = countHostSyncedSliceSamples(60.0f);
        const int samplesAt120Bpm = countHostSyncedSliceSamples(120.0f);

        if (samplesAt120Bpm < 4)
            throw std::runtime_error("host-tempo test did not render the reference slice: "
                                     + std::to_string(samplesAt60Bpm) + " vs "
                                     + std::to_string(samplesAt120Bpm));

        if (samplesAt60Bpm <= samplesAt120Bpm + 4)
            throw std::runtime_error("host 60 BPM slice was not materially longer than host 120 BPM: "
                                     + std::to_string(samplesAt60Bpm) + " vs "
                                     + std::to_string(samplesAt120Bpm));
    }});

    tests.push_back({"step-active host automation gates sequencer rendering", []
    {
        const int automatedOffSamples = countRenderedSliceSamples(120.0f, false);
        if (automatedOffSamples != 0)
            throw std::runtime_error("host automation disabled the step but audio still rendered "
                                     + std::to_string(automatedOffSamples) + " slice samples");
    }});

    tests.push_back({"exported sequencer active state follows host automation", []
    {
        PluginProcessor processor;

        StepData step;
        step.active = true;
        step.presetIndex = 5;
        step.chainLength = 3;
        processor.getSequencerState().setStepData(0, 1, step);
        setParameter(processor, getStepActiveID(0, 1), 0.0f);

        const auto exported = processor.exportFullState();
        const auto sequencer = exported.getChildWithName("SequencerState");
        const auto lane = sequencer.getChild(0);
        const auto exportedStep = lane.getChildWithName("Steps").getChild(1);

        if (static_cast<bool>(exportedStep.getProperty("active", true)))
            throw std::runtime_error("exported sequencer active flag ignored host automation");
        if (static_cast<int>(exportedStep.getProperty("presetIndex", 0)) != 5)
            throw std::runtime_error("export lost sequencer preset metadata");
        if (static_cast<int>(exportedStep.getProperty("chainLength", 1)) != 3)
            throw std::runtime_error("export lost sequencer chain metadata");

        const auto liveStep = processor.getSequencerState().getStepData(0, 1);
        if (!liveStep.active)
            throw std::runtime_error("export mutated the live sequencer active state");
    }});

    tests.push_back({"factory preset APVTS gates match their sequencer patterns", []
    {
        PluginProcessor processor;
        const auto& presets = processor.getPresetManager().getItems();
        int patternedFactoryPresets = 0;

        for (int presetIndex = 0; presetIndex < static_cast<int>(presets.size()); ++presetIndex)
        {
            const auto& preset = presets[static_cast<size_t>(presetIndex)];
            if (!preset.isFactory)
                continue;

            juce::ValueTree loadedState;
            if (!processor.getPresetManager().loadPreset(presetIndex, loadedState))
                throw std::runtime_error("factory preset could not be loaded: " + preset.name.toStdString());

            const auto sequencer = loadedState.getChildWithName("SequencerState");
            if (!sequencer.isValid())
                throw std::runtime_error("factory preset has no sequencer state: " + preset.name.toStdString());

            for (int lane = 0; lane < SequencerState::NumLanes; ++lane)
            {
                const auto muteState = findParameterState(loadedState, getLaneMuteID(lane));
                const auto soloState = findParameterState(loadedState, getLaneSoloID(lane));
                if (!muteState.isValid() || getParameterStateValue(loadedState, getLaneMuteID(lane), 1.0f) > 0.5f)
                    throw std::runtime_error("factory preset does not reset lane mute: "
                                             + preset.name.toStdString() + " lane " + std::to_string(lane));
                if (!soloState.isValid() || getParameterStateValue(loadedState, getLaneSoloID(lane), 1.0f) > 0.5f)
                    throw std::runtime_error("factory preset does not reset lane solo: "
                                             + preset.name.toStdString() + " lane " + std::to_string(lane));

                setParameter(processor, getLaneMuteID(lane), 1.0f);
                setParameter(processor, getLaneSoloID(lane), 1.0f);
            }

            bool hasActiveStep = false;
            for (int lane = 0; lane < SequencerState::NumLanes; ++lane)
            {
                const auto steps = sequencer.getChild(lane).getChildWithName("Steps");
                for (int step = 0; step < SequencerState::NumSteps; ++step)
                {
                    const bool sequencerActive = static_cast<bool>(steps.getChild(step).getProperty("active", false));
                    const bool parameterActive = getParameterStateValue(loadedState, getStepActiveID(lane, step)) > 0.5f;
                    if (sequencerActive != parameterActive)
                        throw std::runtime_error("factory preset has divergent step state: "
                                                 + preset.name.toStdString() + " lane "
                                                 + std::to_string(lane) + " step " + std::to_string(step));
                    hasActiveStep = hasActiveStep || sequencerActive;
                }
            }

            if (hasActiveStep)
                ++patternedFactoryPresets;

            processor.applyFullState(loadedState);
            for (int lane = 0; lane < SequencerState::NumLanes; ++lane)
            {
                const auto steps = sequencer.getChild(lane).getChildWithName("Steps");
                for (int step = 0; step < SequencerState::NumSteps; ++step)
                {
                    const bool expectedActive = static_cast<bool>(steps.getChild(step).getProperty("active", false));
                    const auto restoredStep = processor.getSequencerState().getStepData(lane, step);
                    const auto* activeParameter = processor.getPluginState().getValueTreeState()
                                                      .getRawParameterValue(getStepActiveID(lane, step));
                    if (restoredStep.active != expectedActive
                        || activeParameter == nullptr
                        || (activeParameter->load() > 0.5f) != expectedActive)
                        throw std::runtime_error("factory preset changed while applying: "
                                                 + preset.name.toStdString() + " lane "
                                                 + std::to_string(lane) + " step " + std::to_string(step));
                }

                const auto& apvts = processor.getPluginState().getValueTreeState();
                const auto* muteParameter = apvts.getRawParameterValue(getLaneMuteID(lane));
                const auto* soloParameter = apvts.getRawParameterValue(getLaneSoloID(lane));
                if (muteParameter == nullptr || muteParameter->load() > 0.5f)
                    throw std::runtime_error("factory preset retained lane mute: "
                                             + preset.name.toStdString() + " lane " + std::to_string(lane));
                if (soloParameter == nullptr || soloParameter->load() > 0.5f)
                    throw std::runtime_error("factory preset retained lane solo: "
                                             + preset.name.toStdString() + " lane " + std::to_string(lane));
            }
        }

        if (patternedFactoryPresets == 0)
            throw std::runtime_error("factory preset test found no patterned presets");
    }});

    tests.push_back({"legacy sequencer patterns survive APVTS state migration", []
    {
        PluginProcessor processor;
        auto legacyState = processor.getPluginState().getValueTreeState().copyState();

        SequencerState legacySequencer;
        StepData step;
        step.active = true;
        step.presetIndex = 11;
        step.chainLength = 2;
        legacySequencer.setStepData(5, 12, step);
        setParameterStateValue(legacyState, getStepActiveID(5, 12), 0.0f);
        legacyState.addChild(legacySequencer.toValueTree(), -1, nullptr);

        processor.applyFullState(legacyState);

        const auto restored = processor.getSequencerState().getStepData(5, 12);
        const auto& apvts = processor.getPluginState().getValueTreeState();
        const auto* activeParameter = apvts.getRawParameterValue(getStepActiveID(5, 12));
        if (!restored.active || restored.presetIndex != 11 || restored.chainLength != 2)
            throw std::runtime_error("legacy sequencer pattern was discarded during state migration: active="
                                     + std::to_string(restored.active) + " preset="
                                     + std::to_string(restored.presetIndex) + " chain="
                                     + std::to_string(restored.chainLength) + " tree="
                                     + std::to_string(getParameterStateValue(apvts.state, getStepActiveID(5, 12)))
                                     + " raw=" + std::to_string(activeParameter == nullptr ? -1.0f : activeParameter->load()));

        if (activeParameter == nullptr || activeParameter->load() <= 0.5f)
            throw std::runtime_error("legacy sequencer pattern was not migrated into APVTS");
    }});

    tests.push_back({"host automation synchronizes sequencer metadata without losing preset data", []
    {
        PluginProcessor processor;

        StepData step;
        step.active = true;
        step.presetIndex = 17;
        step.chainLength = 2;
        processor.getSequencerState().setStepData(2, 6, step);
        setParameter(processor, getStepActiveID(2, 6), 0.0f);

        processor.synchronizeSequencerActiveStateFromParameters();

        const auto synchronized = processor.getSequencerState().getStepData(2, 6);
        if (synchronized.active) throw std::runtime_error("sequencer metadata ignored automated step-off state");
        if (synchronized.presetIndex != 17) throw std::runtime_error("active-state sync discarded preset metadata");
        if (synchronized.chainLength != 2) throw std::runtime_error("active-state sync discarded chain metadata");
    }});

    tests.push_back({"step automation retriggers slice when enabled inside the current step", []
    {
        constexpr double sampleRate = 64.0;
        constexpr int blockSize = 4;
        PluginProcessor processor;
        processor.prepareToPlay(sampleRate, blockSize);
        setParameter(processor, ParameterIDs::clockSource, 1.0f);
        setParameter(processor, ParameterIDs::tempo, 60.0f);
        setParameter(processor, ParameterIDs::stepResolution, 0.0f);
        setParameter(processor, ParameterIDs::dryWet, 100.0f);

        StepData step;
        step.active = true;
        step.presetIndex = 5;
        processor.getSequencerState().setStepData(0, 1, step);
        setParameter(processor, getStepActiveID(0, 1), 0.0f);

        juce::MidiBuffer midi;
        for (int block = 0; block < 4; ++block)
        {
            juce::AudioBuffer<float> history(2, blockSize);
            fillBuffer(history, 1.0f);
            processor.processBlock(history, midi);
        }

        juce::AudioBuffer<float> disabledBlock(2, blockSize);
        disabledBlock.clear();
        processor.processBlock(disabledBlock, midi);

        setParameter(processor, getStepActiveID(0, 1), 1.0f);
        juce::AudioBuffer<float> enabledBlock(2, blockSize);
        enabledBlock.clear();
        processor.processBlock(enabledBlock, midi);

        float outputEnergy = 0.0f;
        for (int sample = 0; sample < blockSize; ++sample)
            outputEnergy += std::abs(enabledBlock.getSample(0, sample));

        if (outputEnergy <= 0.01f)
            throw std::runtime_error("slice did not retrigger after host automation enabled the current step");
    }});

    tests.push_back({"step automation retriggers loop when enabled inside the current step", []
    {
        constexpr double sampleRate = 64.0;
        constexpr int blockSize = 4;
        PluginProcessor processor;
        processor.prepareToPlay(sampleRate, blockSize);
        setParameter(processor, ParameterIDs::clockSource, 1.0f);
        setParameter(processor, ParameterIDs::tempo, 60.0f);
        setParameter(processor, ParameterIDs::stepResolution, 0.0f);
        setParameter(processor, ParameterIDs::dryWet, 100.0f);

        StepData step;
        step.active = true;
        step.presetIndex = 5;
        processor.getSequencerState().setStepData(1, 1, step);
        setParameter(processor, getStepActiveID(1, 1), 0.0f);

        juce::MidiBuffer midi;
        for (int block = 0; block < 4; ++block)
        {
            juce::AudioBuffer<float> history(2, blockSize);
            fillBuffer(history, 1.0f);
            processor.processBlock(history, midi);
        }

        juce::AudioBuffer<float> disabledBlock(2, blockSize);
        disabledBlock.clear();
        processor.processBlock(disabledBlock, midi);

        const float disabledEnergy = sumFiniteAbsoluteSamples(disabledBlock, 0, "disabled loop step");
        if (disabledEnergy > 0.0001f)
            throw std::runtime_error("loop rendered while its APVTS step gate was disabled: "
                                     + std::to_string(disabledEnergy));

        setParameter(processor, getStepActiveID(1, 1), 1.0f);
        juce::AudioBuffer<float> enabledBlock(2, blockSize);
        enabledBlock.clear();
        processor.processBlock(enabledBlock, midi);

        const float outputEnergy = sumFiniteAbsoluteSamples(enabledBlock, 0, "enabled loop step");

        if (outputEnergy <= 0.01f)
            throw std::runtime_error("loop did not retrigger after host automation enabled the current step");
    }});

    tests.push_back({"step automation configures filter when enabled inside the current step", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;
        PluginProcessor processor;
        processor.prepareToPlay(sampleRate, blockSize);
        setParameter(processor, ParameterIDs::clockSource, 1.0f);
        setParameter(processor, ParameterIDs::tempo, 60.0f);
        setParameter(processor, ParameterIDs::stepResolution, 0.0f);
        setParameter(processor, ParameterIDs::dryWet, 100.0f);

        StepData step;
        step.active = true;
        step.presetIndex = 7;
        processor.getSequencerState().setStepData(4, 0, step);
        setParameter(processor, getStepActiveID(4, 0), 0.0f);

        juce::MidiBuffer midi;
        juce::AudioBuffer<float> disabledBlock(2, blockSize);
        fillBuffer(disabledBlock, 1.0f);
        processor.processBlock(disabledBlock, midi);

        for (int sample = 0; sample < blockSize; ++sample)
        {
            const float value = disabledBlock.getSample(0, sample);
            if (!std::isfinite(value))
                throw std::runtime_error("disabled filter step produced a non-finite sample");
            requireNear(value, 1.0f, 0.0001f, "disabled filter step changed the input");
        }

        setParameter(processor, getStepActiveID(4, 0), 1.0f);
        juce::AudioBuffer<float> enabledBlock(2, blockSize);
        float initialOutputEnergy = 0.0f;
        float finalOutputEnergy = 0.0f;
        for (int block = 0; block < 64; ++block)
        {
            fillBuffer(enabledBlock, 1.0f);
            processor.processBlock(enabledBlock, midi);
            const float blockEnergy = sumFiniteAbsoluteSamples(enabledBlock, 0, "enabled filter step");
            if (block == 0)
                initialOutputEnergy = blockEnergy;
            if (block == 63)
                finalOutputEnergy = blockEnergy;
        }

        if (initialOutputEnergy <= 0.05f)
            throw std::runtime_error("filter activation muted the signal instead of producing a high-pass transient: "
                                     + std::to_string(initialOutputEnergy));

        const float meanAbsoluteOutput = finalOutputEnergy / static_cast<float>(blockSize);

        if (meanAbsoluteOutput >= 0.05f)
            throw std::runtime_error("filter retained its default low-pass state after same-step automation: "
                                     + std::to_string(meanAbsoluteOutput));
    }});

    tests.push_back({"workspace controls stay visible and contained at supported logical sizes", []
    {
        WorkspacePanel panel;

        for (const auto [width, height] : {std::pair{1184, 718}, std::pair{900, 600}})
        {
            panel.setSize(width, height);

            panel.setMode(WorkspacePanel::Mode::Presets);
            requireVisibleWorkspaceControlsContained(panel, "presets", width, height);

            panel.setMode(WorkspacePanel::Mode::Settings);
            requireVisibleWorkspaceControlsContained(panel, "settings", width, height);
        }
    }});

    tests.push_back({"step chain badge stays inside the painted cell", []
    {
        StepCell cell(0, 0);
        cell.setSize(48, 52);

        const auto badge = cell.getChainBadgeBounds();
        if (badge.isEmpty())
            throw std::runtime_error("chain badge bounds are empty");
        if (!cell.getLocalBounds().toFloat().contains(badge))
            throw std::runtime_error("chain badge escapes the step cell");
        if (badge.getCentreX() >= cell.getWidth() * 0.5f)
            throw std::runtime_error("chain badge is not aligned with the painted upper-left affordance");
    }});

    tests.push_back({"step cell follows its host-controlled toggle state", []
    {
        StepCell cell(0, 0);
        cell.setActive(true);
        cell.setToggleState(false, juce::dontSendNotification);

        if (cell.isActive())
            throw std::runtime_error("step cell remained active after its APVTS toggle was disabled");
    }});

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
        setParameter(processor, getStepActiveID(0, 0), 1.0f);

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

    tests.push_back({"wrong-root host state cannot replace sequencer data", []
    {
        PluginProcessor processor;

        StepData step;
        step.active = true;
        step.presetIndex = 12;
        step.chainLength = 4;
        processor.getSequencerState().setStepData(4, 9, step);

        juce::ValueTree wrongRoot("NotZikadaratorState");
        wrongRoot.addChild(juce::ValueTree("SequencerState"), -1, nullptr);
        processor.applyFullState(wrongRoot);

        const auto preservedStep = processor.getSequencerState().getStepData(4, 9);
        if (!preservedStep.active) throw std::runtime_error("wrong-root state cleared sequencer active flag");
        if (preservedStep.presetIndex != 12) throw std::runtime_error("wrong-root state replaced sequencer preset");
        if (preservedStep.chainLength != 4) throw std::runtime_error("wrong-root state replaced sequencer chain");
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
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
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
