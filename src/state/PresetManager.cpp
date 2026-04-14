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

} // namespace

PresetManager::PresetManager()
    : presetDirectory(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                          .getChildFile("ZIKARATOR")
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
}

void PresetManager::addUserPresets()
{
    const auto files = presetDirectory.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (const auto& file : files)
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr)
            continue;

        auto state = juce::ValueTree::fromXml(*xml);
        if (!state.isValid())
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
    state.setProperty(ParameterIDs::dryWet, 100.0f, nullptr);
    state.setProperty(ParameterIDs::outputGain, 0.0f, nullptr);
    state.setProperty(ParameterIDs::mixMode, 0, nullptr);
    state.setProperty(ParameterIDs::clockSource, 0, nullptr);
    state.setProperty(ParameterIDs::tempo, 120.0f, nullptr);
    state.setProperty(ParameterIDs::stepResolution, 0, nullptr);
    state.setProperty(ParameterIDs::bypass, false, nullptr);

    for (int lane = 0; lane < 6; ++lane)
    {
        state.setProperty(getLaneMixID(lane), 100.0f, nullptr);
        for (int step = 0; step < 16; ++step)
            state.setProperty(getStepActiveID(lane, step), false, nullptr);
    }

    return state;
}

juce::ValueTree PresetManager::createInitFactoryState()
{
    auto state = createBaseState();
    SequencerState sequencerState;
    state.addChild(sequencerState.toValueTree(), -1, nullptr);
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

    state.addChild(sequencerState.toValueTree(), -1, nullptr);
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

    state.addChild(sequencerState.toValueTree(), -1, nullptr);
    return state;
}

} // namespace zikada
