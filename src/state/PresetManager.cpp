#include "state/PresetManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <unordered_set>

namespace zikada {

namespace {

struct FactoryPatternRecipe
{
    const char* name;
    const char* category;
    const char* subtitle;
    std::array<std::uint16_t, SequencerState::NumLanes> stepMasks;
    std::array<int, SequencerState::NumLanes> primaryPresets;
    std::array<int, SequencerState::NumLanes> alternatePresets;
    float dryWet;
    float outputGain;
    float tempo;
    int stepResolution;
    int mixMode;
};

std::uint16_t makeStepMask(std::initializer_list<int> steps)
{
    std::uint16_t mask = 0;
    for (const int step : steps)
        if (step >= 0 && step < SequencerState::NumSteps)
            mask |= static_cast<std::uint16_t>(1u << step);
    return mask;
}

const std::array<FactoryPatternRecipe, 42>& getAdditionalFactoryRecipes()
{
    static const std::array<FactoryPatternRecipe, 42> recipes{{
        {"CIRCUIT TEETH", "Glitch", "Factory - sharp slice and crush syncopation",
         {makeStepMask({0, 3, 7, 8, 11, 15}), makeStepMask({4, 12}), makeStepMask({0, 8}), makeStepMask({3, 11}), makeStepMask({2, 6, 10, 14}), makeStepMask({1, 5, 9, 13})},
         {20, 9, 12, 18, 10, 6}, {18, 10, 18, 13, 16, 17}, 92.0f, -1.0f, 124.0f, 0, 0},
        {"SIGNAL SKIP", "Glitch", "Factory - missing-beat loop and gate fragments",
         {makeStepMask({2, 6, 10, 14}), makeStepMask({0, 7, 8, 15}), makeStepMask({1, 5, 9, 13}), makeStepMask({4, 12}), makeStepMask({3, 11}), makeStepMask({6, 14})},
         {11, 5, 12, 16, 9, 10}, {17, 9, 18, 15, 15, 18}, 88.0f, -0.5f, 118.0f, 0, 1},
        {"PIXEL RIOT", "Glitch", "Factory - dense digital cuts and pitch debris",
         {makeStepMask({0, 2, 5, 7, 8, 10, 13, 15}), makeStepMask({3, 11}), makeStepMask({0, 4, 8, 12}), makeStepMask({2, 6, 10, 14}), makeStepMask({5, 13}), makeStepMask({1, 3, 9, 11})},
         {18, 14, 17, 18, 17, 17}, {20, 18, 12, 20, 19, 18}, 96.0f, -2.0f, 132.0f, 0, 2},
        {"BROKEN CLOCK", "Glitch", "Factory - displaced repeats around an uneven pulse",
         {makeStepMask({0, 5, 9, 14}), makeStepMask({1, 4, 10, 13}), makeStepMask({3, 7, 11, 15}), makeStepMask({6, 14}), makeStepMask({2, 8, 12}), makeStepMask({5, 15})},
         {12, 13, 15, 14, 11, 9}, {19, 17, 16, 16, 17, 14}, 84.0f, -0.5f, 111.0f, 1, 3},
        {"DIGITAL SHIVER", "Glitch", "Factory - nervous tremolo and bit-crushed flicker",
         {makeStepMask({1, 5, 9, 13}), makeStepMask({6, 14}), makeStepMask({0, 2, 4, 6, 8, 10, 12, 14}), makeStepMask({3, 7, 11, 15}), makeStepMask({4, 12}), makeStepMask({2, 6, 10, 14})},
         {15, 10, 15, 16, 10, 6}, {20, 18, 16, 15, 16, 13}, 90.0f, -1.5f, 127.0f, 0, 4},
        {"STUTTER CODE", "Glitch", "Factory - rapid slice repeats with gated tails",
         {makeStepMask({0, 1, 3, 4, 7, 8, 9, 12, 15}), makeStepMask({2, 6, 10, 14}), makeStepMask({0, 4, 8, 12}), makeStepMask({5, 13}), makeStepMask({3, 11}), makeStepMask({7, 15})},
         {20, 5, 12, 18, 8, 10}, {19, 9, 18, 20, 16, 18}, 94.0f, -1.0f, 140.0f, 0, 5},

        {"OFFBEAT ENGINE", "Groove", "Factory - clipped accents between the main beats",
         {makeStepMask({2, 6, 10, 14}), makeStepMask({3, 11}), makeStepMask({0, 8}), makeStepMask({2, 10}), makeStepMask({6, 14}), makeStepMask({5, 13})},
         {8, 6, 17, 17, 9, 13}, {11, 10, 18, 15, 15, 15}, 78.0f, 0.0f, 120.0f, 1, 0},
        {"POCKET CUTTER", "Groove", "Factory - sparse slices with warm driven answers",
         {makeStepMask({0, 4, 7, 8, 12, 15}), makeStepMask({6, 14}), makeStepMask({3, 11}), makeStepMask({4, 12}), makeStepMask({2, 10}), makeStepMask({7, 15})},
         {11, 7, 9, 17, 5, 15}, {17, 11, 17, 9, 13, 17}, 76.0f, 0.5f, 116.0f, 1, 1},
        {"GHOST NOTES", "Groove", "Factory - quiet gaps filled by short envelope taps",
         {makeStepMask({3, 7, 11, 15}), makeStepMask({0, 8}), makeStepMask({1, 5, 9, 13}), makeStepMask({6, 14}), makeStepMask({4, 12}), makeStepMask({2, 10})},
         {9, 8, 9, 9, 13, 10}, {13, 12, 18, 15, 15, 13}, 72.0f, -0.5f, 108.0f, 1, 2},
        {"FOUR FLOOR", "Groove", "Factory - straight quarter-note processing anchors",
         {makeStepMask({0, 4, 8, 12}), makeStepMask({2, 10}), makeStepMask({0, 4, 8, 12}), makeStepMask({4, 12}), makeStepMask({0, 8}), makeStepMask({6, 14})},
         {8, 7, 17, 17, 5, 11}, {11, 11, 12, 18, 12, 19}, 74.0f, 0.0f, 124.0f, 1, 3},
        {"SYNC BUG", "Groove", "Factory - syncopated loop and comb conversation",
         {makeStepMask({0, 3, 6, 10, 12, 15}), makeStepMask({1, 5, 9, 13}), makeStepMask({2, 10}), makeStepMask({3, 11}), makeStepMask({4, 8, 12}), makeStepMask({6, 14})},
         {12, 6, 15, 13, 11, 13}, {18, 10, 16, 14, 17, 15}, 82.0f, -0.5f, 126.0f, 1, 4},
        {"BACKBEAT LASER", "Groove", "Factory - bright answers on the backbeat",
         {makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({4, 12}), makeStepMask({4, 12}), makeStepMask({2, 6, 10, 14}), makeStepMask({3, 7, 11, 15})},
         {11, 10, 17, 10, 9, 18}, {17, 14, 18, 12, 15, 20}, 80.0f, 0.5f, 122.0f, 1, 5},

        {"RATCHET LOOP", "Loop", "Factory - accelerating short-loop accents",
         {makeStepMask({0, 8}), makeStepMask({0, 3, 4, 7, 8, 11, 12, 15}), makeStepMask({2, 6, 10, 14}), makeStepMask({5, 13}), makeStepMask({4, 12}), makeStepMask({7, 15})},
         {8, 13, 18, 11, 10, 10}, {12, 14, 12, 12, 16, 18}, 86.0f, -1.0f, 128.0f, 0, 0},
        {"BACKSPIN GRID", "Loop", "Factory - alternating reverse-loop windows",
         {makeStepMask({2, 10}), makeStepMask({0, 4, 8, 12}), makeStepMask({0, 8}), makeStepMask({3, 11}), makeStepMask({6, 14}), makeStepMask({5, 13})},
         {17, 9, 12, 14, 11, 8}, {18, 10, 18, 16, 17, 9}, 84.0f, -0.5f, 120.0f, 1, 1},
        {"HALF TIME HAZE", "Loop", "Factory - slow loops under a washed texture",
         {makeStepMask({0, 8}), makeStepMask({0, 8}), makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({2, 10}), makeStepMask({6, 14})},
         {16, 15, 10, 8, 13, 19}, {17, 16, 13, 10, 18, 20}, 68.0f, -1.5f, 92.0f, 2, 2},
        {"REVERSE POCKET", "Loop", "Factory - reverse loops tucked into the groove",
         {makeStepMask({3, 11}), makeStepMask({2, 6, 10, 14}), makeStepMask({4, 12}), makeStepMask({0, 8}), makeStepMask({5, 13}), makeStepMask({7, 15})},
         {17, 9, 9, 15, 10, 13}, {18, 11, 18, 16, 16, 15}, 80.0f, 0.0f, 114.0f, 1, 3},
        {"LOOP LADDER", "Loop", "Factory - loop sizes climb across each bar",
         {makeStepMask({1, 5, 9, 13}), makeStepMask({0, 4, 8, 12}), makeStepMask({2, 10}), makeStepMask({6, 14}), makeStepMask({3, 11}), makeStepMask({7, 15})},
         {10, 5, 17, 9, 15, 11}, {12, 8, 19, 11, 19, 12}, 82.0f, -0.5f, 125.0f, 1, 4},
        {"TAIL SPIRAL", "Loop", "Factory - long loop tails folding into space",
         {makeStepMask({0, 8}), makeStepMask({0, 12}), makeStepMask({4, 12}), makeStepMask({3, 11}), makeStepMask({2, 10}), makeStepMask({6, 14})},
         {13, 19, 10, 8, 13, 19}, {17, 20, 13, 12, 18, 20}, 74.0f, -2.0f, 104.0f, 2, 5},

        {"ACID CICADA", "Filter", "Factory - resonant low-pass steps with sharp gates",
         {makeStepMask({0, 4, 8, 12}), makeStepMask({6, 14}), makeStepMask({0, 4, 8, 12}), makeStepMask({3, 11}), makeStepMask({0, 2, 4, 6, 8, 10, 12, 14}), makeStepMask({5, 13})},
         {11, 6, 12, 17, 6, 13}, {15, 10, 18, 18, 13, 15}, 88.0f, -1.0f, 132.0f, 0, 0},
        {"LOWPASS DRIFT", "Filter", "Factory - soft alternating low-pass motion",
         {makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({2, 10}), makeStepMask({6, 14}), makeStepMask({0, 4, 8, 12}), makeStepMask({3, 11})},
         {13, 8, 13, 9, 5, 16}, {16, 12, 19, 10, 13, 19}, 70.0f, -0.5f, 110.0f, 1, 1},
        {"RESONANT STEPS", "Filter", "Factory - stepped peaks across the filter lane",
         {makeStepMask({2, 6, 10, 14}), makeStepMask({0, 8}), makeStepMask({3, 11}), makeStepMask({5, 13}), makeStepMask({0, 2, 5, 7, 8, 10, 13, 15}), makeStepMask({4, 12})},
         {9, 7, 17, 13, 9, 15}, {14, 11, 18, 14, 19, 18}, 86.0f, -1.0f, 126.0f, 0, 2},
        {"COMB RUNNER", "Filter", "Factory - rhythmic comb tones chase the downbeat",
         {makeStepMask({0, 4, 8, 12}), makeStepMask({2, 10}), makeStepMask({1, 5, 9, 13}), makeStepMask({6, 14}), makeStepMask({0, 3, 6, 9, 12, 15}), makeStepMask({7, 15})},
         {8, 6, 15, 13, 11, 13}, {12, 10, 16, 14, 17, 15}, 82.0f, -1.5f, 128.0f, 0, 3},
        {"NOTCH PARADE", "Filter", "Factory - moving notch accents in a wide pattern",
         {makeStepMask({3, 7, 11, 15}), makeStepMask({0, 8}), makeStepMask({2, 6, 10, 14}), makeStepMask({4, 12}), makeStepMask({0, 4, 8, 12}), makeStepMask({5, 13})},
         {12, 10, 16, 14, 10, 18}, {17, 14, 19, 16, 16, 20}, 78.0f, -0.5f, 119.0f, 1, 4},
        {"BAND SCANNER", "Filter", "Factory - narrow band-pass sweeps scan the bar",
         {makeStepMask({1, 5, 9, 13}), makeStepMask({3, 11}), makeStepMask({0, 8}), makeStepMask({2, 10}), makeStepMask({0, 3, 5, 8, 11, 13}), makeStepMask({6, 14})},
         {10, 8, 11, 11, 9, 15}, {14, 12, 19, 14, 15, 18}, 84.0f, -1.0f, 123.0f, 1, 5},

        {"PHOSPHOR RAIN", "Delay", "Factory - sparse echoes glowing behind soft cuts",
         {makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({0, 8}), makeStepMask({3, 11}), makeStepMask({2, 10}), makeStepMask({6, 14})},
         {13, 8, 10, 8, 13, 19}, {17, 12, 13, 10, 18, 20}, 66.0f, -2.0f, 98.0f, 2, 0},
        {"NIGHT TRANSMISSION", "Delay", "Factory - distant modulated delay signals",
         {makeStepMask({0, 12}), makeStepMask({6, 14}), makeStepMask({4, 12}), makeStepMask({2, 10}), makeStepMask({0, 8}), makeStepMask({3, 11})},
         {16, 10, 13, 6, 15, 11}, {18, 12, 19, 10, 19, 19}, 64.0f, -1.5f, 102.0f, 2, 1},
        {"ORBITAL DUST", "Ambient", "Factory - slow pitch color in a filtered orbit",
         {makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({2, 10}), makeStepMask({6, 14}), makeStepMask({1, 9}), makeStepMask({3, 11})},
         {15, 15, 10, 19, 13, 18}, {17, 16, 13, 20, 18, 20}, 70.0f, -2.5f, 88.0f, 2, 2},
        {"DUB BLOOM", "Delay", "Factory - delay taps opening through low-pass space",
         {makeStepMask({0, 8}), makeStepMask({2, 10}), makeStepMask({4, 12}), makeStepMask({0, 6, 8, 14}), makeStepMask({3, 11}), makeStepMask({5, 13})},
         {11, 8, 13, 5, 5, 11}, {16, 12, 19, 8, 13, 19}, 74.0f, -1.0f, 112.0f, 1, 3},
        {"EMPTY STATION", "Ambient", "Factory - minimal repeats with a long dark tail",
         {makeStepMask({0, 12}), makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({6, 14}), makeStepMask({2, 10}), makeStepMask({7, 15})},
         {17, 19, 10, 8, 16, 19}, {18, 20, 13, 12, 18, 20}, 58.0f, -3.0f, 76.0f, 3, 4},
        {"AFTERGLOW", "Ambient", "Factory - gentle filtered reverb after each phrase",
         {makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({2, 10}), makeStepMask({3, 11}), makeStepMask({0, 8}), makeStepMask({6, 14})},
         {13, 8, 10, 8, 13, 20}, {16, 12, 13, 10, 18, 19}, 62.0f, -2.0f, 96.0f, 2, 5},

        {"BIT DUST", "Texture", "Factory - light bit reduction scattered over the bar",
         {makeStepMask({0, 4, 8, 12}), makeStepMask({6, 14}), makeStepMask({2, 10}), makeStepMask({5, 13}), makeStepMask({3, 11}), makeStepMask({1, 5, 9, 13})},
         {9, 6, 17, 17, 9, 5}, {13, 10, 18, 18, 15, 17}, 72.0f, -1.0f, 121.0f, 1, 0},
        {"RING STATIC", "Texture", "Factory - metallic ring modulation sparks",
         {makeStepMask({2, 6, 10, 14}), makeStepMask({0, 8}), makeStepMask({3, 11}), makeStepMask({4, 12}), makeStepMask({5, 13}), makeStepMask({1, 7, 9, 15})},
         {12, 10, 15, 14, 10, 13}, {18, 14, 16, 16, 16, 14}, 78.0f, -2.0f, 129.0f, 0, 1},
        {"CRUSH BLOSSOM", "Texture", "Factory - crushed transients opening into reverb",
         {makeStepMask({0, 4, 8, 12}), makeStepMask({2, 10}), makeStepMask({0, 8}), makeStepMask({3, 11}), makeStepMask({6, 14}), makeStepMask({1, 5, 9, 13})},
         {11, 7, 17, 18, 13, 17}, {17, 11, 18, 20, 18, 19}, 82.0f, -1.5f, 124.0f, 1, 2},
        {"PITCH MOSAIC", "Texture", "Factory - alternating pitch colors form a pattern",
         {makeStepMask({1, 5, 9, 13}), makeStepMask({3, 11}), makeStepMask({0, 4, 8, 12}), makeStepMask({2, 6, 10, 14}), makeStepMask({4, 12}), makeStepMask({0, 3, 8, 11})},
         {10, 9, 19, 19, 15, 7}, {14, 13, 20, 20, 19, 8}, 76.0f, -1.0f, 117.0f, 1, 3},
        {"SPACE DEBRIS", "Delay", "Factory - fragmented delays drifting through space",
         {makeStepMask({0, 5, 8, 13}), makeStepMask({2, 10}), makeStepMask({4, 12}), makeStepMask({1, 7, 9, 15}), makeStepMask({3, 11}), makeStepMask({6, 14})},
         {18, 10, 13, 6, 16, 19}, {20, 14, 19, 12, 18, 20}, 74.0f, -2.0f, 106.0f, 1, 4},
        {"DIGITAL RUST", "Texture", "Factory - rough drive, notch and dark digital grain",
         {makeStepMask({0, 3, 7, 8, 11, 15}), makeStepMask({4, 12}), makeStepMask({2, 10}), makeStepMask({1, 5, 9, 13}), makeStepMask({6, 14}), makeStepMask({3, 7, 11, 15})},
         {12, 9, 18, 17, 10, 6}, {19, 13, 12, 18, 16, 17}, 88.0f, -2.5f, 133.0f, 0, 5},

        {"WOBBLE BUS", "Motion", "Factory - envelope wobble drives alternating colors",
         {makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({0, 2, 4, 6, 8, 10, 12, 14}), makeStepMask({3, 11}), makeStepMask({2, 10}), makeStepMask({1, 5, 9, 13})},
         {11, 8, 16, 10, 5, 10}, {17, 12, 15, 15, 13, 18}, 84.0f, -1.0f, 122.0f, 1, 0},
        {"TREMOR FIELD", "Motion", "Factory - chopped tremolo ripples across the lanes",
         {makeStepMask({2, 6, 10, 14}), makeStepMask({0, 8}), makeStepMask({0, 4, 8, 12}), makeStepMask({1, 5, 9, 13}), makeStepMask({3, 11}), makeStepMask({6, 14})},
         {9, 6, 15, 15, 9, 13}, {13, 10, 16, 16, 15, 15}, 80.0f, -1.0f, 126.0f, 0, 1},
        {"PULSE ENGINE", "Motion", "Factory - square gates pulse against modulated delay",
         {makeStepMask({0, 4, 8, 12}), makeStepMask({2, 10}), makeStepMask({0, 4, 8, 12}), makeStepMask({3, 7, 11, 15}), makeStepMask({6, 14}), makeStepMask({1, 9})},
         {8, 7, 12, 6, 10, 11}, {11, 11, 18, 10, 16, 19}, 86.0f, -0.5f, 130.0f, 0, 2},
        {"SLOW SWARM", "Motion", "Factory - slow envelopes and chorus move as one",
         {makeStepMask({0, 8}), makeStepMask({4, 12}), makeStepMask({0, 8}), makeStepMask({2, 10}), makeStepMask({6, 14}), makeStepMask({3, 11})},
         {13, 15, 10, 9, 13, 19}, {17, 16, 13, 10, 18, 20}, 68.0f, -1.5f, 84.0f, 2, 3},
        {"PANIC SIGNAL", "Motion", "Factory - urgent tremolo, filter and pitch flashes",
         {makeStepMask({0, 2, 5, 7, 8, 10, 13, 15}), makeStepMask({3, 11}), makeStepMask({1, 5, 9, 13}), makeStepMask({0, 4, 8, 12}), makeStepMask({2, 6, 10, 14}), makeStepMask({3, 7, 11, 15})},
         {18, 14, 15, 16, 9, 18}, {20, 18, 16, 15, 19, 20}, 94.0f, -2.0f, 144.0f, 0, 4},
        {"GLIDE MATRIX", "Delay", "Factory - gliding envelopes connect filtered echoes",
         {makeStepMask({0, 4, 8, 12}), makeStepMask({2, 10}), makeStepMask({1, 5, 9, 13}), makeStepMask({3, 11}), makeStepMask({0, 8}), makeStepMask({6, 14})},
         {11, 8, 19, 6, 15, 18}, {16, 12, 13, 10, 19, 20}, 78.0f, -1.0f, 115.0f, 1, 5},
    }};

    return recipes;
}

juce::String sanitizeName(const juce::String& name)
{
    auto trimmed = name.trim();
    juce::String result;

    for (auto character : trimmed)
    {
        if (juce::CharacterFunctions::isLetterOrDigit(character))
            result << character;
        else if (character == ' ' || character == '-' || character == '_')
            result << '_';
    }

    return result.isEmpty() ? juce::String("preset") : result;
}

juce::File resolvePresetDirectory(const juce::File& overrideDirectory)
{
    if (overrideDirectory.getFullPathName().isNotEmpty())
        return overrideDirectory;

    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("ZIKADARATOR")
        .getChildFile("Presets");
}

juce::String makeFactoryPresetId(const juce::String& name)
{
    return "factory:" + name.trim().toUpperCase();
}

juce::String makeUserPresetId(const juce::String& name)
{
    return "user-file:" + name.trim().toUpperCase();
}

std::unordered_set<std::string> getExpectedParameterIds()
{
    std::unordered_set<std::string> ids{
        ParameterIDs::dryWet.toStdString(),
        ParameterIDs::outputGain.toStdString(),
        ParameterIDs::mixMode.toStdString(),
        ParameterIDs::clockSource.toStdString(),
        ParameterIDs::tempo.toStdString(),
        ParameterIDs::stepResolution.toStdString(),
        ParameterIDs::bypass.toStdString()
    };

    for (int lane = 0; lane < SequencerState::NumLanes; ++lane)
    {
        ids.insert(getLaneMixID(lane).toStdString());
        ids.insert(getLaneMuteID(lane).toStdString());
        ids.insert(getLaneSoloID(lane).toStdString());
        for (int step = 0; step < SequencerState::NumSteps; ++step)
            ids.insert(getStepActiveID(lane, step).toStdString());
    }

    return ids;
}

bool isParameterValueInRange(const juce::String& id, double value)
{
    if (id == ParameterIDs::dryWet)
        return value >= 0.0 && value <= 100.0;
    if (id == ParameterIDs::outputGain)
        return value >= -24.0 && value <= 24.0;
    if (id == ParameterIDs::mixMode)
        return value >= 0.0 && value <= 5.0;
    if (id == ParameterIDs::clockSource || id == ParameterIDs::bypass
        || id.startsWith("stepActive_") || id.startsWith("laneMute_") || id.startsWith("laneSolo_"))
        return value >= 0.0 && value <= 1.0;
    if (id == ParameterIDs::tempo)
        return value >= 20.0 && value <= 300.0;
    if (id == ParameterIDs::stepResolution)
        return value >= 0.0 && value <= 3.0;
    if (id.startsWith("laneMix_"))
        return value >= 0.0 && value <= 100.0;
    return false;
}

bool isCompletePresetState(const juce::ValueTree& state)
{
    if (!state.isValid() || !state.hasType(JucePlugin_Name))
        return false;

    auto missingParameterIds = getExpectedParameterIds();
    int parameterCount = 0;
    int sequencerCount = 0;
    juce::ValueTree sequencer;

    for (const auto& child : state)
    {
        if (child.hasType(parameterStateTreeType))
        {
            ++parameterCount;
            const auto id = child.getProperty(parameterStateIdProperty).toString().toStdString();
            const juce::String juceId(id);
            const auto valueText = child.getProperty(parameterStateValueProperty).toString().trim();
            const auto* valueStart = valueText.toRawUTF8();
            char* valueEnd = nullptr;
            const double value = std::strtod(valueStart, &valueEnd);
            if (missingParameterIds.erase(id) != 1 || valueEnd == valueStart
                || valueEnd == nullptr || *valueEnd != '\0' || !std::isfinite(value)
                || !isParameterValueInRange(juceId, value))
                return false;
        }
        else if (child.hasType("SequencerState"))
        {
            ++sequencerCount;
            sequencer = child;
        }
        else
        {
            return false;
        }
    }

    if (parameterCount != static_cast<int>(getExpectedParameterIds().size())
        || !missingParameterIds.empty() || sequencerCount != 1
        || sequencer.getNumChildren() != SequencerState::NumLanes)
        return false;

    for (int lane = 0; lane < SequencerState::NumLanes; ++lane)
    {
        const auto laneTree = sequencer.getChild(lane);
        if (!laneTree.hasType("Lane")
            || static_cast<int>(laneTree.getProperty("index", -1)) != lane
            || laneTree.getNumChildren() != 2)
            return false;

        const auto steps = laneTree.getChildWithName("Steps");
        const auto slots = laneTree.getChildWithName("UserSlots");
        if (!steps.isValid() || !slots.isValid()
            || steps.getNumChildren() != SequencerState::NumSteps
            || slots.getNumChildren() != SequencerState::NumUserSlots)
            return false;

        for (int step = 0; step < SequencerState::NumSteps; ++step)
        {
            const auto stepTree = steps.getChild(step);
            if (!stepTree.hasType("Step")
                || static_cast<int>(stepTree.getProperty("index", -1)) != step
                || !stepTree.hasProperty("active")
                || !stepTree.hasProperty("presetIndex")
                || !stepTree.hasProperty("chainLength")
                || static_cast<int>(stepTree.getProperty("presetIndex", -1)) < 0
                || static_cast<int>(stepTree.getProperty("presetIndex", -1)) > 20
                || static_cast<int>(stepTree.getProperty("chainLength", 0)) < 1
                || static_cast<int>(stepTree.getProperty("chainLength", 0)) > SequencerState::NumSteps - step)
                return false;
        }

        for (int slot = 0; slot < SequencerState::NumUserSlots; ++slot)
        {
            const auto slotTree = slots.getChild(slot);
            if (!slotTree.hasType("UserSlot")
                || static_cast<int>(slotTree.getProperty("index", -1)) != slot)
                return false;

            const auto slotData = UserSlotData::fromValueTree(slotTree);
            if (!std::isfinite(slotData.filterCutoff) || slotData.filterCutoff < 20.0f || slotData.filterCutoff > 20000.0f
                || !std::isfinite(slotData.filterResonance) || slotData.filterResonance < 0.1f || slotData.filterResonance > 20.0f
                || !std::isfinite(slotData.delayTime) || slotData.delayTime < 0.0f || slotData.delayTime > 12.0f
                || !std::isfinite(slotData.delayFeedback) || slotData.delayFeedback < 0.0f || slotData.delayFeedback > 0.99f
                || !std::isfinite(slotData.delayMix) || slotData.delayMix < 0.0f || slotData.delayMix > 1.0f
                || !std::isfinite(slotData.volume) || slotData.volume < 0.0f || slotData.volume > 2.0f
                || !std::isfinite(slotData.pan) || slotData.pan < -1.0f || slotData.pan > 1.0f)
                return false;
        }
    }

    return true;
}

void setStep(SequencerState& state, int lane, int step, int presetIndex)
{
    StepData data;
    data.active = true;
    data.presetIndex = presetIndex;
    state.setStepData(lane, step, data);
}

void setUserSlot(SequencerState& state, int lane, int slot, float cutoff, float resonance,
                 float delayTime, float delayFeedback, float delayMix, float volume, float pan)
{
    UserSlotData data;
    data.filterCutoff = cutoff;
    data.filterResonance = resonance;
    data.delayTime = delayTime;
    data.delayFeedback = delayFeedback;
    data.delayMix = delayMix;
    data.volume = volume;
    data.pan = pan;
    state.setUserSlot(lane, slot, data);
}

void appendSequencerState(juce::ValueTree& state, const SequencerState& sequencerState)
{
    const auto snapshot = sequencerState.getSnapshot();
    for (int lane = 0; lane < SequencerState::NumLanes; ++lane)
        for (int step = 0; step < SequencerState::NumSteps; ++step)
            setParameterStateValue(state,
                                   getStepActiveID(lane, step),
                                   snapshot.getStepData(lane, step).active ? 1.0f : 0.0f);

    state.setProperty(stateSchemaVersionProperty, currentStateSchemaVersion, nullptr);
    state.addChild(snapshot.toValueTree(), -1, nullptr);
}

juce::ValueTree createPatternFactoryState(juce::ValueTree state,
                                          const FactoryPatternRecipe& recipe,
                                          int recipeIndex)
{
    setParameterStateValue(state, ParameterIDs::dryWet, recipe.dryWet);
    setParameterStateValue(state, ParameterIDs::outputGain, recipe.outputGain);
    setParameterStateValue(state, ParameterIDs::tempo, recipe.tempo);
    setParameterStateValue(state, ParameterIDs::stepResolution, static_cast<float>(recipe.stepResolution));
    setParameterStateValue(state, ParameterIDs::mixMode, static_cast<float>(recipe.mixMode));

    SequencerState sequencerState;
    for (int lane = 0; lane < SequencerState::NumLanes; ++lane)
    {
        int eventIndex = 0;
        for (int step = 0; step < SequencerState::NumSteps; ++step)
        {
            const auto bit = static_cast<std::uint16_t>(1u << step);
            if ((recipe.stepMasks[static_cast<size_t>(lane)] & bit) == 0)
                continue;

            const int alternate = recipe.alternatePresets[static_cast<size_t>(lane)];
            const int preset = alternate >= 5 && (eventIndex % 2) != 0
                                   ? alternate
                                   : recipe.primaryPresets[static_cast<size_t>(lane)];
            setStep(sequencerState, lane, step, preset);
            ++eventIndex;
        }

        const int toneSeed = recipeIndex * SequencerState::NumLanes + lane;
        const float cutoff = 700.0f + static_cast<float>(toneSeed % 9) * 850.0f;
        const float resonance = 0.7f + static_cast<float>(toneSeed % 6) * 0.45f;
        const float delayTime = 0.07f + static_cast<float>(toneSeed % 7) * 0.045f;
        const float delayFeedback = 0.20f + static_cast<float>(toneSeed % 5) * 0.10f;
        const float delayMix = 0.30f + static_cast<float>(toneSeed % 4) * 0.10f;
        const float volume = 0.84f + static_cast<float>(toneSeed % 5) * 0.035f;
        const float pan = static_cast<float>((toneSeed % 7) - 3) * 0.10f;
        setUserSlot(sequencerState, lane, 0, cutoff, resonance, delayTime,
                    delayFeedback, delayMix, volume, pan);
    }

    appendSequencerState(state, sequencerState);
    return state;
}

} // namespace

PresetManager::PresetManager(juce::File presetDirectoryOverride)
    : presetDirectory(resolvePresetDirectory(presetDirectoryOverride)),
      metadataFile(presetDirectory.getChildFile("preset-metadata.xml"))
{
    presetDirectory.createDirectory();
    refresh();
}

void PresetManager::refresh()
{
    loadMetadata();
    items.clear();
    addFactoryPresets();
    addUserPresets();
    sortItems();
}

PresetManager::SaveResult PresetManager::saveUserPreset(const juce::String& name,
                                                        const juce::ValueTree& state)
{
    const auto trimmedName = name.trim();
    if (trimmedName.isEmpty())
        return SaveResult::InvalidName;
    if (!isCompletePresetState(state))
        return SaveResult::InvalidState;

    const auto normalizedName = normalizeUserPresetName(trimmedName);
    if (normalizedName.isEmpty())
        return SaveResult::InvalidName;

    for (const auto& item : items)
        if (item.isFactory && item.name.equalsIgnoreCase(normalizedName))
            return SaveResult::NameConflict;

    presetDirectory.createDirectory();
    const auto file = presetDirectory.getChildFile(sanitizeName(trimmedName) + ".xml");
    const bool updatingExistingPreset = file.existsAsFile();
    if (updatingExistingPreset)
    {
        const auto existingItem = std::find_if(items.begin(), items.end(), [&file](const PresetItem& item)
        {
            return !item.isFactory && item.file == file;
        });
        if (existingItem == items.end())
            return SaveResult::NameConflict;
    }

    auto xml = state.createXml();
    if (xml == nullptr)
        return SaveResult::WriteFailed;

    juce::TemporaryFile temporaryFile(file);
    if (!xml->writeTo(temporaryFile.getFile())
        || !temporaryFile.overwriteTargetFileWithTemporary())
        return SaveResult::WriteFailed;

    markPresetUsed(normalizedName);
    refresh();
    return updatingExistingPreset ? SaveResult::Updated : SaveResult::Saved;
}

juce::String PresetManager::normalizeUserPresetName(const juce::String& name)
{
    juce::StringArray words;
    words.addTokens(sanitizeName(name).replaceCharacter('_', ' '), " \t\r\n", "");
    words.removeEmptyStrings();
    return words.joinIntoString(" ").toUpperCase();
}

bool PresetManager::loadPreset(int index, juce::ValueTree& outState) const
{
    if (index < 0 || index >= static_cast<int>(items.size()))
        return false;

    outState = items[static_cast<size_t>(index)].state.createCopy();
    return outState.isValid();
}

bool PresetManager::deleteUserPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(items.size()))
        return false;

    const auto& item = items[static_cast<size_t>(index)];
    if (item.isFactory || !item.file.existsAsFile())
        return false;

    const bool removed = item.file.deleteFile();
    if (removed)
    {
        const auto metadataName = normalizeUserPresetName(item.file.getFileNameWithoutExtension());
        favoritePresetNames.removeString(metadataName);
        recentPresetNames.removeString(metadataName);
        saveMetadata();
        refresh();
    }

    return removed;
}

bool PresetManager::toggleFavorite(const juce::String& name)
{
    const auto normalized = name.trim().toUpperCase();
    if (normalized.isEmpty())
        return false;

    if (favoritePresetNames.contains(normalized))
        favoritePresetNames.removeString(normalized);
    else
        favoritePresetNames.addIfNotAlreadyThere(normalized);

    saveMetadata();
    refresh();
    return favoritePresetNames.contains(normalized);
}

bool PresetManager::isFavorite(const juce::String& name) const
{
    return favoritePresetNames.contains(name.trim().toUpperCase());
}

void PresetManager::markPresetUsed(const juce::String& name)
{
    const auto normalized = name.trim().toUpperCase();
    if (normalized.isEmpty())
        return;

    recentPresetNames.removeString(normalized);
    recentPresetNames.insert(0, normalized);
    while (recentPresetNames.size() > 12)
        recentPresetNames.remove(recentPresetNames.size() - 1);

    saveMetadata();
}

int PresetManager::findItemIndexById(const juce::String& id) const
{
    if (id.isEmpty())
        return -1;

    for (int index = 0; index < static_cast<int>(items.size()); ++index)
        if (items[static_cast<size_t>(index)].id == id)
            return index;

    return -1;
}

void PresetManager::addFactoryPresets()
{
    items.push_back({"INIT", "Utility", "Factory - clean starting point", true, isFavorite("INIT"), recentPresetNames.indexOf("INIT"), {}, createInitFactoryState(), makeFactoryPresetId("INIT")});
    items.push_back({"NEON GATE", "Glitch", "Factory - gated stutter rhythm", true, isFavorite("NEON GATE"), recentPresetNames.indexOf("NEON GATE"), {}, createNeonGateFactoryState(), makeFactoryPresetId("NEON GATE")});
    items.push_back({"SPACE BLOOM", "Ambient", "Factory - airy delay and filter trail", true, isFavorite("SPACE BLOOM"), recentPresetNames.indexOf("SPACE BLOOM"), {}, createSpaceBloomFactoryState(), makeFactoryPresetId("SPACE BLOOM")});
    items.push_back({"DELAY PULSE", "Delay", "Factory - synced delay pattern", true, isFavorite("DELAY PULSE"), recentPresetNames.indexOf("DELAY PULSE"), {}, createDelayPulseFactoryState(), makeFactoryPresetId("DELAY PULSE")});
    items.push_back({"FILTER CUTS", "Filter", "Factory - stepped filter movement", true, isFavorite("FILTER CUTS"), recentPresetNames.indexOf("FILTER CUTS"), {}, createFilterCutsFactoryState(), makeFactoryPresetId("FILTER CUTS")});
    items.push_back({"CRUSH GRID", "Texture", "Factory - bitcrush and drive rhythm", true, isFavorite("CRUSH GRID"), recentPresetNames.indexOf("CRUSH GRID"), {}, createCrushGridFactoryState(), makeFactoryPresetId("CRUSH GRID")});
    items.push_back({"LOOP CHOP", "Loop", "Factory - micro-loop cuts", true, isFavorite("LOOP CHOP"), recentPresetNames.indexOf("LOOP CHOP"), {}, createLoopChopFactoryState(), makeFactoryPresetId("LOOP CHOP")});
    items.push_back({"NOTCH MOTION", "Filter", "Factory - notch and tremolo motion", true, isFavorite("NOTCH MOTION"), recentPresetNames.indexOf("NOTCH MOTION"), {}, createNotchMotionFactoryState(), makeFactoryPresetId("NOTCH MOTION")});

    const auto& recipes = getAdditionalFactoryRecipes();
    for (int recipeIndex = 0; recipeIndex < static_cast<int>(recipes.size()); ++recipeIndex)
    {
        const auto& recipe = recipes[static_cast<size_t>(recipeIndex)];
        const juce::String name(recipe.name);
        items.push_back({name,
                         recipe.category,
                         recipe.subtitle,
                         true,
                         isFavorite(name),
                         recentPresetNames.indexOf(name),
                         {},
                         createPatternFactoryState(createBaseState(), recipe, recipeIndex),
                         makeFactoryPresetId(name)});
    }
}

void PresetManager::addUserPresets()
{
    const auto files = presetDirectory.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (const auto& file : files)
    {
        if (file == metadataFile)
            continue;

        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr)
            continue;

        auto state = juce::ValueTree::fromXml(*xml);
        if (!isCompletePresetState(state))
            continue;

        const auto basePresetName = normalizeUserPresetName(file.getFileNameWithoutExtension());
        if (basePresetName.isEmpty())
            continue;

        auto presetName = basePresetName;
        int suffix = 1;
        while (std::any_of(items.begin(), items.end(), [&presetName](const PresetItem& item)
        {
            return item.name.equalsIgnoreCase(presetName);
        }))
        {
            presetName = basePresetName + (suffix == 1 ? " (USER)" : " (USER " + juce::String(suffix) + ")");
            ++suffix;
        }

        items.push_back({presetName,
                         "User",
                         "User - " + file.getFullPathName(),
                         false,
                         isFavorite(basePresetName),
                         recentPresetNames.indexOf(basePresetName),
                         file,
                         state,
                         makeUserPresetId(file.getFileName())});
    }
}

void PresetManager::loadMetadata()
{
    favoritePresetNames.clear();
    recentPresetNames.clear();

    if (!metadataFile.existsAsFile())
        return;

    auto xml = juce::XmlDocument::parse(metadataFile);
    if (xml == nullptr)
        return;

    auto root = juce::ValueTree::fromXml(*xml);
    if (!root.isValid())
        return;

    auto favorites = root.getChildWithName("Favorites");
    for (int i = 0; i < favorites.getNumChildren(); ++i)
    {
        auto child = favorites.getChild(i);
        favoritePresetNames.addIfNotAlreadyThere(child.getProperty("name").toString());
    }

    auto recent = root.getChildWithName("Recent");
    for (int i = 0; i < recent.getNumChildren(); ++i)
    {
        auto child = recent.getChild(i);
        recentPresetNames.addIfNotAlreadyThere(child.getProperty("name").toString());
    }
}

void PresetManager::saveMetadata() const
{
    juce::ValueTree root("PresetMetadata");
    juce::ValueTree favorites("Favorites");
    juce::ValueTree recent("Recent");

    for (const auto& name : favoritePresetNames)
    {
        juce::ValueTree child("Preset");
        child.setProperty("name", name, nullptr);
        favorites.addChild(child, -1, nullptr);
    }

    for (const auto& name : recentPresetNames)
    {
        juce::ValueTree child("Preset");
        child.setProperty("name", name, nullptr);
        recent.addChild(child, -1, nullptr);
    }

    root.addChild(favorites, -1, nullptr);
    root.addChild(recent, -1, nullptr);

    if (auto xml = root.createXml())
        xml->writeTo(metadataFile);
}

void PresetManager::sortItems()
{
    std::stable_sort(items.begin(), items.end(), [](const PresetItem& a, const PresetItem& b)
    {
        if (a.isFactory != b.isFactory)
            return a.isFactory > b.isFactory;

        if (a.isFactory)
            return false;

        return a.name < b.name;
    });
}

juce::ValueTree PresetManager::createBaseState()
{
    juce::ValueTree state(JucePlugin_Name);
    state.setProperty(stateSchemaVersionProperty, currentStateSchemaVersion, nullptr);
    setParameterStateValue(state, ParameterIDs::dryWet, 100.0f);
    setParameterStateValue(state, ParameterIDs::outputGain, 0.0f);
    setParameterStateValue(state, ParameterIDs::mixMode, 0.0f);
    setParameterStateValue(state, ParameterIDs::clockSource, 0.0f);
    setParameterStateValue(state, ParameterIDs::tempo, 120.0f);
    setParameterStateValue(state, ParameterIDs::stepResolution, 1.0f);
    setParameterStateValue(state, ParameterIDs::bypass, 0.0f);

    for (int lane = 0; lane < 6; ++lane)
    {
        setParameterStateValue(state, getLaneMixID(lane), 100.0f);
        setParameterStateValue(state, getLaneMuteID(lane), 0.0f);
        setParameterStateValue(state, getLaneSoloID(lane), 0.0f);
        for (int step = 0; step < 16; ++step)
            setParameterStateValue(state, getStepActiveID(lane, step), 0.0f);
    }

    return state;
}

juce::ValueTree PresetManager::createInitFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;
    appendSequencerState(state, sequencerState);
    return state;
}

juce::ValueTree PresetManager::createNeonGateFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;

    for (int step = 0; step < 16; step += 2)
        setStep(sequencerState, 1, step, 8);

    for (int step = 0; step < 16; step += 4)
        setStep(sequencerState, 2, step, 8);

    setStep(sequencerState, 3, 4, 5);
    setStep(sequencerState, 3, 5, 5);
    setStep(sequencerState, 3, 6, 10);
    setStep(sequencerState, 4, 10, 5);
    setStep(sequencerState, 4, 11, 12);

    setUserSlot(sequencerState, 1, 0, 1800.0f, 0.707f, 0.08f, 0.72f, 0.7f, 1.0f, 0.0f);
    setUserSlot(sequencerState, 2, 0, 1200.0f, 1.4f, 0.12f, 0.28f, 0.45f, 1.0f, 0.0f);
    setUserSlot(sequencerState, 3, 0, 4200.0f, 0.6f, 0.24f, 0.45f, 0.42f, 1.0f, -0.15f);
    setUserSlot(sequencerState, 4, 0, 900.0f, 2.2f, 0.15f, 0.35f, 0.5f, 1.0f, 0.0f);

    appendSequencerState(state, sequencerState);
    return state;
}

juce::ValueTree PresetManager::createSpaceBloomFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;

    setStep(sequencerState, 0, 0, 10);
    setStep(sequencerState, 0, 4, 11);
    setStep(sequencerState, 0, 8, 12);
    setStep(sequencerState, 0, 12, 9);

    setStep(sequencerState, 3, 3, 6);
    setStep(sequencerState, 3, 7, 5);
    setStep(sequencerState, 4, 8, 5);
    setStep(sequencerState, 4, 9, 7);
    setStep(sequencerState, 5, 12, 6);
    setStep(sequencerState, 5, 13, 9);

    setUserSlot(sequencerState, 0, 0, 3400.0f, 0.5f, 0.18f, 0.2f, 0.25f, 1.0f, 0.0f);
    setUserSlot(sequencerState, 3, 0, 5600.0f, 0.8f, 0.48f, 0.55f, 0.5f, 0.9f, -0.2f);
    setUserSlot(sequencerState, 4, 0, 780.0f, 2.6f, 0.2f, 0.22f, 0.36f, 1.0f, 0.0f);
    setUserSlot(sequencerState, 5, 0, 6400.0f, 0.65f, 0.4f, 0.35f, 0.56f, 0.92f, 0.25f);

    appendSequencerState(state, sequencerState);
    return state;
}

juce::ValueTree PresetManager::createDelayPulseFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;

    for (int step = 0; step < 16; step += 4)
        setStep(sequencerState, 3, step, 5);

    for (int step = 2; step < 16; step += 4)
        setStep(sequencerState, 5, step, 11);

    setStep(sequencerState, 2, 0, 7);
    setStep(sequencerState, 2, 8, 10);
    setStep(sequencerState, 4, 4, 6);
    setStep(sequencerState, 4, 12, 7);

    setUserSlot(sequencerState, 3, 0, 4800.0f, 0.8f, 0.25f, 0.45f, 0.46f, 0.94f, -0.12f);
    setUserSlot(sequencerState, 5, 0, 5200.0f, 0.7f, 0.38f, 0.36f, 0.38f, 0.88f, 0.18f);
    setUserSlot(sequencerState, 4, 0, 1800.0f, 0.9f, 0.20f, 0.25f, 0.32f, 1.0f, 0.0f);

    appendSequencerState(state, sequencerState);
    return state;
}

juce::ValueTree PresetManager::createFilterCutsFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;

    setStep(sequencerState, 4, 0, 5);
    setStep(sequencerState, 4, 2, 6);
    setStep(sequencerState, 4, 4, 9);
    setStep(sequencerState, 4, 6, 10);
    setStep(sequencerState, 4, 8, 7);
    setStep(sequencerState, 4, 10, 8);
    setStep(sequencerState, 4, 12, 11);
    setStep(sequencerState, 4, 14, 18);
    setStep(sequencerState, 2, 1, 12);
    setStep(sequencerState, 2, 5, 11);
    setStep(sequencerState, 2, 9, 12);
    setStep(sequencerState, 2, 13, 11);

    setUserSlot(sequencerState, 4, 0, 1400.0f, 2.4f, 0.18f, 0.2f, 0.44f, 1.0f, 0.0f);
    setUserSlot(sequencerState, 2, 0, 2400.0f, 0.707f, 0.12f, 0.22f, 0.40f, 0.95f, 0.0f);

    appendSequencerState(state, sequencerState);
    return state;
}

juce::ValueTree PresetManager::createCrushGridFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;

    for (int step = 0; step < 16; step += 4)
        setStep(sequencerState, 5, step, 5);

    for (int step = 2; step < 16; step += 4)
        setStep(sequencerState, 5, step, 6);

    setStep(sequencerState, 3, 6, 17);
    setStep(sequencerState, 3, 7, 18);
    setStep(sequencerState, 3, 14, 17);
    setStep(sequencerState, 3, 15, 18);
    setStep(sequencerState, 4, 8, 10);
    setStep(sequencerState, 4, 12, 11);

    setUserSlot(sequencerState, 5, 0, 3600.0f, 5.0f, 0.32f, 0.28f, 0.65f, 0.86f, 0.0f);
    setUserSlot(sequencerState, 3, 0, 2600.0f, 2.0f, 0.12f, 0.2f, 0.55f, 0.80f, 0.0f);
    setUserSlot(sequencerState, 4, 0, 950.0f, 3.0f, 0.18f, 0.25f, 0.42f, 1.0f, 0.0f);

    appendSequencerState(state, sequencerState);
    return state;
}

juce::ValueTree PresetManager::createLoopChopFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;

    setStep(sequencerState, 1, 0, 5);
    setStep(sequencerState, 1, 4, 6);
    setStep(sequencerState, 1, 8, 9);
    setStep(sequencerState, 1, 12, 10);
    setStep(sequencerState, 0, 2, 8);
    setStep(sequencerState, 0, 6, 11);
    setStep(sequencerState, 0, 10, 12);
    setStep(sequencerState, 0, 14, 9);
    setStep(sequencerState, 2, 0, 9);
    setStep(sequencerState, 2, 8, 12);

    setUserSlot(sequencerState, 0, 0, 2800.0f, 0.707f, 0.16f, 0.18f, 0.40f, 1.0f, 0.0f);
    setUserSlot(sequencerState, 1, 0, 2200.0f, 0.9f, 0.10f, 0.30f, 0.72f, 0.92f, 0.0f);
    setUserSlot(sequencerState, 2, 0, 1800.0f, 1.0f, 0.12f, 0.20f, 0.38f, 0.90f, 0.0f);

    appendSequencerState(state, sequencerState);
    return state;
}

juce::ValueTree PresetManager::createNotchMotionFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;

    for (int step = 0; step < 16; step += 2)
        setStep(sequencerState, 4, step, 10);

    setStep(sequencerState, 3, 3, 15);
    setStep(sequencerState, 3, 7, 16);
    setStep(sequencerState, 3, 11, 15);
    setStep(sequencerState, 3, 15, 16);
    setStep(sequencerState, 5, 4, 13);
    setStep(sequencerState, 5, 12, 15);

    setUserSlot(sequencerState, 4, 0, 1200.0f, 4.0f, 0.20f, 0.25f, 0.52f, 0.98f, 0.0f);
    setUserSlot(sequencerState, 3, 0, 3400.0f, 1.3f, 0.14f, 0.22f, 0.48f, 0.90f, -0.15f);
    setUserSlot(sequencerState, 5, 0, 4200.0f, 2.5f, 0.18f, 0.30f, 0.46f, 0.88f, 0.16f);

    appendSequencerState(state, sequencerState);
    return state;
}

} // namespace zikada
