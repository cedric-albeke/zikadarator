#include "state/PresetManager.h"

namespace zikada {

namespace {

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

} // namespace

PresetManager::PresetManager()
    : presetDirectory(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                          .getChildFile("ZIKADARATOR")
                          .getChildFile("Presets")),
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

bool PresetManager::saveUserPreset(const juce::String& name, const juce::ValueTree& state)
{
    const auto trimmedName = name.trim();
    if (trimmedName.isEmpty() || !state.isValid())
        return false;

    presetDirectory.createDirectory();
    const auto file = presetDirectory.getChildFile(sanitizeName(trimmedName) + ".xml");
    auto xml = state.createXml();
    if (xml == nullptr)
        return false;

    if (!xml->writeTo(file))
        return false;

    markPresetUsed(trimmedName.toUpperCase());
    refresh();
    return true;
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
        favoritePresetNames.removeString(item.name);
        recentPresetNames.removeString(item.name);
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

void PresetManager::addFactoryPresets()
{
    items.push_back({"INIT", "Utility", "Factory · clean starting point", true, isFavorite("INIT"), recentPresetNames.indexOf("INIT"), {}, createInitFactoryState()});
    items.push_back({"NEON GATE", "Glitch", "Factory · gated stutter rhythm", true, isFavorite("NEON GATE"), recentPresetNames.indexOf("NEON GATE"), {}, createNeonGateFactoryState()});
    items.push_back({"SPACE BLOOM", "Ambient", "Factory · airy delay and filter trail", true, isFavorite("SPACE BLOOM"), recentPresetNames.indexOf("SPACE BLOOM"), {}, createSpaceBloomFactoryState()});
    items.push_back({"DELAY PULSE", "Delay", "Factory - synced delay pattern", true, isFavorite("DELAY PULSE"), recentPresetNames.indexOf("DELAY PULSE"), {}, createDelayPulseFactoryState()});
    items.push_back({"FILTER CUTS", "Filter", "Factory - stepped filter movement", true, isFavorite("FILTER CUTS"), recentPresetNames.indexOf("FILTER CUTS"), {}, createFilterCutsFactoryState()});
    items.push_back({"CRUSH GRID", "Texture", "Factory - bitcrush and drive rhythm", true, isFavorite("CRUSH GRID"), recentPresetNames.indexOf("CRUSH GRID"), {}, createCrushGridFactoryState()});
    items.push_back({"LOOP CHOP", "Loop", "Factory - micro-loop cuts", true, isFavorite("LOOP CHOP"), recentPresetNames.indexOf("LOOP CHOP"), {}, createLoopChopFactoryState()});
    items.push_back({"NOTCH MOTION", "Filter", "Factory - notch and tremolo motion", true, isFavorite("NOTCH MOTION"), recentPresetNames.indexOf("NOTCH MOTION"), {}, createNotchMotionFactoryState()});
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
        if (!state.isValid() || state.hasType("PresetMetadata"))
            continue;

        const auto presetName = file.getFileNameWithoutExtension().replaceCharacter('_', ' ').toUpperCase();
        items.push_back({presetName,
                         "User",
                         "User · " + file.getFullPathName(),
                         false,
                         isFavorite(presetName),
                         recentPresetNames.indexOf(presetName),
                         file,
                         state});
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
        const auto recentA = a.recentRank >= 0 ? a.recentRank : 9999;
        const auto recentB = b.recentRank >= 0 ? b.recentRank : 9999;

        if (a.isFavorite != b.isFavorite)
            return a.isFavorite > b.isFavorite;

        if (recentA != recentB)
            return recentA < recentB;

        if (a.isFactory != b.isFactory)
            return a.isFactory > b.isFactory;

        if (a.category != b.category)
            return a.category < b.category;

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
