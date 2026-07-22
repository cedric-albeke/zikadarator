#include "PluginProcessor.h"
#include "engine/WaveformTap.h"
#include "state/ParameterIDs.h"
#include "ui/components/Knob.h"
#include "ui/components/StepGrid.h"
#include "ui/components/StepCell.h"
#include "ui/components/WaveformDisplay.h"
#include "ui/panels/HeaderPanel.h"
#include "ui/panels/FooterPanel.h"
#include "ui/panels/SidebarPanel.h"
#include "ui/panels/WorkspacePanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

struct ScopedPresetTestDirectory
{
    ScopedPresetTestDirectory()
        : directory(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("zikadarator-preset-tests-" + juce::Uuid().toString()))
    {
        if (directory.createDirectory().failed())
            throw std::runtime_error("failed to create isolated preset test directory");
    }

    ~ScopedPresetTestDirectory()
    {
        directory.deleteRecursively(false);
    }

    juce::File directory;
};

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

zikada::Knob* findLaneMixKnob(zikada::StepGrid& grid, int lane)
{
    int knobIndex = 0;
    for (int childIndex = 0; childIndex < grid.getNumChildComponents(); ++childIndex)
    {
        if (auto* knob = dynamic_cast<zikada::Knob*>(grid.getChildComponent(childIndex)))
        {
            if (knobIndex == lane)
                return knob;
            ++knobIndex;
        }
    }

    return nullptr;
}

juce::MouseEvent makeMouseEvent(juce::Component& component,
                                juce::Point<float> position,
                                juce::Point<float> mouseDownPosition,
                                juce::ModifierKeys modifiers,
                                bool wasDragged,
                                int numberOfClicks = 1)
{
    const auto eventTime = juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(),
            position,
            modifiers,
            1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            &component,
            &component,
            eventTime,
            mouseDownPosition,
            eventTime,
            numberOfClicks,
            wasDragged};
}

juce::Point<float> stepGridCellCentre(zikada::StepGrid& grid, int lane, int step)
{
    auto* cell = grid.getCell(lane, step);
    if (cell == nullptr)
        throw std::runtime_error("step-grid test could not resolve its target cell");
    return cell->getBounds().getCentre().toFloat();
}

class ParameterGestureProbe final : public juce::AudioProcessorParameter::Listener
{
public:
    void parameterValueChanged(int, float) override {}

    void parameterGestureChanged(int, bool gestureIsStarting) override
    {
        if (gestureIsStarting)
            ++beginCount;
        else
            ++endCount;
    }

    int beginCount{0};
    int endCount{0};
};

class TestPlayHead final : public juce::AudioPlayHead
{
public:
    void setPosition(double newBpm,
                     double newPpqPosition,
                     bool playing,
                     juce::Optional<int64_t> newTimeInSamples = juce::nullopt)
    {
        bpm = newBpm;
        ppqPosition = newPpqPosition;
        isPlaying = playing;
        timeInSamples = newTimeInSamples;
    }

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo position;
        position.setBpm(bpm);
        position.setPpqPosition(ppqPosition);
        position.setIsPlaying(isPlaying);
        position.setTimeInSamples(timeInSamples);
        return position;
    }

private:
    double bpm{120.0};
    double ppqPosition{0.0};
    bool isPlaying{true};
    juce::Optional<int64_t> timeInSamples;
};

void configureHostSliceStep(zikada::PluginProcessor& processor, int stepIndex, int chainLength = 1)
{
    setParameter(processor, zikada::ParameterIDs::clockSource, 0.0f);
    setParameter(processor, zikada::ParameterIDs::stepResolution, 1.0f);
    setParameter(processor, zikada::ParameterIDs::dryWet, 100.0f);

    zikada::StepData step;
    step.active = true;
    step.presetIndex = 5;
    step.chainLength = chainLength;
    processor.getSequencerState().setStepData(0, stepIndex, step);
    setParameter(processor, zikada::getStepActiveID(0, stepIndex), 1.0f);
}

void processHostBlock(zikada::PluginProcessor& processor,
                      TestPlayHead& playHead,
                      double bpm,
                      double ppq,
                      bool playing,
                      juce::Optional<int64_t> timeInSamples = juce::nullopt,
                      int blockSize = 64)
{
    playHead.setPosition(bpm, ppq, playing, timeInSamples);
    juce::AudioBuffer<float> buffer(2, blockSize);
    fillBuffer(buffer, 0.25f);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
}

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
    tests.push_back({"continuous host blocks trigger once before a same-step sample-position loop", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;
        constexpr double ppqPerSample = 2.0 / sampleRate;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, blockSize);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        processHostBlock(processor, playHead, 120.0, blockSize * ppqPerSample, true, int64_t{blockSize});
        if (processor.getLaneOnsetCountForTesting(0) != 1)
            throw std::runtime_error("continuous sample-position blocks retriggered the same step");

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("same-step sample-position loop did not retrigger");
    }});

    tests.push_back({"PPQ fallback tolerates continuity and detects same-step loops", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;
        constexpr double ppqPerSample = 2.0 / sampleRate;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, blockSize);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true);
        processHostBlock(processor, playHead, 90.0, blockSize * ppqPerSample, true);
        if (processor.getLaneOnsetCountForTesting(0) != 1)
            throw std::runtime_error("continuous PPQ blocks or a boundary tempo change caused a false retrigger");

        processHostBlock(processor, playHead, 90.0, 0.0, true);
        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("same-step PPQ loop did not retrigger");
    }});

    tests.push_back({"PPQ loops retrigger even when host sample time stays monotonic", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, blockSize);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{blockSize});
        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("PPQ loop was hidden by monotonic host sample time");
    }});

    tests.push_back({"large tempo changes cannot hide short PPQ loops", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int longBlockSize = 4096;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, longBlockSize);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0}, longBlockSize);
        processHostBlock(processor, playHead, 300.0, 0.1, true, int64_t{longBlockSize});
        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("tempo allowance masked a short backward PPQ loop");
    }});

    tests.push_back({"complete-pattern host seeks retrigger the same modulo step", []
    {
        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(48000.0, 64);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        processHostBlock(processor, playHead, 120.0, 8.0, true, int64_t{192000});
        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("16-step host seek did not retrigger modulo step zero");
    }});

    tests.push_back({"transport epochs retrigger every onset-driven lane", []
    {
        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(48000.0, 64);
        setParameter(processor, ParameterIDs::clockSource, 0.0f);
        setParameter(processor, ParameterIDs::stepResolution, 1.0f);

        for (const int lane : {0, 1, 4})
        {
            StepData step;
            step.active = true;
            step.presetIndex = 5;
            processor.getSequencerState().setStepData(lane, 0, step);
            setParameter(processor, getStepActiveID(lane, 0), 1.0f);
        }

        auto requireCounts = [&](std::uint64_t expected, const char* context)
        {
            for (const int lane : {0, 1, 4})
                if (processor.getLaneOnsetCountForTesting(lane) != expected)
                    throw std::runtime_error(std::string(context) + " lane " + std::to_string(lane));
        };

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        requireCounts(1, "initial transport onset missing for");

        processHostBlock(processor, playHead, 120.0, 0.0, false, int64_t{0});
        requireCounts(1, "stopped transport consumed onset for");

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        requireCounts(2, "transport restart did not retrigger");

        processHostBlock(processor, playHead, 120.0, 8.0, true, int64_t{192000});
        requireCounts(3, "complete-pattern seek did not retrigger");
    }});

    tests.push_back({"stopped host blocks neither process nor consume onsets", []
    {
        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(48000.0, 64);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, false, int64_t{0});
        if (processor.getLaneOnsetCountForTesting(0) != 0
            || processor.getProcessedSequencerSampleCountForTesting() != 0)
            throw std::runtime_error("stopped callback consumed sequencer onset or playback state");

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        if (processor.getLaneOnsetCountForTesting(0) != 1
            || processor.getProcessedSequencerSampleCountForTesting() != 64)
            throw std::runtime_error("play start did not trigger and process the selected step");

        processHostBlock(processor, playHead, 120.0, 0.0, false, int64_t{0});
        if (processor.getProcessedSequencerSampleCountForTesting() != 64)
            throw std::runtime_error("stopped callback advanced an active sequencer effect");

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("restart at identical host position did not retrigger");
    }});

    tests.push_back({"stopped host keeps output gain and bypass contracts", []
    {
        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(48000.0, 64);
        setParameter(processor, ParameterIDs::clockSource, 0.0f);
        setParameter(processor, ParameterIDs::outputGain, 6.0206f);

        playHead.setPosition(120.0, 0.0, false, int64_t{0});
        juce::AudioBuffer<float> gainedBuffer(2, 64);
        fillBuffer(gainedBuffer, 0.25f);
        juce::MidiBuffer midi;
        processor.processBlock(gainedBuffer, midi);
        requireNear(gainedBuffer.getSample(0, 0), 0.5f, 0.001f,
                    "stopped host bypassed the global output gain stage");

        setParameter(processor, ParameterIDs::bypass, 1.0f);
        juce::AudioBuffer<float> bypassedBuffer(2, 64);
        fillBuffer(bypassedBuffer, 0.25f);
        processor.processBlock(bypassedBuffer, midi);
        requireNear(bypassedBuffer.getSample(0, 0), 0.25f, 0.0001f,
                    "plugin bypass changed stopped live-monitor audio");
    }});

    tests.push_back({"negative host PPQ triggers the normalized absolute step", []
    {
        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(48000.0, 64);
        configureHostSliceStep(processor, 15);

        processHostBlock(processor, playHead, 120.0, -0.25, true, int64_t{-6000});
        if (processor.getCurrentStep() != 15 || processor.getLaneOnsetCountForTesting(0) != 1)
            throw std::runtime_error("negative PPQ did not select and trigger sequencer step 15");
    }});

    tests.push_back({"continuous chained steps keep one absolute onset identity", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;
        constexpr int64_t startSample = 11968;
        constexpr double ppqPerSample = 2.0 / sampleRate;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, blockSize);
        configureHostSliceStep(processor, 0, 2);

        processHostBlock(processor,
                         playHead,
                         120.0,
                         static_cast<double>(startSample) * ppqPerSample,
                         true,
                         startSample,
                         blockSize);
        if (processor.getCurrentStep() != 1 || processor.getLaneOnsetCountForTesting(0) != 1)
            throw std::runtime_error("chain continuation retriggered instead of retaining its root onset");
    }});

    tests.push_back({"preset and resolution changes refresh onset identity in place", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;
        constexpr double ppqPerSample = 2.0 / sampleRate;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, blockSize);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});

        auto changedStep = processor.getSequencerState().getStepData(0, 0);
        changedStep.presetIndex = 6;
        processor.getSequencerState().setStepData(0, 0, changedStep);
        processHostBlock(processor,
                         playHead,
                         120.0,
                         blockSize * ppqPerSample,
                         true,
                         int64_t{blockSize});
        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("in-place preset change did not refresh the onset");

        setParameter(processor, ParameterIDs::stepResolution, 0.0f);
        processHostBlock(processor,
                         playHead,
                         120.0,
                         2.0 * blockSize * ppqPerSample,
                         true,
                         int64_t{2 * blockSize});
        if (processor.getLaneOnsetCountForTesting(0) != 3)
            throw std::runtime_error("in-place step-resolution change did not refresh the onset epoch");
    }});

    tests.push_back({"host and free clock switches refresh onset identity", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;
        constexpr double ppqPerSample = 2.0 / sampleRate;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, blockSize);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        setParameter(processor, ParameterIDs::clockSource, 1.0f);
        processHostBlock(processor,
                         playHead,
                         120.0,
                         blockSize * ppqPerSample,
                         true,
                         int64_t{blockSize});
        setParameter(processor, ParameterIDs::clockSource, 0.0f);
        processHostBlock(processor,
                         playHead,
                         120.0,
                         2.0 * blockSize * ppqPerSample,
                         true,
                         int64_t{2 * blockSize});

        if (processor.getLaneOnsetCountForTesting(0) != 3)
            throw std::runtime_error("host/free clock domain switches did not refresh onset identity");
    }});

    tests.push_back({"state restore requests realtime-safe onset invalidation", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;
        constexpr double ppqPerSample = 2.0 / sampleRate;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, blockSize);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        const auto state = processor.exportFullState();
        processor.applyFullState(state);
        processHostBlock(processor,
                         playHead,
                         120.0,
                         blockSize * ppqPerSample,
                         true,
                         int64_t{blockSize});

        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("state restore did not invalidate onset identity on the audio thread");
    }});

    tests.push_back({"bypassed host seeks retrigger after bypass is released", []
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;
        constexpr double ppqPerSample = 2.0 / sampleRate;

        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(sampleRate, blockSize);
        configureHostSliceStep(processor, 0);

        processHostBlock(processor, playHead, 120.0, 0.0, true, int64_t{0});
        setParameter(processor, ParameterIDs::bypass, 1.0f);
        processHostBlock(processor, playHead, 120.0, 8.0, true, int64_t{192000});
        setParameter(processor, ParameterIDs::bypass, 0.0f);
        processHostBlock(processor,
                         playHead,
                         120.0,
                         8.0 + blockSize * ppqPerSample,
                         true,
                         int64_t{192000 + blockSize});

        if (processor.getLaneOnsetCountForTesting(0) != 2)
            throw std::runtime_error("bypassed host seek was not retained for the next active onset");
    }});

    tests.push_back({"extreme finite host positions remain bounded", []
    {
        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(48000.0, 64);
        configureHostSliceStep(processor, 0);
        setParameter(processor, ParameterIDs::dryWet, 0.0f);

        processHostBlock(processor,
                         playHead,
                         120.0,
                         std::numeric_limits<double>::max(),
                         true,
                         std::numeric_limits<int64_t>::max());
        if (processor.getCurrentStep() < 0 || processor.getCurrentStep() >= 16)
            throw std::runtime_error("extreme host position escaped the sequencer grid");
    }});

    tests.push_back({"invalid host sample rates are normalized before engine prepare", []
    {
        const double invalidRates[] = {
            0.0,
            1.0,
            9.0,
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::infinity()
        };

        for (const double invalidRate : invalidRates)
        {
            PluginProcessor processor;
            processor.prepareToPlay(invalidRate, 64);
            if (processor.getCurrentSampleRate() != 44100.0)
                throw std::runtime_error("invalid sample rate was not normalized before engine prepare");

            setParameter(processor, ParameterIDs::clockSource, 1.0f);
            setParameter(processor, ParameterIDs::dryWet, 100.0f);
            StepData pitchStep;
            pitchStep.active = true;
            pitchStep.presetIndex = 7;
            processor.getSequencerState().setStepData(5, 0, pitchStep);
            setParameter(processor, getStepActiveID(5, 0), 1.0f);

            juce::AudioBuffer<float> buffer(2, 64);
            fillBuffer(buffer, 0.25f);
            juce::MidiBuffer midi;
            processor.processBlock(buffer, midi);
            sumFiniteAbsoluteSamples(buffer, 0, "normalized sample-rate pitch render");
        }

        PluginProcessor validProcessor;
        validProcessor.prepareToPlay(96000.0, 64);
        if (validProcessor.getCurrentSampleRate() != 96000.0)
            throw std::runtime_error("valid sample rate was unexpectedly replaced");
    }});

    tests.push_back({"host offline blocks preserve every pattern-cycle onset", []
    {
        constexpr int blockSize = 4097;
        TestPlayHead playHead;
        PluginProcessor processor;
        processor.setPlayHead(&playHead);
        processor.prepareToPlay(64.0, blockSize);
        configureHostSliceStep(processor, 0);
        setParameter(processor, ParameterIDs::stepResolution, 0.0f);
        setParameter(processor, ParameterIDs::dryWet, 0.0f);

        processHostBlock(processor, playHead, 300.0, 0.0, true, int64_t{0}, blockSize);
        if (processor.getLaneOnsetCountForTesting(0) != 81)
            throw std::runtime_error("scheduler segment cap collapsed offline pattern-cycle onsets: "
                                     + std::to_string(processor.getLaneOnsetCountForTesting(0)));
    }});

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
        PluginProcessor binaryRoundTripProcessor;
        const auto& presets = processor.getPresetManager().getItems();
        int factoryPresetCount = 0;
        int initPresetCount = 0;
        int patternedFactoryPresets = 0;
        std::unordered_set<std::string> factoryNames;
        std::unordered_set<std::string> factoryCategories;
        std::unordered_set<std::string> serializedFactoryStates;
        std::unordered_set<std::string> expectedParameterIds;
        int processorParameterCount = 0;

        const auto processorState = processor.getPluginState().getValueTreeState().copyState();
        for (int childIndex = 0; childIndex < processorState.getNumChildren(); ++childIndex)
        {
            const auto child = processorState.getChild(childIndex);
            if (child.hasType("PARAM"))
            {
                ++processorParameterCount;
                if (!expectedParameterIds.insert(child.getProperty("id").toString().toStdString()).second)
                    throw std::runtime_error("processor parameter schema contains duplicate ids");
            }
        }
        if (processorParameterCount != 121 || expectedParameterIds.size() != 121)
            throw std::runtime_error("processor parameter schema contains "
                                     + std::to_string(expectedParameterIds.size())
                                     + " ids, expected 121");

        for (int presetIndex = 0; presetIndex < static_cast<int>(presets.size()); ++presetIndex)
        {
            const auto& preset = presets[static_cast<size_t>(presetIndex)];
            if (!preset.isFactory)
                continue;

            ++factoryPresetCount;
            if (preset.name.isEmpty() || preset.category.isEmpty() || preset.subtitle.isEmpty())
                throw std::runtime_error("factory preset has incomplete browser metadata");
            if (preset.name != preset.name.toUpperCase())
                throw std::runtime_error("factory preset name is not normalized: " + preset.name.toStdString());
            if (!factoryNames.insert(preset.name.toStdString()).second)
                throw std::runtime_error("duplicate factory preset name: " + preset.name.toStdString());
            factoryCategories.insert(preset.category.toStdString());

            juce::ValueTree loadedState;
            if (!processor.getPresetManager().loadPreset(presetIndex, loadedState))
                throw std::runtime_error("factory preset could not be loaded: " + preset.name.toStdString());
            if (!serializedFactoryStates.insert(loadedState.toXmlString().toStdString()).second)
                throw std::runtime_error("factory preset duplicates another serialized state: "
                                         + preset.name.toStdString());

            std::unordered_set<std::string> loadedParameterIds;
            int loadedParameterCount = 0;
            int loadedSequencerCount = 0;
            for (int childIndex = 0; childIndex < loadedState.getNumChildren(); ++childIndex)
            {
                const auto child = loadedState.getChild(childIndex);
                if (child.hasType("PARAM"))
                {
                    ++loadedParameterCount;
                    if (!loadedParameterIds.insert(child.getProperty("id").toString().toStdString()).second)
                        throw std::runtime_error("factory preset has duplicate APVTS ids: "
                                                 + preset.name.toStdString());
                }
                else if (child.hasType("SequencerState"))
                {
                    ++loadedSequencerCount;
                }
                else
                {
                    throw std::runtime_error("factory preset has an unexpected state child: "
                                             + preset.name.toStdString());
                }
            }
            if (loadedParameterCount != 121 || loadedParameterIds != expectedParameterIds
                || loadedSequencerCount != 1 || loadedState.getNumChildren() != 122)
                throw std::runtime_error("factory preset APVTS schema is incomplete: "
                                         + preset.name.toStdString());

            const auto sequencer = loadedState.getChildWithName("SequencerState");
            if (!sequencer.isValid())
                throw std::runtime_error("factory preset has no sequencer state: " + preset.name.toStdString());
            if (sequencer.getNumChildren() != SequencerState::NumLanes)
                throw std::runtime_error("factory preset has an invalid lane count: " + preset.name.toStdString());

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
                const auto laneTree = sequencer.getChild(lane);
                if (!laneTree.hasType("Lane")
                    || static_cast<int>(laneTree.getProperty("index", -1)) != lane)
                    throw std::runtime_error("factory preset has an invalid lane index: "
                                             + preset.name.toStdString() + " lane " + std::to_string(lane));
                const auto steps = laneTree.getChildWithName("Steps");
                const auto slots = laneTree.getChildWithName("UserSlots");
                if (steps.getNumChildren() != SequencerState::NumSteps
                    || slots.getNumChildren() != SequencerState::NumUserSlots)
                    throw std::runtime_error("factory preset has an incomplete sequencer lane: "
                                             + preset.name.toStdString() + " lane " + std::to_string(lane));

                for (int step = 0; step < SequencerState::NumSteps; ++step)
                {
                    const auto stepTree = steps.getChild(step);
                    if (!stepTree.hasType("Step")
                        || static_cast<int>(stepTree.getProperty("index", -1)) != step)
                        throw std::runtime_error("factory preset has an invalid step index: "
                                                 + preset.name.toStdString() + " lane "
                                                 + std::to_string(lane) + " step " + std::to_string(step));
                    const bool sequencerActive = static_cast<bool>(stepTree.getProperty("active", false));
                    const int lanePresetIndex = static_cast<int>(stepTree.getProperty("presetIndex", 0));
                    const int chainLength = static_cast<int>(stepTree.getProperty("chainLength", 1));
                    const bool parameterActive = getParameterStateValue(loadedState, getStepActiveID(lane, step)) > 0.5f;
                    if (sequencerActive != parameterActive)
                        throw std::runtime_error("factory preset has divergent step state: "
                                                 + preset.name.toStdString() + " lane "
                                                 + std::to_string(lane) + " step " + std::to_string(step));
                    if (sequencerActive && (lanePresetIndex < 5 || lanePresetIndex > 20))
                        throw std::runtime_error("factory preset uses an invalid lane preset index: "
                                                 + preset.name.toStdString() + " lane "
                                                 + std::to_string(lane) + " step " + std::to_string(step)
                                                 + " preset " + std::to_string(lanePresetIndex));
                    if (chainLength < 1 || chainLength > SequencerState::NumSteps - step)
                        throw std::runtime_error("factory preset has an invalid step chain: "
                                                 + preset.name.toStdString() + " lane "
                                                 + std::to_string(lane) + " step " + std::to_string(step));
                    hasActiveStep = hasActiveStep || sequencerActive;
                }

                for (int slot = 0; slot < SequencerState::NumUserSlots; ++slot)
                {
                    const auto slotTree = slots.getChild(slot);
                    if (!slotTree.hasType("UserSlot")
                        || static_cast<int>(slotTree.getProperty("index", -1)) != slot)
                        throw std::runtime_error("factory preset has an invalid user-slot index: "
                                                 + preset.name.toStdString() + " lane "
                                                 + std::to_string(lane) + " slot " + std::to_string(slot));
                    const auto data = UserSlotData::fromValueTree(slotTree);
                    if (!std::isfinite(data.filterCutoff) || data.filterCutoff < 20.0f || data.filterCutoff > 20000.0f
                        || !std::isfinite(data.filterResonance) || data.filterResonance < 0.1f || data.filterResonance > 20.0f
                        || !std::isfinite(data.delayTime) || data.delayTime < 0.0f || data.delayTime > 12.0f
                        || !std::isfinite(data.delayFeedback) || data.delayFeedback < 0.0f || data.delayFeedback > 0.99f
                        || !std::isfinite(data.delayMix) || data.delayMix < 0.0f || data.delayMix > 1.0f
                        || !std::isfinite(data.volume) || data.volume < 0.0f || data.volume > 2.0f
                        || !std::isfinite(data.pan) || data.pan < -1.0f || data.pan > 1.0f)
                        throw std::runtime_error("factory preset has invalid user-slot data: "
                                                 + preset.name.toStdString() + " lane "
                                                 + std::to_string(lane) + " slot " + std::to_string(slot));
                }
            }

            if (hasActiveStep)
                ++patternedFactoryPresets;
            if (preset.name == "INIT")
            {
                ++initPresetCount;
                if (hasActiveStep)
                    throw std::runtime_error("INIT factory preset must not contain active steps");
            }
            else if (!hasActiveStep)
            {
                throw std::runtime_error("factory preset has no active pattern: " + preset.name.toStdString());
            }

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

            const auto firstExport = processor.exportFullState();
            processor.applyFullState(firstExport);
            const auto secondExport = processor.exportFullState();
            if (!firstExport.isEquivalentTo(secondExport))
                throw std::runtime_error("factory preset failed full-state roundtrip: "
                                         + preset.name.toStdString());

            juce::MemoryBlock binaryState;
            processor.getStateInformation(binaryState);
            binaryRoundTripProcessor.setStateInformation(binaryState.getData(),
                                                          static_cast<int>(binaryState.getSize()));
            const auto binaryExport = binaryRoundTripProcessor.exportFullState();
            if (!firstExport.isEquivalentTo(binaryExport))
                throw std::runtime_error("factory preset failed binary host-state roundtrip: "
                                         + preset.name.toStdString());
        }

        if (factoryPresetCount != 50)
            throw std::runtime_error("factory preset count is " + std::to_string(factoryPresetCount)
                                     + ", expected 50");
        if (initPresetCount != 1)
            throw std::runtime_error("factory bank must contain exactly one INIT preset");
        if (patternedFactoryPresets != 49)
            throw std::runtime_error("factory bank contains " + std::to_string(patternedFactoryPresets)
                                     + " patterned presets, expected 49");
        if (factoryCategories.size() < 8)
            throw std::runtime_error("factory bank does not provide enough browser categories");
    }});

    tests.push_back({"preset library identity and browser selection remain stable", []
    {
        ScopedPresetTestDirectory storage;
        PresetManager manager(storage.directory);

        std::vector<juce::String> initialFactoryIds;
        std::unordered_set<std::string> uniqueIds;
        for (const auto& item : manager.getItems())
        {
            if (!item.isFactory)
                continue;
            if (item.id.isEmpty())
                throw std::runtime_error("factory preset is missing a stable id");
            if (!uniqueIds.insert(item.id.toStdString()).second)
                throw std::runtime_error("factory preset id is not unique");
            initialFactoryIds.push_back(item.id);
        }
        if (initialFactoryIds.size() != 50)
            throw std::runtime_error("isolated preset library did not expose 50 factory presets");

        manager.markPresetUsed("SPACE BLOOM");
        manager.refresh();
        manager.toggleFavorite("NEON GATE");

        std::vector<juce::String> refreshedFactoryIds;
        for (const auto& item : manager.getItems())
            if (item.isFactory)
                refreshedFactoryIds.push_back(item.id);
        if (refreshedFactoryIds != initialFactoryIds)
            throw std::runtime_error("recent/favorite metadata changed factory navigation order");

        const int initIndex = manager.findItemIndexById("factory:INIT");
        if (initIndex < 0)
            throw std::runtime_error("stable factory id lookup failed");

        juce::ValueTree initState;
        if (!manager.loadPreset(initIndex, initState))
            throw std::runtime_error("failed to load INIT for user-preset identity test");
        if (manager.saveUserPreset("INIT", initState) != PresetManager::SaveResult::NameConflict)
            throw std::runtime_error("user preset was allowed to collide with a factory name");
        if (manager.saveUserPreset("A B", initState) != PresetManager::SaveResult::Saved)
            throw std::runtime_error("valid isolated user preset could not be saved");
        const auto updateResult = manager.saveUserPreset("A_B", initState);
        if (updateResult != PresetManager::SaveResult::Updated)
            throw std::runtime_error("existing user preset could not be deliberately updated, result="
                                     + std::to_string(static_cast<int>(updateResult)));
        if (manager.saveUserPreset("---", initState) != PresetManager::SaveResult::InvalidName)
            throw std::runtime_error("delimiter-only user preset name was accepted");
        if (manager.saveUserPreset("WRONG ROOT", juce::ValueTree("WrongRoot")) != PresetManager::SaveResult::InvalidState)
            throw std::runtime_error("wrong-root user preset was accepted");

        auto incompleteState = initState.createCopy();
        incompleteState.removeChild(incompleteState.getChildWithName("SequencerState"), nullptr);
        if (manager.saveUserPreset("INCOMPLETE", incompleteState) != PresetManager::SaveResult::InvalidState)
            throw std::runtime_error("incomplete correct-root user preset was accepted");

        auto outOfRangeState = initState.createCopy();
        setParameterStateValue(outOfRangeState, ParameterIDs::tempo, 999.0f);
        if (manager.saveUserPreset("BAD TEMPO", outOfRangeState) != PresetManager::SaveResult::InvalidState)
            throw std::runtime_error("out-of-range user preset parameter was accepted");

        auto duplicateParameterState = initState.createCopy();
        duplicateParameterState.addChild(findParameterState(duplicateParameterState, ParameterIDs::tempo).createCopy(),
                                         -1, nullptr);
        if (manager.saveUserPreset("DUPLICATE PARAM", duplicateParameterState) != PresetManager::SaveResult::InvalidState)
            throw std::runtime_error("duplicate user preset parameter was accepted");

        const auto legacyCollisionFile = storage.directory.getChildFile("WOBBLE_BUS.xml");
        const auto legacyXml = initState.createXml();
        if (legacyXml == nullptr || !legacyXml->writeTo(legacyCollisionFile))
            throw std::runtime_error("failed to create legacy collision fixture");
        manager.refresh();

        bool foundLegacyCollision = false;
        uniqueIds.clear();
        for (const auto& item : manager.getItems())
        {
            if (!uniqueIds.insert(item.id.toStdString()).second)
                throw std::runtime_error("preset library contains duplicate stable ids");
            if (!item.isFactory && item.file == legacyCollisionFile)
            {
                foundLegacyCollision = item.name == "WOBBLE BUS (USER)"
                    && item.id == "user-file:WOBBLE_BUS.XML";
            }
        }
        if (!foundLegacyCollision)
            throw std::runtime_error("legacy user preset colliding with a factory name was hidden");

        WorkspacePanel browser;
        std::vector<PresetManager::PresetItem> factoryItems;
        for (const auto& item : manager.getItems())
            if (item.isFactory)
                factoryItems.push_back(item);
        browser.setPresetItems(factoryItems);
        if (browser.getVisiblePresetCountForTesting() != 50)
            throw std::runtime_error("browser did not expose the complete factory bank");

        browser.setPresetCategoryForTesting("Filter");
        if (browser.getVisiblePresetCountForTesting() != 8)
            throw std::runtime_error("Filter category did not expose the expected eight presets");
        browser.setPresetCategoryForTesting("ALL CATEGORIES");
        browser.setPresetSearchForTesting("wobble");
        if (browser.getVisiblePresetCountForTesting() != 1)
            throw std::runtime_error("case-insensitive preset search did not isolate WOBBLE BUS");
        browser.setPresetSearchForTesting("wobble motion");
        if (browser.getVisiblePresetCountForTesting() != 1)
            throw std::runtime_error("multi-token preset search did not combine name and category");

        browser.setPresetSearchForTesting({});
        browser.selectPresetByIdForTesting("factory:SPACE BLOOM");
        if (browser.getSelectedPresetIdForTesting() != "factory:SPACE BLOOM")
            throw std::runtime_error("browser could not select a preset by stable id");
        std::rotate(factoryItems.begin(), factoryItems.begin() + 7, factoryItems.end());
        browser.setPresetItems(factoryItems);
        if (browser.getSelectedPresetIdForTesting() != "factory:SPACE BLOOM")
            throw std::runtime_error("browser selection changed after library reorder");

        WorkspacePanel completeBrowser;
        completeBrowser.setPresetItems(manager.getItems());
        if (completeBrowser.getVisiblePresetCountForTesting() != 52)
            throw std::runtime_error("browser all-source filter omitted presets");
        completeBrowser.setPresetSourceForTesting("FACTORY");
        if (completeBrowser.getVisiblePresetCountForTesting() != 50)
            throw std::runtime_error("browser factory filter returned the wrong count");
        completeBrowser.setPresetSourceForTesting("USER");
        if (completeBrowser.getVisiblePresetCountForTesting() != 2)
            throw std::runtime_error("browser user filter returned the wrong count");
        completeBrowser.setPresetSourceForTesting("FAVORITES");
        if (completeBrowser.getVisiblePresetCountForTesting() != 1)
            throw std::runtime_error("browser favorites filter returned the wrong count");
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

    tests.push_back({"hidden waveform consumers discard stale tap samples", []
    {
        WaveformTap tap;
        tap.prepare(16);

        const std::array<float, 8> staleSamples{1.0f, 2.0f, 3.0f, 4.0f,
                                                 5.0f, 6.0f, 7.0f, 8.0f};
        tap.pushFromAudioThread(staleSamples.data(), static_cast<int>(staleSamples.size()));
        if (tap.discardAllForUi() != static_cast<int>(staleSamples.size()))
            throw std::runtime_error("waveform tap did not discard every stale sample");

        std::array<float, 8> destination{};
        if (tap.popForUi(destination.data(), static_cast<int>(destination.size())) != 0)
            throw std::runtime_error("discarded waveform samples remained readable");

        const std::array<float, 4> freshSamples{11.0f, 12.0f, 13.0f, 14.0f};
        tap.pushFromAudioThread(freshSamples.data(), static_cast<int>(freshSamples.size()));
        const int copied = tap.popForUi(destination.data(), static_cast<int>(destination.size()));
        if (copied != static_cast<int>(freshSamples.size()))
            throw std::runtime_error("waveform tap lost fresh samples after discarding stale data");
        for (int i = 0; i < copied; ++i)
            requireNear(destination[static_cast<size_t>(i)], freshSamples[static_cast<size_t>(i)], 0.001f,
                        "waveform tap returned stale data after discard");
    }});

    tests.push_back({"waveform tap tolerates reprepare during UI consumption", []
    {
        WaveformTap tap;
        tap.prepare(128);
        std::atomic<bool> keepConsuming{true};
        std::atomic<bool> reprepareInProgress{false};
        std::atomic<int> consumerIterations{0};
        std::atomic<int> producerIterations{0};
        std::atomic<int> consumerIterationsDuringReprepare{0};
        std::atomic<int> producerIterationsDuringReprepare{0};

        std::thread consumer([&]
        {
            std::array<float, 32> destination{};
            while (keepConsuming.load())
            {
                tap.popForUi(destination.data(), static_cast<int>(destination.size()));
                tap.discardAllForUi();
                consumerIterations.fetch_add(1);
                if (reprepareInProgress.load())
                    consumerIterationsDuringReprepare.fetch_add(1);
            }
        });

        const std::array<float, 32> samples{0.25f};
        std::thread producer([&]
        {
            while (keepConsuming.load())
            {
                tap.pushFromAudioThread(samples.data(), static_cast<int>(samples.size()));
                producerIterations.fetch_add(1);
                if (reprepareInProgress.load())
                    producerIterationsDuringReprepare.fetch_add(1);
            }
        });

        auto waitForIterations = [&](int minimumConsumerIterations, int minimumProducerIterations)
        {
            const double deadline = juce::Time::getMillisecondCounterHiRes() + 2000.0;
            while ((consumerIterations.load() < minimumConsumerIterations
                    || producerIterations.load() < minimumProducerIterations)
                   && juce::Time::getMillisecondCounterHiRes() < deadline)
                std::this_thread::yield();

            return consumerIterations.load() >= minimumConsumerIterations
                && producerIterations.load() >= minimumProducerIterations;
        };

        if (!waitForIterations(100, 100))
        {
            keepConsuming = false;
            consumer.join();
            producer.join();
            throw std::runtime_error("waveform reprepare test could not start producer and consumer work");
        }

        const auto initialGeneration = tap.getGeneration();
        reprepareInProgress = true;
        for (int iteration = 0; iteration < 500; ++iteration)
        {
            tap.prepare(64 + iteration % 193);
            if (iteration % 10 == 0)
                std::this_thread::yield();
        }
        reprepareInProgress = false;

        keepConsuming = false;
        consumer.join();
        producer.join();

        if (consumerIterationsDuringReprepare.load() <= 0
            || producerIterationsDuringReprepare.load() <= 0)
            throw std::runtime_error("waveform reprepare test did not overlap producer and consumer work");
        if (tap.getGeneration() != initialGeneration + 500)
            throw std::runtime_error("waveform tap generation did not track every reprepare");

        tap.reset();
        const std::array<float, 4> freshSamples{21.0f, 22.0f, 23.0f, 24.0f};
        tap.pushFromAudioThread(freshSamples.data(), static_cast<int>(freshSamples.size()));
        std::array<float, 4> destination{};
        const int copied = tap.popForUi(destination.data(), static_cast<int>(destination.size()));
        if (copied != static_cast<int>(freshSamples.size()))
            throw std::runtime_error("waveform tap stopped working after concurrent reprepare");
        for (int i = 0; i < copied; ++i)
            requireNear(destination[static_cast<size_t>(i)], freshSamples[static_cast<size_t>(i)], 0.001f,
                        "waveform tap corrupted data after concurrent reprepare");
    }});

    tests.push_back({"waveform display clears retained history when hidden", []
    {
        auto display = std::make_unique<WaveformDisplay>();
        const std::array<float, 4> samples{0.25f, -0.5f, 0.75f, -1.0f};
        display->pushInputSamples(samples.data(), static_cast<int>(samples.size()));
        display->pushOutputSamples(samples.data(), static_cast<int>(samples.size()));
        if (!display->hasRetainedSamples())
            throw std::runtime_error("waveform display did not retain pushed samples");

        display->clearHistory();
        if (display->hasRetainedSamples())
            throw std::runtime_error("waveform display retained stale history after being cleared");
    }});

    tests.push_back({"CRT monitor advances independently of state changes", []
    {
        HeaderPanel header;
        int animationFrames = 0;
        header.onFxDisplayAnimationFrameForTesting = [&animationFrames] { ++animationFrames; };

        juce::MessageManager::getInstance()->runDispatchLoopUntil(150);
        for (int attempt = 0; animationFrames < 2 && attempt < 3; ++attempt)
        {
            juce::Thread::sleep(40);
            juce::Timer::callPendingTimersSynchronously();
        }
        if (animationFrames < 2)
            throw std::runtime_error("CRT monitor did not produce autonomous animation frames");
    }});

    tests.push_back({"custom-drawn header actions expose stable accessible names and focus order", []
    {
        HeaderPanel header;
        header.setSize(1200, 60);
        header.setUndoEnabled(true);
        header.setRedoEnabled(true);
        header.setPresetStepEnabled(true, true);
        int buttonCount = 0;

        for (int childIndex = 0; childIndex < header.getNumChildComponents(); ++childIndex)
        {
            auto* button = dynamic_cast<juce::Button*>(header.getChildComponent(childIndex));
            if (button == nullptr)
                continue;

            ++buttonCount;
            if (button->getTitle().isEmpty())
                throw std::runtime_error("custom-drawn header button has no accessible name");
            if (button->getHelpText().isEmpty() && button->getTooltip().isEmpty())
                throw std::runtime_error("custom-drawn header button has no accessible help");
            if (button->isEnabled() && !button->getWantsKeyboardFocus())
                throw std::runtime_error("custom-drawn header button cannot receive keyboard focus");
            if (button->getExplicitFocusOrder() <= 0)
                throw std::runtime_error("custom-drawn header button has no deterministic focus order");
        }

        if (buttonCount != 8)
            throw std::runtime_error("header accessibility test did not inspect every action");

        juce::Component root;
        root.setSize(1300, 100);
        root.addAndMakeVisible(header);
        KeyboardTextButton trailingAction("TRAILING");
        trailingAction.setTitle("Trailing action");
        trailingAction.setBounds(1210, 10, 80, 30);
        root.addAndMakeVisible(trailingAction);

        juce::KeyboardFocusTraverser traverser;
        const auto focusables = traverser.getAllComponents(&root);
        int reachableHeaderActions = 0;
        bool trailingActionReachable = false;
        for (auto* component : focusables)
        {
            if (component == &trailingAction)
                trailingActionReachable = true;
            else if (component != nullptr && component->getParentComponent() == &header)
                ++reachableHeaderActions;
        }
        if (reachableHeaderActions != 8 || !trailingActionReachable)
            throw std::runtime_error("global Tab traversal cannot enter and leave all header actions");

        int pageSelections = 0;
        header.onPageSelected = [&pageSelections](HeaderPanel::Page) { ++pageSelections; };
        for (int childIndex = 0; childIndex < header.getNumChildComponents(); ++childIndex)
        {
            auto* button = dynamic_cast<juce::Button*>(header.getChildComponent(childIndex));
            if (button == nullptr || button->getTitle() != "Presets")
                continue;

            juce::Component& control = *button;
            if (control.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey)))
                throw std::runtime_error("header action consumed the host transport key");
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
            if (!control.keyPressed(juce::KeyPress(juce::KeyPress::returnKey)))
                throw std::runtime_error("header action did not handle Enter");
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        }
        if (pageSelections != 1)
            throw std::runtime_error("header Enter activation invoked "
                                     + std::to_string(pageSelections) + " commands instead of one");
    }});

    tests.push_back({"interactive controls pass Space through to the host", []
    {
        KeyboardTextButton textButton{"BUTTON"};
        HostSafeToggleButton toggleButton{"TOGGLE"};
        HostSafeTextEditor textEditor;
        HostSafeComboBox comboBox;
        HostSafeSlider slider;
        HostSafeListBox listBox;
        std::array<juce::Component*, 6> controls{
            &textButton, &toggleButton, &textEditor, &comboBox, &slider, &listBox
        };

        int clicks = 0;
        textButton.onClick = [&clicks] { ++clicks; };
        textEditor.setText("UNCHANGED", false);
        const auto space = juce::KeyPress(juce::KeyPress::spaceKey);
        for (auto* control : controls)
            if (control->keyPressed(space))
                throw std::runtime_error("plugin control consumed the host transport key");

        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        if (clicks != 0 || textEditor.getText() != "UNCHANGED")
            throw std::runtime_error("Space changed plugin state instead of reaching the host");
    }});

    tests.push_back({"preset palette exposes named keyboard actions", []
    {
        SidebarPanel sidebar;
        sidebar.setSize(280, 600);
        StepData selected;
        selected.active = true;
        selected.presetIndex = 5;
        sidebar.setSelectedStep(0, 0, selected);

        int buttonCount = 0;
        for (int childIndex = 0; childIndex < sidebar.getNumChildComponents(); ++childIndex)
        {
            auto* button = dynamic_cast<juce::Button*>(sidebar.getChildComponent(childIndex));
            if (button == nullptr)
                continue;

            ++buttonCount;
            if (button->getTitle().isEmpty())
                throw std::runtime_error("preset button has no accessible name");
            if (button->getDescription().isEmpty())
                throw std::runtime_error("preset button has no accessible description");
            if (button->getExplicitFocusOrder() <= 0)
                throw std::runtime_error("preset button has no deterministic focus order");
        }

        if (buttonCount != 20)
            throw std::runtime_error("preset accessibility test did not inspect the full palette");

        int assignedPreset = -1;
        sidebar.onPresetAssigned = [&assignedPreset](int, int, int preset) { assignedPreset = preset; };
        if (!sidebar.keyPressed(juce::KeyPress(juce::KeyPress::rightKey)))
            throw std::runtime_error("preset palette did not support arrow navigation");
        if (sidebar.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey)) || assignedPreset != -1)
            throw std::runtime_error("preset palette consumed Space or assigned a preset from it");
        if (!sidebar.keyPressed(juce::KeyPress(juce::KeyPress::returnKey)))
            throw std::runtime_error("preset palette did not support Enter activation");
        if (assignedPreset != 6)
            throw std::runtime_error("preset palette activated the old focus target after arrow navigation");
    }});

    tests.push_back({"knob exposes a ranged accessible value and complete keyboard gestures", []
    {
        Knob knob;
        knob.setLabel("MIX");
        knob.setRange(0.0, 100.0);
        knob.setValue(50.0);

        auto handler = knob.createAccessibilityHandler();
        if (handler == nullptr || handler->getRole() != juce::AccessibilityRole::slider)
            throw std::runtime_error("knob does not expose the slider accessibility role");
        if (handler->getTitle() != "MIX")
            throw std::runtime_error("knob accessible name does not match its visible label");

        auto* valueInterface = handler->getValueInterface();
        if (valueInterface == nullptr)
            throw std::runtime_error("knob has no accessibility value interface");
        const auto range = valueInterface->getRange();
        if (!range.isValid() || range.getMinimumValue() != 0.0 || range.getMaximumValue() != 100.0)
            throw std::runtime_error("knob accessibility range does not match the control range");

        int gestureStarts = 0;
        int gestureEnds = 0;
        knob.onDragStart = [&gestureStarts] { ++gestureStarts; };
        knob.onDragEnd = [&gestureEnds] { ++gestureEnds; };

        if (!knob.keyPressed(juce::KeyPress(juce::KeyPress::upKey)))
            throw std::runtime_error("knob did not handle the up arrow");
        requireNear(static_cast<float>(knob.getValue()), 51.0f, 0.001f,
                    "knob keyboard increment was not one percent of its range");

        valueInterface->setValue(75.0);
        requireNear(static_cast<float>(knob.getValue()), 75.0f, 0.001f,
                    "knob accessibility setter did not update the value");
        knob.setRange(0.0, 1.0);
        knob.setDisplayMode(KnobDisplayMode::Percent);
        valueInterface->setValueAsString("25%");
        requireNear(static_cast<float>(knob.getValue()), 0.25f, 0.001f,
                    "knob accessibility text setter ignored displayed percent units");
        if (gestureStarts != 3 || gestureEnds != 3)
            throw std::runtime_error("knob keyboard/accessibility changes emitted incomplete gestures");
    }});

    tests.push_back({"sequencer grid exposes its keyboard interaction model", []
    {
        PluginProcessor processor;
        StepGrid grid(processor.getPluginState().getValueTreeState(), processor.getSequencerState());
        auto handler = grid.createAccessibilityHandler();

        if (handler == nullptr || handler->getRole() != juce::AccessibilityRole::group)
            throw std::runtime_error("sequencer grid does not expose a grouped accessibility role");
        if (handler->getTitle().isEmpty() || handler->getDescription().isEmpty()
            || handler->getHelp().isEmpty())
            throw std::runtime_error("sequencer grid accessibility guidance is incomplete");
        if (!handler->getHelp().contains("Page Up") || !handler->getHelp().contains("Shift+Right"))
            throw std::runtime_error("sequencer grid does not announce its advanced keyboard commands");

        grid.setSelectedStep(0, 0);
        const auto before = processor.getSequencerState().getStepData(0, 0);
        if (grid.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey)))
            throw std::runtime_error("sequencer handled a child-bubbled key without owning focus");
        const auto after = processor.getSequencerState().getStepData(0, 0);
        if (after.active != before.active || after.presetIndex != before.presetIndex)
            throw std::runtime_error("child-bubbled key modified the selected sequencer step");
        if (grid.handleKeyCommandForTesting(juce::KeyPress(juce::KeyPress::spaceKey)))
            throw std::runtime_error("sequencer consumed Space while owning keyboard focus");
        const auto afterSpace = processor.getSequencerState().getStepData(0, 0);
        if (afterSpace.active != before.active || afterSpace.presetIndex != before.presetIndex)
            throw std::runtime_error("Space modified the selected sequencer step");
        if (!grid.handleKeyCommandForTesting(juce::KeyPress(juce::KeyPress::returnKey)))
            throw std::runtime_error("sequencer did not retain Enter activation");
        if (processor.getSequencerState().getStepData(0, 0).active == before.active)
            throw std::runtime_error("Enter did not toggle the selected sequencer step");
    }});

    tests.push_back({"sequencer grid keyboard commands cycle presets and resize ties", []
    {
        PluginProcessor processor;
        auto& state = processor.getSequencerState();

        StepData source;
        source.active = true;
        source.presetIndex = 1;
        state.setStepData(1, 2, source);
        setParameter(processor, getStepActiveID(1, 2), 1.0f);

        StepData blocker;
        blocker.active = true;
        blocker.presetIndex = 7;
        state.setStepData(1, 4, blocker);
        setParameter(processor, getStepActiveID(1, 4), 1.0f);

        StepGrid grid(processor.getPluginState().getValueTreeState(), state);
        grid.setSelectedStep(1, 2);

        int undoBoundaries = 0;
        int presetChanges = 0;
        int chainChanges = 0;
        grid.onStepPresetEditStarting = [&undoBoundaries] { ++undoBoundaries; };
        grid.onStepPresetChanged = [&presetChanges](int, int, int) { ++presetChanges; };
        grid.onChainChanged = [&chainChanges](int, int, int) { ++chainChanges; };

        if (!grid.handleKeyCommandForTesting(juce::KeyPress(juce::KeyPress::pageUpKey))
            || state.getStepData(1, 2).presetIndex != 2)
            throw std::runtime_error("Page Up did not advance the selected step preset");
        if (!grid.handleKeyCommandForTesting(juce::KeyPress(juce::KeyPress::pageDownKey))
            || state.getStepData(1, 2).presetIndex != 1)
            throw std::runtime_error("Page Down did not restore the selected step preset");

        const auto shiftRight = juce::KeyPress(juce::KeyPress::rightKey,
                                                juce::ModifierKeys::shiftModifier, 0);
        const auto shiftLeft = juce::KeyPress(juce::KeyPress::leftKey,
                                               juce::ModifierKeys::shiftModifier, 0);
        if (!grid.handleKeyCommandForTesting(shiftRight)
            || state.getStepData(1, 2).chainLength != 2)
            throw std::runtime_error("Shift+Right did not extend the selected tie");

        grid.setSelectedStep(1, 3);
        if (!grid.handleKeyCommandForTesting(shiftLeft)
            || state.getStepData(1, 2).chainLength != 1)
            throw std::runtime_error("Shift+Left did not shorten a tie from its consumed step");

        grid.setSelectedStep(1, 2);
        grid.handleKeyCommandForTesting(shiftRight);
        grid.handleKeyCommandForTesting(shiftRight);
        if (state.getStepData(1, 2).chainLength != 2)
            throw std::runtime_error("keyboard tie extension overwrote an active step");
        if (undoBoundaries != 5 || presetChanges != 2 || chainChanges != 3)
            throw std::runtime_error("keyboard step edits did not preserve callback and undo boundaries");
    }});

    tests.push_back({"visible footer controls expose purpose-specific accessible names", []
    {
        PluginProcessor processor;
        FooterPanel footer(processor.getPluginState().getValueTreeState());
        footer.setSize(1200, 150);
        footer.setSelectedSlot(0, 0, UserSlotData{}, "Slice");

        int interactiveControls = 0;
        for (int childIndex = 0; childIndex < footer.getNumChildComponents(); ++childIndex)
        {
            auto* child = footer.getChildComponent(childIndex);
            if (child == nullptr || !child->isVisible())
                continue;
            if (dynamic_cast<juce::Button*>(child) == nullptr
                && dynamic_cast<juce::Slider*>(child) == nullptr
                && dynamic_cast<Knob*>(child) == nullptr)
                continue;

            ++interactiveControls;
            if (child->getTitle().isEmpty())
                throw std::runtime_error("visible footer control has no accessible name");
        }

        if (interactiveControls < 11)
            throw std::runtime_error("footer accessibility test did not inspect the expected controls");
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

    tests.push_back({"step drag copies a preset through every crossed cell with one undo boundary", []
    {
        PluginProcessor processor;
        auto& state = processor.getSequencerState();

        StepData source;
        source.presetIndex = 13;
        state.setStepData(3, 2, source);
        setParameter(processor, getStepActiveID(3, 2), 1.0f);

        StepData existingChain;
        existingChain.active = true;
        existingChain.presetIndex = 4;
        existingChain.chainLength = 3;
        state.setStepData(3, 5, existingChain);
        setParameter(processor, getStepActiveID(3, 5), 1.0f);

        StepGrid grid(processor.getPluginState().getValueTreeState(), state);
        grid.setSize(960, 480);

        int undoBoundaries = 0;
        juce::ValueTree undoSnapshot;
        grid.onStepPresetEditStarting = [&]
        {
            ++undoBoundaries;
            undoSnapshot = processor.exportFullState();
        };

        auto* firstPaintedParameter = processor.getPluginState().getValueTreeState()
                                          .getParameter(getStepActiveID(3, 3));
        if (firstPaintedParameter == nullptr)
            throw std::runtime_error("step paint gesture test could not resolve its target parameter");
        ParameterGestureProbe gestureProbe;
        firstPaintedParameter->addListener(&gestureProbe);

        const auto start = stepGridCellCentre(grid, 3, 2);
        const auto finish = stepGridCellCentre(grid, 3, 6);
        grid.mouseDown(makeMouseEvent(grid, start, start,
                                      juce::ModifierKeys::leftButtonModifier, false));
        grid.mouseDrag(makeMouseEvent(grid, finish, start,
                                      juce::ModifierKeys::leftButtonModifier, true));
        grid.mouseUp(makeMouseEvent(grid, finish, start, juce::ModifierKeys{}, true));
        firstPaintedParameter->removeListener(&gestureProbe);

        if (undoBoundaries != 1)
            throw std::runtime_error("step paint gesture did not create exactly one undo boundary");
        if (gestureProbe.beginCount != 1 || gestureProbe.endCount != 1)
            throw std::runtime_error("painted step did not emit one complete host gesture");

        for (int step = 3; step <= 6; ++step)
        {
            const auto painted = state.getStepData(3, step);
            if (!painted.active || painted.presetIndex != 13 || painted.chainLength != 1)
                throw std::runtime_error("step paint skipped or corrupted crossed step " + std::to_string(step));

            const auto* parameter = processor.getPluginState().getValueTreeState()
                                        .getRawParameterValue(getStepActiveID(3, step));
            if (parameter == nullptr || parameter->load() < 0.5f)
                throw std::runtime_error("step paint did not synchronize active automation for step "
                                         + std::to_string(step));
        }

        if (state.getStepData(3, 2).chainLength != 1)
            throw std::runtime_error("normal step drag created a tie chain instead of copying the preset");
        if (grid.isStepConsumedByChain(3, 7))
            throw std::runtime_error("step paint left a stale chain spanning beyond an overwritten root");

        if (!undoSnapshot.isValid())
            throw std::runtime_error("step paint did not capture a restorable pre-edit snapshot");
        processor.applyFullState(undoSnapshot);

        const auto restoredTarget = state.getStepData(3, 3);
        const auto restoredChain = state.getStepData(3, 5);
        if (restoredTarget.active || restoredTarget.presetIndex != 0 || restoredTarget.chainLength != 1)
            throw std::runtime_error("step paint undo snapshot did not restore overwritten step metadata");
        if (!restoredChain.active || restoredChain.presetIndex != 4 || restoredChain.chainLength != 3)
            throw std::runtime_error("step paint undo snapshot did not restore overwritten chain metadata");

        const auto* restoredTargetGate = processor.getPluginState().getValueTreeState()
                                             .getRawParameterValue(getStepActiveID(3, 3));
        const auto* restoredChainGate = processor.getPluginState().getValueTreeState()
                                            .getRawParameterValue(getStepActiveID(3, 5));
        if (restoredTargetGate == nullptr || restoredTargetGate->load() >= 0.5f
            || restoredChainGate == nullptr || restoredChainGate->load() < 0.5f)
            throw std::runtime_error("step paint undo snapshot did not restore APVTS step gates");
    }});

    tests.push_back({"inactive step drag erases every crossed cell", []
    {
        PluginProcessor processor;
        auto& state = processor.getSequencerState();

        StepData staleSourceMetadata;
        staleSourceMetadata.active = true;
        staleSourceMetadata.presetIndex = 11;
        state.setStepData(4, 1, staleSourceMetadata);

        for (int step = 2; step <= 5; ++step)
        {
            StepData active;
            active.active = true;
            active.presetIndex = 7;
            state.setStepData(4, step, active);
            setParameter(processor, getStepActiveID(4, step), 1.0f);
        }

        StepGrid grid(processor.getPluginState().getValueTreeState(), state);
        grid.setSize(960, 480);

        int undoBoundaries = 0;
        grid.onStepPresetEditStarting = [&undoBoundaries] { ++undoBoundaries; };

        const auto start = stepGridCellCentre(grid, 4, 1);
        const auto finish = stepGridCellCentre(grid, 4, 5);
        grid.mouseDown(makeMouseEvent(grid, start, start,
                                      juce::ModifierKeys::leftButtonModifier, false));
        grid.mouseDrag(makeMouseEvent(grid, finish, start,
                                      juce::ModifierKeys::leftButtonModifier, true));
        grid.mouseUp(makeMouseEvent(grid, finish, start, juce::ModifierKeys{}, true));

        if (undoBoundaries != 1)
            throw std::runtime_error("inactive paint did not create exactly one undo boundary");

        for (int step = 2; step <= 5; ++step)
        {
            const auto erased = state.getStepData(4, step);
            if (erased.active || erased.presetIndex != 0 || erased.chainLength != 1)
                throw std::runtime_error("inactive paint did not erase crossed step " + std::to_string(step));

            const auto* parameter = processor.getPluginState().getValueTreeState()
                                        .getRawParameterValue(getStepActiveID(4, step));
            if (parameter == nullptr || parameter->load() >= 0.5f)
                throw std::runtime_error("inactive paint did not clear active automation for step "
                                         + std::to_string(step));
        }
    }});

    tests.push_back({"shift drag creates a tie without painting the consumed cells", []
    {
        PluginProcessor processor;
        auto& state = processor.getSequencerState();

        StepData source;
        source.active = true;
        source.presetIndex = 5;
        state.setStepData(1, 4, source);
        setParameter(processor, getStepActiveID(1, 4), 1.0f);

        StepGrid grid(processor.getPluginState().getValueTreeState(), state);
        grid.setSize(960, 480);

        int undoBoundaries = 0;
        grid.onStepPresetEditStarting = [&undoBoundaries] { ++undoBoundaries; };

        const auto shiftLeft = juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier
                                                   | juce::ModifierKeys::shiftModifier);
        const auto start = stepGridCellCentre(grid, 1, 4);
        const auto finish = stepGridCellCentre(grid, 1, 7);
        grid.mouseDown(makeMouseEvent(grid, start, start, shiftLeft, false));
        grid.mouseDrag(makeMouseEvent(grid, finish, start, shiftLeft, true));
        grid.mouseUp(makeMouseEvent(grid, finish, start, juce::ModifierKeys{}, true));

        if (state.getStepData(1, 4).chainLength != 4)
            throw std::runtime_error("shift drag did not create the expected four-step tie");
        if (undoBoundaries != 1)
            throw std::runtime_error("shift drag did not create exactly one undo boundary");

        for (int step = 5; step <= 7; ++step)
        {
            const auto consumed = state.getStepData(1, step);
            if (consumed.active || consumed.presetIndex != 0)
                throw std::runtime_error("shift drag painted a consumed tie cell");
        }

        const auto consumedPosition = stepGridCellCentre(grid, 1, 6);
        grid.mouseDown(makeMouseEvent(grid, consumedPosition, consumedPosition, shiftLeft, false));
        grid.mouseUp(makeMouseEvent(grid, consumedPosition, consumedPosition,
                                    juce::ModifierKeys{}, false));
        if (state.getStepData(1, 4).chainLength != 4 || undoBoundaries != 1)
            throw std::runtime_error("shift-click without dragging resized the tie chain");
    }});

    tests.push_back({"host-enabled blank step paints the default preset", []
    {
        PluginProcessor processor;
        auto& state = processor.getSequencerState();
        setParameter(processor, getStepActiveID(0, 0), 1.0f);

        StepGrid grid(processor.getPluginState().getValueTreeState(), state);
        grid.setSize(960, 480);

        const auto start = stepGridCellCentre(grid, 0, 0);
        const auto finish = stepGridCellCentre(grid, 0, 1);
        grid.mouseDown(makeMouseEvent(grid, start, start,
                                      juce::ModifierKeys::leftButtonModifier, false));
        grid.mouseDrag(makeMouseEvent(grid, finish, start,
                                      juce::ModifierKeys::leftButtonModifier, true));
        grid.mouseUp(makeMouseEvent(grid, finish, start, juce::ModifierKeys{}, true));

        const auto painted = state.getStepData(0, 1);
        if (!painted.active || painted.presetIndex != 1 || painted.chainLength != 1)
            throw std::runtime_error("host-enabled blank source produced an invalid active preset-zero step");
    }});

    tests.push_back({"step click only selects without editing data", []
    {
        PluginProcessor processor;
        auto& state = processor.getSequencerState();

        StepData source;
        source.active = true;
        source.presetIndex = 9;
        state.setStepData(2, 8, source);
        setParameter(processor, getStepActiveID(2, 8), 1.0f);

        StepGrid grid(processor.getPluginState().getValueTreeState(), state);
        grid.setSize(960, 480);

        int selectedLane = -1;
        int selectedStep = -1;
        int undoBoundaries = 0;
        grid.onStepSelected = [&selectedLane, &selectedStep](int lane, int step)
        {
            selectedLane = lane;
            selectedStep = step;
        };
        grid.onStepPresetEditStarting = [&undoBoundaries] { ++undoBoundaries; };

        const auto position = stepGridCellCentre(grid, 2, 8);
        grid.mouseDown(makeMouseEvent(grid, position, position,
                                      juce::ModifierKeys::leftButtonModifier, false));
        grid.mouseUp(makeMouseEvent(grid, position, position, juce::ModifierKeys{}, false));

        const auto unchanged = state.getStepData(2, 8);
        if (!unchanged.active || unchanged.presetIndex != 9 || unchanged.chainLength != 1)
            throw std::runtime_error("click-only step selection edited its source data");
        if (selectedLane != 2 || selectedStep != 8)
            throw std::runtime_error("click-only step selection did not notify the selected cell");
        if (undoBoundaries != 0)
            throw std::runtime_error("click-only step selection created an unnecessary undo boundary");
    }});

    tests.push_back({"lane mix knob follows automation and preset restore", []
    {
        PluginProcessor processor;
        auto& apvts = processor.getPluginState().getValueTreeState();
        StepGrid grid(apvts, processor.getSequencerState());
        auto* knob = findLaneMixKnob(grid, 0);
        if (knob == nullptr)
            throw std::runtime_error("lane mix knob was not found in the sequencer grid");

        setParameter(processor, getLaneMixID(0), 37.0f);
        requireNear(static_cast<float>(knob->getValue()), 37.0f, 0.001f,
                    "lane mix knob ignored external parameter automation");

        auto* parameter = apvts.getParameter(getLaneMixID(0));
        if (parameter == nullptr)
            throw std::runtime_error("lane mix automation test could not resolve its parameter");
        std::thread automationThread([parameter]
        {
            parameter->setValueNotifyingHost(parameter->convertTo0to1(48.0f));
        });
        automationThread.join();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
        requireNear(static_cast<float>(knob->getValue()), 48.0f, 0.001f,
                    "lane mix knob ignored asynchronous host automation");

        auto restoredState = apvts.copyState();
        setParameterStateValue(restoredState, getLaneMixID(0), 62.0f);
        apvts.replaceState(restoredState);
        requireNear(static_cast<float>(knob->getValue()), 62.0f, 0.001f,
                    "lane mix knob ignored preset state restore");
    }});

    tests.push_back({"lane mix knob sends one bounded host gesture per drag", []
    {
        PluginProcessor processor;
        setParameter(processor, getLaneMixID(0), 40.0f);

        auto& apvts = processor.getPluginState().getValueTreeState();
        StepGrid grid(apvts, processor.getSequencerState());
        auto* knob = findLaneMixKnob(grid, 0);
        auto* parameter = apvts.getParameter(getLaneMixID(0));
        if (knob == nullptr || parameter == nullptr)
            throw std::runtime_error("lane mix gesture test could not resolve its control or parameter");

        ParameterGestureProbe probe;
        parameter->addListener(&probe);

        juce::Component& control = *knob;
        const auto mouseDownPosition = juce::Point<float>(24.0f, 32.0f);
        control.mouseDown(makeMouseEvent(control,
                                         mouseDownPosition,
                                         mouseDownPosition,
                                         juce::ModifierKeys::leftButtonModifier,
                                         false));
        control.mouseDrag(makeMouseEvent(control,
                                         {24.0f, 12.0f},
                                         mouseDownPosition,
                                         juce::ModifierKeys::leftButtonModifier,
                                         true));
        control.mouseUp(makeMouseEvent(control,
                                       {24.0f, 12.0f},
                                       mouseDownPosition,
                                       juce::ModifierKeys{},
                                       true));

        parameter->removeListener(&probe);

        if (probe.beginCount != 1 || probe.endCount != 1)
            throw std::runtime_error("lane mix drag emitted unbalanced host gestures: begin="
                                     + std::to_string(probe.beginCount) + " end="
                                     + std::to_string(probe.endCount));

        requireNear(parameter->convertFrom0to1(parameter->getValue()), 50.0f, 0.001f,
                    "lane mix drag did not update the host parameter");
    }});

    tests.push_back({"lane mix double-click reset uses a complete host gesture", []
    {
        PluginProcessor processor;
        setParameter(processor, getLaneMixID(0), 40.0f);

        auto& apvts = processor.getPluginState().getValueTreeState();
        StepGrid grid(apvts, processor.getSequencerState());
        auto* knob = findLaneMixKnob(grid, 0);
        auto* parameter = apvts.getParameter(getLaneMixID(0));
        if (knob == nullptr || parameter == nullptr)
            throw std::runtime_error("lane mix reset test could not resolve its control or parameter");

        ParameterGestureProbe probe;
        parameter->addListener(&probe);

        juce::Component& control = *knob;
        const auto position = juce::Point<float>(24.0f, 24.0f);
        control.mouseDown(makeMouseEvent(control,
                                         position,
                                         position,
                                         juce::ModifierKeys::leftButtonModifier,
                                         false));
        control.mouseUp(makeMouseEvent(control,
                                       position,
                                       position,
                                       juce::ModifierKeys{},
                                       false));

        const int beginsBeforeReset = probe.beginCount;
        const int endsBeforeReset = probe.endCount;
        if (beginsBeforeReset != 1 || endsBeforeReset != 1)
            throw std::runtime_error("lane mix click-without-drag emitted unbalanced host gestures");
        control.mouseDoubleClick(makeMouseEvent(control,
                                                position,
                                                position,
                                                juce::ModifierKeys::leftButtonModifier,
                                                false,
                                                2));

        parameter->removeListener(&probe);

        if (probe.beginCount != beginsBeforeReset + 1 || probe.endCount != endsBeforeReset + 1)
            throw std::runtime_error("lane mix double-click reset was not wrapped in one complete host gesture");
        requireNear(parameter->convertFrom0to1(parameter->getValue()), 100.0f, 0.001f,
                    "lane mix double-click did not restore its default value");
    }});

    tests.push_back({"lane mix attachment closes an active gesture during destruction", []
    {
        PluginProcessor processor;
        auto& apvts = processor.getPluginState().getValueTreeState();
        auto* parameter = apvts.getParameter(getLaneMixID(0));
        if (parameter == nullptr)
            throw std::runtime_error("lane mix destruction test could not resolve its parameter");

        Knob knob;
        knob.setRange(0.0, 100.0);
        auto attachment = std::make_unique<KnobParameterAttachment>(*parameter, knob);
        ParameterGestureProbe probe;
        parameter->addListener(&probe);

        juce::Component& control = knob;
        const auto position = juce::Point<float>(24.0f, 24.0f);
        control.mouseDown(makeMouseEvent(control,
                                         position,
                                         position,
                                         juce::ModifierKeys::leftButtonModifier,
                                         false));
        attachment.reset();

        parameter->removeListener(&probe);

        if (probe.beginCount != 1 || probe.endCount != 1)
            throw std::runtime_error("destroyed lane mix attachment left an unbalanced host gesture: begin="
                                     + std::to_string(probe.beginCount) + " end="
                                     + std::to_string(probe.endCount));
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

    tests.push_back({"oversized host blocks use fixed scratch chunks without growth", []
    {
        PluginProcessor processor;
        processor.prepareToPlay(48000.0, 64);
        setParameter(processor, ParameterIDs::clockSource, 1.0f);
        setParameter(processor, ParameterIDs::dryWet, 0.0f);

        StepData sliceStep;
        sliceStep.active = true;
        sliceStep.presetIndex = 5;
        processor.getSequencerState().setStepData(0, 0, sliceStep);
        setParameter(processor, getStepActiveID(0, 0), 1.0f);

        const auto preparedCapacity = processor.getScratchCapacityForTesting();
        if (preparedCapacity < 64)
            throw std::runtime_error("processor did not prepare its declared scratch capacity");

        constexpr int oversizedBlock = 4097;
        juce::AudioBuffer<float> buffer(2, oversizedBlock);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                buffer.setSample(channel, sample, 0.1f + 0.00001f * static_cast<float>(sample));

        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);

        if (processor.getScratchCapacityForTesting() != preparedCapacity)
            throw std::runtime_error("oversized host block grew processor scratch storage on the audio thread");

        const int expectedChunks = (oversizedBlock + static_cast<int>(preparedCapacity) - 1)
                                 / static_cast<int>(preparedCapacity);
        if (processor.getLastProcessChunkCountForTesting() != expectedChunks)
            throw std::runtime_error("oversized host block was not split at the prepared scratch boundary");
        if (processor.getLaneOnsetCountForTesting(0) != 1)
            throw std::runtime_error("internal scratch chunks changed the single expected transport onset");

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                requireNear(buffer.getSample(channel, sample),
                            0.1f + 0.00001f * static_cast<float>(sample),
                            0.00001f,
                            "chunked dry processing changed oversized host audio");
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
