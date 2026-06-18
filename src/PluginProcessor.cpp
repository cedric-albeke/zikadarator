#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "engine/MixUtils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace zikada {

namespace {

std::unique_ptr<juce::FileLogger> processorLogFile;

void ensureProcessorLogger()
{
    static bool initialised = false;

    if (initialised)
        return;

    initialised = true;

    if (juce::Logger::getCurrentLogger() != nullptr)
        return;

    if (auto* logger = juce::FileLogger::createDefaultAppLogger("ZIKADARATOR",
                                                                "UI-Debug.log",
                                                                "ZIKADARATOR UI debug log",
                                                                512 * 1024))
    {
        processorLogFile.reset(logger);
        juce::Logger::setCurrentLogger(processorLogFile.get());
        juce::Logger::writeToLog("[ZIKADARATOR] logging to " + processorLogFile->getLogFile().getFullPathName());
    }
}

void debugProcessorLog(const juce::String& message)
{
    ensureProcessorLogger();
    juce::Logger::writeToLog("[ZIKADARATOR] PluginProcessor " + message);

#if JUCE_DEBUG
    DBG("[ZIKADARATOR] PluginProcessor " + message);
#endif
}

constexpr int kSliceLane = 0;
constexpr int kLoopLane = 1;
constexpr int kEnvelopeLane = 2;
constexpr int kFX1Lane = 3;
constexpr int kFilterLane = 4;
constexpr int kFX2Lane = 5;

bool isUserSlotPreset(int presetIndex)
{
    return presetIndex >= 1 && presetIndex <= 4;
}

UserSlotData getSlotDataForStep(const SequencerState& sequencerState, int lane, const StepData& stepData)
{
    const int slotIndex = isUserSlotPreset(stepData.presetIndex) ? stepData.presetIndex - 1 : 0;
    return sequencerState.getUserSlot(lane, slotIndex);
}

double getBeatSeconds(double bpm)
{
    return bpm > 0.0 ? 60.0 / bpm : 0.5;
}

bool isSemanticLoopSlot(const UserSlotData& slotData)
{
    return slotData.filterCutoff >= 0.25f && slotData.filterCutoff <= 4.0f
        && slotData.filterResonance >= 0.25f && slotData.filterResonance <= 4.0f;
}

float getLoopSlotLengthBeats(const UserSlotData& slotData)
{
    if (isSemanticLoopSlot(slotData))
        return juce::jlimit(0.25f, 4.0f, slotData.filterCutoff);

    return 0.5f;
}

float getLoopSlotRate(const UserSlotData& slotData)
{
    if (isSemanticLoopSlot(slotData))
        return juce::jlimit(0.25f, 4.0f, slotData.filterResonance);

    return 1.0f;
}

bool getLoopSlotReverse(const UserSlotData& slotData)
{
    if (!isSemanticLoopSlot(slotData))
        return false;

    return slotData.delayTime >= 0.5f;
}

float getLoopSlotMix(const UserSlotData& slotData, float baseMix)
{
    const float userMix = juce::jlimit(0.0f, 1.0f, slotData.delayMix);
    return juce::jlimit(0.0f, 0.95f, baseMix * (0.55f + userMix * 0.45f));
}

float getLoopSlotSmooth(const UserSlotData& slotData)
{
    if (!isSemanticLoopSlot(slotData))
        return 0.45f;

    return juce::jlimit(0.0f, 1.0f, slotData.delayFeedback);
}

float getParameterValue(const juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& parameterID,
                        float fallback)
{
    if (auto* parameter = apvts.getRawParameterValue(parameterID))
        return parameter->load();

    return fallback;
}

int getChoiceIndex(const juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& parameterID,
                   int fallback)
{
    return juce::roundToInt(getParameterValue(apvts, parameterID, static_cast<float>(fallback)));
}

double getPpqPerStepForResolution(int resolutionIndex)
{
    switch (resolutionIndex)
    {
        case 0: return 0.25; // 1/16 note
        case 1: return 0.5; // 1/8 note
        case 2: return 1.0; // 1/4 note
        case 3: return 2.0; // 1/2 note
        default: return 0.5;
    }
}

float blendGlobalMixSample(float dry, float wet, int mixMode, float wetAmount)
{
    wetAmount = juce::jlimit(0.0f, 1.0f, wetAmount);

    float modeSignal = wet;
    switch (mixMode)
    {
        case 1: // Ducking
        {
            const float detector = juce::jlimit(0.0f, 1.0f, std::abs(wet));
            const float duckedDry = dry * (1.0f - detector * wetAmount * 0.75f);
            return duckedDry * (1.0f - wetAmount) + wet * wetAmount;
        }
        case 2: // Sidechain
        {
            const float trigger = juce::jlimit(0.0f, 1.0f, std::abs(dry));
            return dry * (1.0f - wetAmount) + wet * (wetAmount * trigger);
        }
        case 3: // Multiply
            modeSignal = dry * wet;
            break;
        case 4: // Screen-style blend for bipolar audio.
            modeSignal = dry + wet - dry * wet;
            break;
        case 5: // Difference
            modeSignal = wet >= dry ? wet - dry : dry - wet;
            break;
        case 0:
        default:
            modeSignal = wet;
            break;
    }

    return dry * (1.0f - wetAmount) + modeSignal * wetAmount;
}

int getSliceIndexForPreset(int presetIndex, int currentStep)
{
    int divisions = 16;

    switch (presetIndex)
    {
        case 5: divisions = 1; break;
        case 6: divisions = 2; break;
        case 7: divisions = 3; break;
        case 8: divisions = 4; break;
        case 9: divisions = 6; break;
        case 10: divisions = 8; break;
        case 11: divisions = 12; break;
        case 12: divisions = 16; break;
        default: break;
    }

    const int divisionStep = currentStep % juce::jmax(1, divisions);
    return juce::jlimit(0, 15,
                        static_cast<int>(std::floor(static_cast<double>(divisionStep) * 16.0 / static_cast<double>(divisions))));
}

void applyGainPan(float* left, float* right, int numSamples, float volume, float pan)
{
    const float clampedVolume = juce::jlimit(0.0f, 2.0f, volume);
    const float clampedPan = juce::jlimit(-1.0f, 1.0f, pan);
    constexpr float panLaw = 0.70710678f;
    const float angle = (clampedPan + 1.0f) * 0.25f * 3.14159265f;
    const float leftGain = clampedVolume * panLaw * std::cos(angle) * 2.0f;
    const float rightGain = clampedVolume * panLaw * std::sin(angle) * 2.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        left[i] *= leftGain;
        right[i] *= rightGain;
    }
}

void applyTremolo(float* left, float* right, int numSamples, float depth, double phaseStart, double phaseDelta)
{
    const float clampedDepth = juce::jlimit(0.0f, 1.0f, depth);
    for (int i = 0; i < numSamples; ++i)
    {
        const float phase = static_cast<float>(std::fmod(phaseStart + phaseDelta * static_cast<double>(i), 1.0));
        const float lfo = 0.5f + 0.5f * std::sin(phase * 6.2831853f);
        const float gain = (1.0f - clampedDepth) + clampedDepth * lfo;
        left[i] *= gain;
        right[i] *= gain;
    }
}

void applyEnvelopeShape(float* left, float* right, int numSamples, int presetIndex,
                        double phaseStart, double phaseDelta, float volume, float pan)
{
    const float clampedPan = juce::jlimit(-1.0f, 1.0f, pan);
    constexpr float panLaw = 0.70710678f;
    const float angle = (clampedPan + 1.0f) * 0.25f * 3.14159265f;
    const float leftPanGain = panLaw * std::cos(angle) * 2.0f;
    const float rightPanGain = panLaw * std::sin(angle) * 2.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float phase = static_cast<float>(std::fmod(phaseStart + phaseDelta * static_cast<double>(i), 1.0));
        float shape = 1.0f;

        switch (presetIndex)
        {
            case 5: shape = phase; break;
            case 6: shape = 1.0f - phase; break;
            case 7: shape = 0.75f; break;
            case 8: shape = phase < 0.65f ? 1.0f : 0.0f; break;
            case 9: shape = std::exp(-6.0f * phase); break;
            case 10: shape = 0.35f + 0.65f * std::sin(phase * 3.14159265f); break;
            case 11: shape = phase * phase; break;
            case 12: shape = std::sin(phase * 6.2831853f) > 0.0f ? 1.0f : 0.25f; break;
            default: break;
        }

        const float gain = juce::jlimit(0.0f, 2.0f, volume * shape);
        left[i] *= gain * leftPanGain;
        right[i] *= gain * rightPanGain;
    }
}

void configureFilterForStep(FilterEngine& filterEngine, int presetIndex, const UserSlotData& slotData)
{
    switch (presetIndex)
    {
        case 5: filterEngine.setFilterType(FilterEngine::FilterType::LowPass12); break;
        case 6: filterEngine.setFilterType(FilterEngine::FilterType::LowPass24); break;
        case 7: filterEngine.setFilterType(FilterEngine::FilterType::HighPass12); break;
        case 8: filterEngine.setFilterType(FilterEngine::FilterType::HighPass24); break;
        case 9: filterEngine.setFilterType(FilterEngine::FilterType::BandPass); break;
        case 10: filterEngine.setFilterType(FilterEngine::FilterType::BandReject); break;
        case 11: filterEngine.setFilterType(FilterEngine::FilterType::Comb); break;
        case 12: filterEngine.setFilterType(FilterEngine::FilterType::LowPass24); break;
        case 13: filterEngine.setFilterType(FilterEngine::FilterType::LowPass12); break;
        case 14: filterEngine.setFilterType(FilterEngine::FilterType::HighPass12); break;
        case 15: filterEngine.setFilterType(FilterEngine::FilterType::BandPass); break;
        case 16: filterEngine.setFilterType(FilterEngine::FilterType::BandReject); break;
        case 17: filterEngine.setFilterType(FilterEngine::FilterType::Comb); break;
        case 18: filterEngine.setFilterType(FilterEngine::FilterType::LowPass24); break;
        case 19: filterEngine.setFilterType(FilterEngine::FilterType::BandPass); break;
        case 20: filterEngine.setFilterType(FilterEngine::FilterType::LowPass12); break;
        default: filterEngine.setFilterType(FilterEngine::FilterType::LowPass24); break;
    }

    filterEngine.setCutoff(slotData.filterCutoff);
    filterEngine.setResonance(slotData.filterResonance);
    filterEngine.setEnabled(true);
}

void processFxLane(float* left, float* right, int numSamples, int lane, int presetIndex, const UserSlotData& slotData,
                   DelayEngine& delayEngine, ReverbEngine& reverbEngine,
                   BitcrushEngine& bitcrushEngine, PitchEngine& pitchEngine, FilterEngine& toneFilter,
                   double bpm, double phaseStart, double phaseDelta)
{
    delayEngine.setEnabled(false);
    reverbEngine.setEnabled(false);
    bitcrushEngine.setEnabled(false);
    pitchEngine.setEnabled(false);
    toneFilter.setEnabled(false);

    const double beatSeconds = getBeatSeconds(bpm);
    const float phase = static_cast<float>(std::fmod(phaseStart, 1.0));

    if (lane == kFX1Lane)
    {
        switch (presetIndex)
        {
            case 5:
                delayEngine.setDelayTime(juce::jlimit(0.01f, 1.0f, slotData.delayTime * static_cast<float>(beatSeconds)));
                delayEngine.setFeedback(slotData.delayFeedback * 0.5f);
                delayEngine.setMix(slotData.delayMix);
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
            case 6:
                delayEngine.setDelayTime(0.012f + 0.01f * (0.5f + 0.5f * std::sin(phase * 6.2831853f)));
                delayEngine.setFeedback(juce::jlimit(0.0f, 0.35f, slotData.delayFeedback * 0.4f));
                delayEngine.setMix(juce::jlimit(0.15f, 0.65f, slotData.delayMix * 0.65f));
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
            case 7:
                reverbEngine.setRoomSize(juce::jlimit(0.1f, 0.5f, slotData.delayFeedback));
                reverbEngine.setDamping(juce::jlimit(0.0f, 1.0f, 1.0f - slotData.filterResonance * 0.1f));
                reverbEngine.setWidth(juce::jlimit(0.0f, 1.0f, 0.5f + slotData.pan * 0.5f));
                reverbEngine.setMix(slotData.delayMix);
                reverbEngine.setEnabled(true);
                reverbEngine.process(left, right, numSamples);
                break;
            case 8:
                reverbEngine.setRoomSize(juce::jlimit(0.5f, 1.0f, slotData.delayFeedback));
                reverbEngine.setDamping(juce::jlimit(0.0f, 0.6f, 1.0f - slotData.filterResonance * 0.15f));
                reverbEngine.setWidth(juce::jlimit(0.0f, 1.0f, 0.5f + slotData.pan * 0.5f));
                reverbEngine.setMix(juce::jlimit(0.2f, 0.9f, slotData.delayMix));
                reverbEngine.setEnabled(true);
                reverbEngine.process(left, right, numSamples);
                break;
            case 9:
                delayEngine.setDelayTime(0.015f + 0.008f * std::sin(phase * 6.2831853f));
                delayEngine.setFeedback(0.05f);
                delayEngine.setMix(juce::jlimit(0.2f, 0.5f, slotData.delayMix * 0.4f));
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
            case 10:
                delayEngine.setDelayTime(0.020f + 0.012f * std::sin(phase * 6.2831853f));
                delayEngine.setFeedback(0.15f);
                delayEngine.setMix(juce::jlimit(0.35f, 0.75f, slotData.delayMix * 0.6f));
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
            case 11:
                for (int i = 0; i < numSamples; ++i)
                {
                    float modPhase = static_cast<float>(std::fmod(phaseStart + phaseDelta * i, 1.0));
                    toneFilter.setFilterType(FilterEngine::FilterType::Comb);
                    toneFilter.setCutoff(juce::jlimit(200.0f, 4000.0f, 800.0f + 1200.0f * std::sin(modPhase * 3.14159265f)));
                    toneFilter.setResonance(juce::jlimit(2.0f, 10.0f, 4.0f + slotData.filterResonance));
                    toneFilter.setEnabled(true);
                    left[i] = toneFilter.processSampleLeft(left[i]);
                    right[i] = toneFilter.processSampleRight(right[i]);
                }
                break;
            case 12:
                for (int i = 0; i < numSamples; ++i)
                {
                    float modPhase = static_cast<float>(std::fmod(phaseStart + phaseDelta * i * 2.0, 1.0));
                    toneFilter.setFilterType(FilterEngine::FilterType::Comb);
                    toneFilter.setCutoff(juce::jlimit(200.0f, 6000.0f, 1000.0f + 2000.0f * std::sin(modPhase * 6.2831853f)));
                    toneFilter.setResonance(juce::jlimit(2.0f, 10.0f, 4.0f + slotData.filterResonance));
                    toneFilter.setEnabled(true);
                    left[i] = toneFilter.processSampleLeft(left[i]);
                    right[i] = toneFilter.processSampleRight(right[i]);
                }
                break;
            case 13:
                for (int i = 0; i < numSamples; ++i)
                {
                    float modPhase = static_cast<float>(std::fmod(phaseStart + phaseDelta * i, 1.0));
                    toneFilter.setFilterType(FilterEngine::FilterType::BandReject);
                    toneFilter.setCutoff(juce::jlimit(300.0f, 3000.0f, 600.0f + 1200.0f * std::sin(modPhase * 3.14159265f)));
                    toneFilter.setResonance(juce::jlimit(1.0f, 6.0f, 2.0f + slotData.filterResonance * 0.3f));
                    toneFilter.setEnabled(true);
                    left[i] = toneFilter.processSampleLeft(left[i]);
                    right[i] = toneFilter.processSampleRight(right[i]);
                }
                break;
            case 14:
                for (int i = 0; i < numSamples; ++i)
                {
                    float modPhase = static_cast<float>(std::fmod(phaseStart + phaseDelta * i * 1.5, 1.0));
                    toneFilter.setFilterType(FilterEngine::FilterType::BandReject);
                    toneFilter.setCutoff(juce::jlimit(200.0f, 5000.0f, 800.0f + 2000.0f * std::sin(modPhase * 6.2831853f)));
                    toneFilter.setResonance(juce::jlimit(3.0f, 12.0f, 5.0f + slotData.filterResonance));
                    toneFilter.setEnabled(true);
                    left[i] = toneFilter.processSampleLeft(left[i]);
                    right[i] = toneFilter.processSampleRight(right[i]);
                }
                break;
            case 15:
                applyTremolo(left, right, numSamples, juce::jlimit(0.2f, 0.8f, slotData.delayMix), phaseStart, phaseDelta);
                applyGainPan(left, right, numSamples, slotData.volume, slotData.pan);
                return;
            case 16:
                applyTremolo(left, right, numSamples, juce::jlimit(0.5f, 1.0f, slotData.delayMix), phaseStart * 2.0, phaseDelta * 2.0);
                applyGainPan(left, right, numSamples, slotData.volume, slotData.pan);
                return;
            case 17:
                for (int i = 0; i < numSamples; ++i)
                {
                    left[i] = std::tanh(left[i] * 2.5f);
                    right[i] = std::tanh(right[i] * 2.5f);
                }
                break;
            case 18:
                for (int i = 0; i < numSamples; ++i)
                {
                    left[i] = std::tanh(left[i] * 6.0f);
                    right[i] = std::tanh(right[i] * 6.0f);
                }
                break;
            case 19:
                pitchEngine.setSemitones(juce::jmap(slotData.pan, -1.0f, 1.0f, -3.0f, 3.0f));
                pitchEngine.setMix(juce::jlimit(0.2f, 0.6f, slotData.delayMix * 0.5f));
                pitchEngine.setEnabled(true);
                pitchEngine.process(left, right, numSamples);
                break;
            case 20:
                bitcrushEngine.setBitDepth(juce::jmap(slotData.filterResonance, 0.1f, 10.0f, 10.0f, 5.0f));
                bitcrushEngine.setSampleRateReduction(juce::jmap(slotData.delayTime, 0.0f, 1.0f, 12000.0f, 4000.0f));
                bitcrushEngine.setEnabled(true);
                bitcrushEngine.process(left, right, numSamples);
                pitchEngine.setSemitones(juce::jmap(slotData.pan, -1.0f, 1.0f, -5.0f, 5.0f));
                pitchEngine.setMix(0.4f);
                pitchEngine.setEnabled(true);
                pitchEngine.process(left, right, numSamples);
                break;
            default:
                delayEngine.setDelayTime(juce::jlimit(0.01f, 1.0f, slotData.delayTime * static_cast<float>(beatSeconds)));
                delayEngine.setFeedback(slotData.delayFeedback * 0.5f);
                delayEngine.setMix(slotData.delayMix);
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
        }
    }
    else
    {
        switch (presetIndex)
        {
            case 5:
                bitcrushEngine.setBitDepth(juce::jmap(slotData.filterResonance, 0.1f, 10.0f, 12.0f, 7.0f));
                bitcrushEngine.setSampleRateReduction(juce::jmap(slotData.delayTime, 0.0f, 1.0f, 16000.0f, 8000.0f));
                bitcrushEngine.setEnabled(true);
                bitcrushEngine.process(left, right, numSamples);
                break;
            case 6:
                bitcrushEngine.setBitDepth(juce::jmap(slotData.filterResonance, 0.1f, 10.0f, 8.0f, 2.0f));
                bitcrushEngine.setSampleRateReduction(juce::jmap(slotData.delayTime, 0.0f, 1.0f, 12000.0f, 1800.0f));
                bitcrushEngine.setEnabled(true);
                bitcrushEngine.process(left, right, numSamples);
                break;
            case 7:
                pitchEngine.setSemitones(juce::jmap(slotData.pan, -1.0f, 1.0f, 0.0f, 12.0f));
                pitchEngine.setMix(slotData.delayMix);
                pitchEngine.setEnabled(true);
                pitchEngine.process(left, right, numSamples);
                break;
            case 8:
                pitchEngine.setSemitones(juce::jmap(slotData.pan, -1.0f, 1.0f, -12.0f, 0.0f));
                pitchEngine.setMix(slotData.delayMix);
                pitchEngine.setEnabled(true);
                pitchEngine.process(left, right, numSamples);
                break;
            case 9:
                pitchEngine.setSemitones(-7.0f);
                pitchEngine.setMix(juce::jlimit(0.3f, 0.8f, slotData.delayMix));
                pitchEngine.setEnabled(true);
                pitchEngine.process(left, right, numSamples);
                toneFilter.setFilterType(FilterEngine::FilterType::LowPass12);
                toneFilter.setCutoff(800.0f);
                toneFilter.setResonance(1.5f);
                toneFilter.setEnabled(true);
                toneFilter.process(left, right, numSamples);
                break;
            case 10:
                delayEngine.setDelayTime(0.003f + 0.002f * std::sin(phase * 12.5663706f));
                delayEngine.setFeedback(0.1f);
                delayEngine.setMix(0.5f);
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
            case 11:
                delayEngine.setDelayTime(juce::jlimit(0.05f, 0.8f, slotData.delayTime * static_cast<float>(beatSeconds) * 2.0f));
                delayEngine.setFeedback(juce::jlimit(0.1f, 0.5f, slotData.delayFeedback * 0.5f));
                delayEngine.setMix(juce::jlimit(0.2f, 0.7f, slotData.delayMix));
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
            case 12:
                delayEngine.setDelayTime(juce::jlimit(0.05f, 1.0f, slotData.delayTime * static_cast<float>(beatSeconds) * 2.5f));
                delayEngine.setFeedback(juce::jlimit(0.2f, 0.75f, slotData.delayFeedback));
                delayEngine.setMix(juce::jlimit(0.3f, 0.8f, slotData.delayMix));
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
            case 13:
                toneFilter.setFilterType(FilterEngine::FilterType::Comb);
                toneFilter.setCutoff(3000.0f);
                toneFilter.setResonance(juce::jlimit(4.0f, 14.0f, 8.0f + slotData.filterResonance));
                toneFilter.setEnabled(true);
                for (int i = 0; i < numSamples; ++i)
                {
                    float rm = std::sin(static_cast<float>(i) * 0.15f) * 0.15f;
                    left[i] *= (1.0f + rm);
                    right[i] *= (1.0f + rm);
                    left[i] = toneFilter.processSampleLeft(left[i]);
                    right[i] = toneFilter.processSampleRight(right[i]);
                }
                break;
            case 14:
                bitcrushEngine.setBitDepth(4.0f);
                bitcrushEngine.setSampleRateReduction(6000.0f);
                bitcrushEngine.setEnabled(true);
                bitcrushEngine.process(left, right, numSamples);
                toneFilter.setFilterType(FilterEngine::FilterType::Comb);
                toneFilter.setCutoff(2500.0f);
                toneFilter.setResonance(12.0f);
                toneFilter.setEnabled(true);
                for (int i = 0; i < numSamples; ++i)
                {
                    float rm = std::sin(static_cast<float>(i) * 0.25f) * 0.35f;
                    left[i] *= (1.0f + rm);
                    right[i] *= (1.0f + rm);
                    left[i] = toneFilter.processSampleLeft(left[i]);
                    right[i] = toneFilter.processSampleRight(right[i]);
                }
                break;
            case 15:
                toneFilter.setFilterType(FilterEngine::FilterType::BandPass);
                toneFilter.setCutoff(juce::jlimit(800.0f, 8000.0f, slotData.filterCutoff));
                toneFilter.setResonance(juce::jlimit(2.0f, 10.0f, slotData.filterResonance));
                toneFilter.setEnabled(true);
                toneFilter.process(left, right, numSamples);
                break;
            case 16:
                toneFilter.setFilterType(FilterEngine::FilterType::LowPass12);
                toneFilter.setCutoff(juce::jlimit(100.0f, 2000.0f, slotData.filterCutoff * 0.5f));
                toneFilter.setResonance(juce::jlimit(1.0f, 8.0f, slotData.filterResonance));
                toneFilter.setEnabled(true);
                toneFilter.process(left, right, numSamples);
                break;
            case 17:
                bitcrushEngine.setBitDepth(juce::jmap(slotData.filterResonance, 0.1f, 10.0f, 6.0f, 3.0f));
                bitcrushEngine.setSampleRateReduction(juce::jmap(slotData.delayTime, 0.0f, 1.0f, 14000.0f, 3000.0f));
                bitcrushEngine.setEnabled(true);
                bitcrushEngine.process(left, right, numSamples);
                break;
            case 18:
                pitchEngine.setSemitones(juce::jmap(phase, 0.0f, 1.0f, -7.0f, 7.0f));
                pitchEngine.setMix(juce::jlimit(0.3f, 0.7f, slotData.delayMix));
                pitchEngine.setEnabled(true);
                pitchEngine.process(left, right, numSamples);
                break;
            case 19:
                reverbEngine.setRoomSize(juce::jlimit(0.7f, 1.0f, slotData.delayFeedback));
                reverbEngine.setDamping(0.2f);
                reverbEngine.setWidth(0.8f);
                reverbEngine.setMix(juce::jlimit(0.4f, 0.9f, slotData.delayMix));
                reverbEngine.setEnabled(true);
                reverbEngine.process(left, right, numSamples);
                break;
            case 20:
                pitchEngine.setSemitones(12.0f);
                pitchEngine.setMix(0.35f);
                pitchEngine.setEnabled(true);
                pitchEngine.process(left, right, numSamples);
                reverbEngine.setRoomSize(juce::jlimit(0.5f, 1.0f, slotData.delayFeedback));
                reverbEngine.setDamping(0.3f);
                reverbEngine.setWidth(0.7f);
                reverbEngine.setMix(juce::jlimit(0.3f, 0.8f, slotData.delayMix));
                reverbEngine.setEnabled(true);
                reverbEngine.process(left, right, numSamples);
                break;
            default:
                delayEngine.setDelayTime(juce::jlimit(0.01f, 1.0f, slotData.delayTime * static_cast<float>(beatSeconds)));
                delayEngine.setFeedback(slotData.delayFeedback);
                delayEngine.setMix(slotData.delayMix);
                delayEngine.setEnabled(true);
                delayEngine.process(left, right, numSamples);
                break;
        }
    }

    applyGainPan(left, right, numSamples, slotData.volume, slotData.pan);
}

void configureLoopEngineForPreset(LoopEngine& loopEngine, int presetIndex, const UserSlotData& slotData,
                                  double beatSeconds)
{
    loopEngine.setEnabled(true);

    const auto sixteenth = static_cast<float>(beatSeconds * 0.25);
    const auto eighth    = static_cast<float>(beatSeconds * 0.5);
    const auto quarter   = static_cast<float>(beatSeconds);
    const auto half      = static_cast<float>(beatSeconds * 2.0);
    const auto fourBeat  = static_cast<float>(beatSeconds * 4.0);
    const auto eightBeat = static_cast<float>(beatSeconds * 8.0);

    if (presetIndex >= 1 && presetIndex <= 4)
    {
        loopEngine.setLoopParameters(static_cast<float>(beatSeconds) * getLoopSlotLengthBeats(slotData),
                                     getLoopSlotRate(slotData),
                                     getLoopSlotReverse(slotData),
                                     getLoopSlotMix(slotData, 0.92f),
                                     getLoopSlotSmooth(slotData));
        return;
    }

    switch (presetIndex)
    {
        case 5:
            loopEngine.setLoopParameters(sixteenth, 1.0f, false, getLoopSlotMix(slotData, 0.56f), getLoopSlotSmooth(slotData));
            break;
        case 6:
            loopEngine.setLoopParameters(eighth, 1.0f, false, getLoopSlotMix(slotData, 0.62f), getLoopSlotSmooth(slotData));
            break;
        case 7:
            loopEngine.setLoopParameters(quarter, 1.0f, false, getLoopSlotMix(slotData, 0.68f), getLoopSlotSmooth(slotData));
            break;
        case 8:
            loopEngine.setLoopParameters(half, 1.0f, false, getLoopSlotMix(slotData, 0.74f), getLoopSlotSmooth(slotData));
            break;
        case 9:
            loopEngine.setLoopParameters(sixteenth, 1.0f, true, getLoopSlotMix(slotData, 0.58f), getLoopSlotSmooth(slotData));
            break;
        case 10:
            loopEngine.setLoopParameters(eighth, 1.0f, true, getLoopSlotMix(slotData, 0.64f), getLoopSlotSmooth(slotData));
            break;
        case 11:
            loopEngine.setLoopParameters(quarter, 1.0f, true, getLoopSlotMix(slotData, 0.70f), getLoopSlotSmooth(slotData));
            break;
        case 12:
            loopEngine.setLoopParameters(half, 1.0f, true, getLoopSlotMix(slotData, 0.76f), getLoopSlotSmooth(slotData));
            break;
        case 13:
            loopEngine.setLoopParameters(eighth, 2.0f, false, getLoopSlotMix(slotData, 0.62f), getLoopSlotSmooth(slotData));
            break;
        case 14:
            loopEngine.setLoopParameters(eighth, 4.0f, false, getLoopSlotMix(slotData, 0.58f), getLoopSlotSmooth(slotData));
            break;
        case 15:
            loopEngine.setLoopParameters(quarter, 0.5f, false, getLoopSlotMix(slotData, 0.64f), getLoopSlotSmooth(slotData));
            break;
        case 16:
            loopEngine.setLoopParameters(quarter, 0.25f, false, getLoopSlotMix(slotData, 0.62f), getLoopSlotSmooth(slotData));
            break;
        case 17:
            loopEngine.setLoopParameters(sixteenth, 2.0f, true, getLoopSlotMix(slotData, 0.60f), getLoopSlotSmooth(slotData));
            break;
        case 18:
            loopEngine.setLoopParameters(sixteenth, 4.0f, true, getLoopSlotMix(slotData, 0.56f), getLoopSlotSmooth(slotData));
            break;
        case 19:
            loopEngine.setLoopParameters(fourBeat, 1.0f, false, getLoopSlotMix(slotData, 0.64f), getLoopSlotSmooth(slotData));
            break;
        case 20:
            loopEngine.setLoopParameters(eightBeat, 1.0f, false, getLoopSlotMix(slotData, 0.60f), getLoopSlotSmooth(slotData));
            break;
        default:
            loopEngine.setLoopParameters(eighth, 1.0f, false, 0.0f);
            break;
    }
}

} // namespace

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this)
{
    debugProcessorLog("constructed processor=" + juce::String::toHexString(static_cast<juce::int64>(reinterpret_cast<std::uintptr_t>(this))));
}

PluginProcessor::~PluginProcessor()
{
    debugProcessorLog("destructed processor=" + juce::String::toHexString(static_cast<juce::int64>(reinterpret_cast<std::uintptr_t>(this))));
}

void PluginProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    sequencerEngine.prepare(newSampleRate, samplesPerBlock);
    sliceEngine.prepare(newSampleRate, samplesPerBlock);
    loopEngine.prepare(newSampleRate, samplesPerBlock);
    filterEngine.prepare(newSampleRate, samplesPerBlock);
    fx1DelayEngine.prepare(newSampleRate, samplesPerBlock);
    fx1ReverbEngine.prepare(newSampleRate, samplesPerBlock);
    fx1BitcrushEngine.prepare(newSampleRate, samplesPerBlock);
    fx1PitchEngine.prepare(newSampleRate, samplesPerBlock);
    fx1ToneFilter.prepare(newSampleRate, samplesPerBlock);
    fx2DelayEngine.prepare(newSampleRate, samplesPerBlock);
    fx2ReverbEngine.prepare(newSampleRate, samplesPerBlock);
    fx2BitcrushEngine.prepare(newSampleRate, samplesPerBlock);
    fx2PitchEngine.prepare(newSampleRate, samplesPerBlock);
    fx2ToneFilter.prepare(newSampleRate, samplesPerBlock);
    modulationEngine.prepare(newSampleRate);
    gainPanEngine.prepare(newSampleRate, samplesPerBlock);
    waveformTap.prepare(static_cast<int>(newSampleRate * 2.0));
    processedWaveformTap.prepare(static_cast<int>(newSampleRate * 2.0));
    ensureScratchBuffers(samplesPerBlock);
    lastEffectiveSteps.fill(-1);
}

void PluginProcessor::ensureScratchBuffers(int numSamples)
{
    const auto requiredSize = static_cast<size_t>(juce::jmax(0, numSamples));
    auto ensureSize = [requiredSize](std::vector<float>& buffer)
    {
        if (buffer.size() < requiredSize)
            buffer.resize(requiredSize, 0.0f);
    };

    ensureSize(dryLeftBuffer);
    ensureSize(dryRightBuffer);
    ensureSize(monoRightBuffer);
    ensureSize(wetLeftBuffer);
    ensureSize(wetRightBuffer);
    ensureSize(sliceLeftBuffer);
    ensureSize(sliceRightBuffer);
    ensureSize(laneInputLeftBuffer);
    ensureSize(laneInputRightBuffer);
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

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || buffer.getNumChannels() <= 0)
        return;

    ensureScratchBuffers(numSamples);

    auto* leftChannel = buffer.getWritePointer(0);
    const bool hasRightChannel = buffer.getNumChannels() > 1;
    auto* rightChannel = hasRightChannel ? buffer.getWritePointer(1) : monoRightBuffer.data();

    if (! hasRightChannel)
        std::copy(leftChannel, leftChannel + numSamples, rightChannel);

    std::copy(leftChannel, leftChannel + numSamples, dryLeftBuffer.begin());
    std::copy(rightChannel, rightChannel + numSamples, dryRightBuffer.begin());
    waveformTap.pushFromAudioThread(dryLeftBuffer.data(), numSamples);

    auto& apvts = state.getValueTreeState();
    const float globalDryWet = juce::jlimit(0.0f, 1.0f, getParameterValue(apvts, ParameterIDs::dryWet, 100.0f) / 100.0f);
    const float outputGain = std::pow(10.0f, getParameterValue(apvts, ParameterIDs::outputGain, 0.0f) / 20.0f);
    const int globalMixMode = juce::jlimit(0, 5, getChoiceIndex(apvts, ParameterIDs::mixMode, 0));
    const bool bypassed = getParameterValue(apvts, ParameterIDs::bypass, 0.0f) > 0.5f;
    const int clockSource = juce::jlimit(0, 1, getChoiceIndex(apvts, ParameterIDs::clockSource, 0));
    const float freeTempo = juce::jlimit(20.0f, 300.0f, getParameterValue(apvts, ParameterIDs::tempo, 120.0f));
    const int stepResolutionIndex = juce::jlimit(0, 3, getChoiceIndex(apvts, ParameterIDs::stepResolution, 1));
    const double blockPpqPerStep = getPpqPerStepForResolution(stepResolutionIndex);
    ppqPerStep = blockPpqPerStep;
    currentPpqPerStep = blockPpqPerStep;
    sequencerEngine.setStepResolution(stepResolutionIndex);

    double bpm = freeTempo;
    bool useFreeClock = clockSource == 1;
    bool blockIsPlaying = false;
    double blockStartPpq = ppqPosition;

    if (auto currentPlayHead = getPlayHead(); currentPlayHead != nullptr && !useFreeClock)
    {
        if (const auto posInfo = currentPlayHead->getPosition())
        {
            blockIsPlaying = posInfo->getIsPlaying();
            isPlayingFlag = blockIsPlaying;

            if (const auto hostBpm = posInfo->getBpm(); hostBpm && *hostBpm > 0.0)
                bpm = *hostBpm;

            if (const auto hostPpq = posInfo->getPpqPosition(); hostPpq && *hostPpq >= 0.0)
            {
                blockStartPpq = *hostPpq;
                ppqPosition = blockStartPpq;
            }
        }
        else
        {
            isPlayingFlag = false;
        }

        currentBPM = bpm;
        sequencerEngine.setTempo(bpm);
        sequencerEngine.setPlaying(false);
    }
    else
    {
        useFreeClock = true;
        currentBPM = bpm;
        blockStartPpq = ppqPosition;
        blockIsPlaying = true;
        isPlayingFlag = true;
        sequencerEngine.setTempo(bpm);
        sequencerEngine.setPlaying(true);
    }

    StepScheduler::Segment segments[64]{};
    const int segmentCount = stepScheduler.makeHostSegments({sampleRate,
                                                             bpm,
                                                             blockPpqPerStep,
                                                             blockStartPpq,
                                                             numSamples,
                                                             blockIsPlaying},
                                                            segments,
                                                            static_cast<int>(std::size(segments)));
    if (segmentCount <= 0)
    {
        processedWaveformTap.pushFromAudioThread(leftChannel, numSamples);
        return;
    }

    currentStep = segments[segmentCount - 1].stepIndex;

    if (useFreeClock)
        ppqPosition = blockStartPpq + ((bpm / 60.0) / sampleRate) * static_cast<double>(numSamples);

    if (bypassed)
    {
        processedWaveformTap.pushFromAudioThread(leftChannel, numSamples);
        return;
    }

    float laneMix[6] = {};
    bool laneMuted[6] = {};
    bool laneSoloed[6] = {};
    bool anySolo = false;

    for (int lane = 0; lane < 6; ++lane)
    {
        if (auto* p = state.getValueTreeState().getRawParameterValue(getLaneMixID(lane)))
            laneMix[lane] = p->load() / 100.0f;
        else
            laneMix[lane] = 1.0f;

        if (auto* p = state.getValueTreeState().getRawParameterValue(getLaneMuteID(lane)))
            laneMuted[lane] = p->load() > 0.5f;

        if (auto* p = state.getValueTreeState().getRawParameterValue(getLaneSoloID(lane)))
        {
            laneSoloed[lane] = p->load() > 0.5f;
            if (laneSoloed[lane]) anySolo = true;
        }
    }

    for (int i = 0; i < segmentCount; ++i)
    {
        const auto& segment = segments[i];
        processSegment(leftChannel + segment.startSample,
                       rightChannel + segment.startSample,
                       dryLeftBuffer.data() + segment.startSample,
                       dryRightBuffer.data() + segment.startSample,
                       segment.numSamples,
                       hasRightChannel,
                       segment,
                       bpm,
                       blockPpqPerStep,
                       laneMix,
                       laneMuted,
                       laneSoloed,
                       anySolo,
                       globalMixMode,
                       globalDryWet,
                       outputGain);
    }

    processedWaveformTap.pushFromAudioThread(leftChannel, numSamples);
}

void PluginProcessor::processSegment(float* leftChannel,
                                     float* rightChannel,
                                     const float* dryLeft,
                                     const float* dryRight,
                                     int numSamples,
                                     bool hasRightChannel,
                                     const StepScheduler::Segment& segment,
                                     double bpm,
                                     double blockPpqPerStep,
                                     const float* laneMix,
                                     const bool* laneMuted,
                                     const bool* laneSoloed,
                                     bool anySolo,
                                     int globalMixMode,
                                     float globalDryWet,
                                     float outputGain)
{
    if (numSamples <= 0)
        return;

    const int sequencerStep = segment.stepIndex;
    const double stepPhaseStart = segment.phaseStart;
    const double phaseDelta = segment.phaseDelta;
    currentStep = sequencerStep;

    auto getEffectiveStep = [&](int lane, int rawStep) -> int
    {
        for (int lookback = rawStep; lookback >= juce::jmax(0, rawStep - 15); --lookback)
        {
            const auto& s = sequencerState.getStepData(lane, lookback);
            if (s.active && s.presetIndex > 0)
            {
                if (rawStep < lookback + s.chainLength)
                    return lookback;
            }
        }
        return rawStep;
    };

    const int effSliceStep    = getEffectiveStep(kSliceLane,    sequencerStep);
    const int effLoopStep     = getEffectiveStep(kLoopLane,     sequencerStep);
    const int effEnvelopeStep = getEffectiveStep(kEnvelopeLane, sequencerStep);
    const int effFx1Step      = getEffectiveStep(kFX1Lane,      sequencerStep);
    const int effFilterStep   = getEffectiveStep(kFilterLane,   sequencerStep);
    const int effFx2Step      = getEffectiveStep(kFX2Lane,      sequencerStep);

    const auto& sliceStep    = sequencerState.getStepData(kSliceLane,    effSliceStep);
    const auto& loopStep     = sequencerState.getStepData(kLoopLane,     effLoopStep);
    const auto& envelopeStep = sequencerState.getStepData(kEnvelopeLane, effEnvelopeStep);
    const auto& fx1Step      = sequencerState.getStepData(kFX1Lane,      effFx1Step);
    const auto& filterStep   = sequencerState.getStepData(kFilterLane,   effFilterStep);
    const auto& fx2Step      = sequencerState.getStepData(kFX2Lane,      effFx2Step);

    const auto sliceSlot    = getSlotDataForStep(sequencerState, kSliceLane,    sliceStep);
    const auto loopSlot     = getSlotDataForStep(sequencerState, kLoopLane,     loopStep);
    const auto envelopeSlot = getSlotDataForStep(sequencerState, kEnvelopeLane, envelopeStep);
    const auto fx1Slot      = getSlotDataForStep(sequencerState, kFX1Lane,      fx1Step);
    const auto filterSlot   = getSlotDataForStep(sequencerState, kFilterLane,   filterStep);
    const auto fx2Slot      = getSlotDataForStep(sequencerState, kFX2Lane,      fx2Step);

    const double beatSeconds = getBeatSeconds(bpm);
    const double stepDurationSeconds = beatSeconds * blockPpqPerStep;

    if (effSliceStep != lastEffectiveSteps[kSliceLane])
    {
        lastEffectiveSteps[kSliceLane] = effSliceStep;
        if (sliceStep.active && sliceStep.presetIndex > 0)
            sliceEngine.triggerSlice(getSliceIndexForPreset(sliceStep.presetIndex, effSliceStep));
    }

    if (effLoopStep != lastEffectiveSteps[kLoopLane])
    {
        lastEffectiveSteps[kLoopLane] = effLoopStep;
        if (loopStep.active && loopStep.presetIndex > 0)
        {
            configureLoopEngineForPreset(loopEngine, loopStep.presetIndex, loopSlot, beatSeconds);
            loopEngine.trigger();
        }
        else
        {
            loopEngine.setEnabled(false);
        }
    }

    if (effFilterStep != lastEffectiveSteps[kFilterLane])
    {
        lastEffectiveSteps[kFilterLane] = effFilterStep;
        if (filterStep.active && filterStep.presetIndex > 0)
        {
            configureFilterForStep(filterEngine, filterStep.presetIndex, filterSlot);
            modulationEngine.setStepData(filterSlot.modulation, bpm, stepDurationSeconds);
        }
    }

    sliceEngine.writeToBuffer(leftChannel, rightChannel, numSamples);

    auto* wetLeft = wetLeftBuffer.data();
    auto* wetRight = wetRightBuffer.data();
    for (int i = 0; i < numSamples; ++i)
    {
        wetLeft[i] = leftChannel[i];
        wetRight[i] = rightChannel[i];
    }

    auto isLaneActive = [&](int lane) -> bool
    {
        if (laneMuted[lane]) return false;
        if (anySolo && !laneSoloed[lane]) return false;
        return true;
    };

    auto captureLaneInput = [&](int n)
    {
        std::copy(wetLeft, wetLeft + n, laneInputLeftBuffer.data());
        std::copy(wetRight, wetRight + n, laneInputRightBuffer.data());
    };

    auto blendLaneOutput = [&](int lane, int n)
    {
        applyLaneMix(wetLeft,
                     wetRight,
                     laneInputLeftBuffer.data(),
                     laneInputRightBuffer.data(),
                     n,
                     laneMix[lane]);
    };

    if (sliceStep.active && sliceStep.presetIndex > 0 && isLaneActive(kSliceLane))
    {
        captureLaneInput(numSamples);
        auto* sliceLeft = sliceLeftBuffer.data();
        auto* sliceRight = sliceRightBuffer.data();
        sliceEngine.process(sliceLeft, sliceRight, numSamples);
        std::copy(sliceLeft, sliceLeft + numSamples, wetLeft);
        std::copy(sliceRight, sliceRight + numSamples, wetRight);
        applyGainPan(wetLeft, wetRight, numSamples, sliceSlot.volume, sliceSlot.pan);
        blendLaneOutput(kSliceLane, numSamples);
    }

    loopEngine.captureInput(wetLeft, wetRight, numSamples);

    if (loopStep.active && loopStep.presetIndex > 0 && isLaneActive(kLoopLane))
    {
        captureLaneInput(numSamples);
        loopEngine.process(wetLeft, wetRight, numSamples);
        applyGainPan(wetLeft, wetRight, numSamples, loopSlot.volume, loopSlot.pan);
        blendLaneOutput(kLoopLane, numSamples);
    }

    if (envelopeStep.active && envelopeStep.presetIndex > 0 && isLaneActive(kEnvelopeLane))
    {
        captureLaneInput(numSamples);
        applyEnvelopeShape(wetLeft, wetRight, numSamples, envelopeStep.presetIndex,
                           stepPhaseStart, phaseDelta, envelopeSlot.volume, envelopeSlot.pan);
        blendLaneOutput(kEnvelopeLane, numSamples);
    }

    if (fx1Step.active && fx1Step.presetIndex > 0 && isLaneActive(kFX1Lane))
    {
        captureLaneInput(numSamples);
        processFxLane(wetLeft, wetRight, numSamples, kFX1Lane, fx1Step.presetIndex, fx1Slot,
                      fx1DelayEngine, fx1ReverbEngine, fx1BitcrushEngine, fx1PitchEngine, fx1ToneFilter,
                      bpm, stepPhaseStart, phaseDelta);
        blendLaneOutput(kFX1Lane, numSamples);
    }

    if (filterStep.active && filterStep.presetIndex > 0 && isLaneActive(kFilterLane))
    {
        captureLaneInput(numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            float modValues[ModulationEngine::NumTargets]{};
            modulationEngine.processSample(wetLeft[i], wetRight[i], modValues);

            const float cutoffMod = modValues[static_cast<int>(ModulationTarget::FilterCutoff)];
            const float modCutoff = filterSlot.filterCutoff * std::pow(2.0f, cutoffMod * 3.0f);
            filterEngine.setCutoff(modCutoff);
            filterEngine.setResonance(filterSlot.filterResonance);

            wetLeft[i] = filterEngine.processSampleLeft(wetLeft[i]);
            wetRight[i] = filterEngine.processSampleRight(wetRight[i]);
        }

        applyGainPan(wetLeft, wetRight, numSamples, filterSlot.volume, filterSlot.pan);
        blendLaneOutput(kFilterLane, numSamples);
    }

    if (fx2Step.active && fx2Step.presetIndex > 0 && isLaneActive(kFX2Lane))
    {
        captureLaneInput(numSamples);
        processFxLane(wetLeft, wetRight, numSamples, kFX2Lane, fx2Step.presetIndex, fx2Slot,
                      fx2DelayEngine, fx2ReverbEngine, fx2BitcrushEngine, fx2PitchEngine, fx2ToneFilter,
                      bpm, stepPhaseStart, phaseDelta);
        blendLaneOutput(kFX2Lane, numSamples);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        const auto index = static_cast<size_t>(i);
        const float outLeft = blendGlobalMixSample(dryLeft[index],
                                                   wetLeft[i],
                                                   globalMixMode,
                                                   globalDryWet) * outputGain;

        const float outRight = blendGlobalMixSample(dryRight[index],
                                                    wetRight[i],
                                                    globalMixMode,
                                                    globalDryWet) * outputGain;

        if (hasRightChannel)
        {
            leftChannel[i] = outLeft;
            rightChannel[i] = outRight;
        }
        else
        {
            leftChannel[i] = 0.5f * (outLeft + outRight);
        }
    }
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    auto* existingEditor = getActiveEditor();
    debugProcessorLog("createEditor processor=" + juce::String::toHexString(static_cast<juce::int64>(reinterpret_cast<std::uintptr_t>(this)))
                      + " activeEditor=" + juce::String::toHexString(static_cast<juce::int64>(reinterpret_cast<std::uintptr_t>(existingEditor))));

    auto* editor = new PluginEditor(*this);
    debugProcessorLog("createEditor produced editor#" + juce::String(editor->getInstanceId())
                      + " ptr=" + juce::String::toHexString(static_cast<juce::int64>(reinterpret_cast<std::uintptr_t>(editor))));
    return editor;
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
    juce::ignoreUnused(index);
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto stateTree = exportFullState();
    std::unique_ptr<juce::XmlElement> xml(stateTree.createXml());
    copyXmlToBinary(*xml, destData);
}

juce::ValueTree PluginProcessor::exportFullState()
{
    auto stateTree = state.getValueTreeState().copyState();
    auto existingSequencer = stateTree.getChildWithName("SequencerState");
    if (existingSequencer.isValid())
        stateTree.removeChild(existingSequencer, nullptr);
    stateTree.addChild(sequencerState.toValueTree(), -1, nullptr);
    return stateTree;
}

void PluginProcessor::applyFullState(const juce::ValueTree& stateTree)
{
    if (!stateTree.isValid())
        return;

    auto fullTree = stateTree.createCopy();
    auto seqChild = fullTree.getChildWithName("SequencerState");

    if (seqChild.isValid())
        fullTree.removeChild(seqChild, nullptr);

    if (fullTree.hasType(state.getValueTreeState().state.getType()))
        state.getValueTreeState().replaceState(fullTree);

    if (seqChild.isValid())
        sequencerState.fromValueTree(seqChild);

    lastEffectiveSteps.fill(-1);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr)
        return;

    auto fullTree = juce::ValueTree::fromXml(*xmlState);
    applyFullState(fullTree);
}

} // namespace zikada

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new zikada::PluginProcessor();
}
