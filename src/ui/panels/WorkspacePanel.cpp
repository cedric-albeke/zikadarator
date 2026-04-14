#include "ui/panels/WorkspacePanel.h"

#include "state/ParameterIDs.h"

namespace zikada {

namespace {

void configureLabel(juce::Label& label, float size, juce::Colour colour, juce::Justification justification, bool bold = false)
{
    label.setJustificationType(justification);
    label.setColour(juce::Label::textColourId, colour);
    label.setFont(juce::Font(juce::FontOptions().withHeight(size + 2.0f).withStyle(bold ? "Bold" : "Regular")));
}

void configureFilterButton(juce::TextButton& button)
{
    button.setClickingTogglesState(true);
    button.setRadioGroupId(2201);
    button.setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
    button.setColour(juce::TextButton::buttonOnColourId, Colours::neonGreen);
    button.setColour(juce::TextButton::textColourOffId, Colours::white85);
    button.setColour(juce::TextButton::textColourOnId, Colours::bgPrimary);
}

void configureActionButton(juce::TextButton& button, juce::Colour colour)
{
    button.setColour(juce::TextButton::buttonColourId, colour);
    button.setColour(juce::TextButton::buttonOnColourId, colour.brighter(0.08f));
    button.setColour(juce::TextButton::textColourOffId, Colours::bgPrimary);
    button.setColour(juce::TextButton::textColourOnId, Colours::bgPrimary);
}

void configureSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 20);
}

void configureCombo(juce::ComboBox& combo)
{
    combo.setColour(juce::ComboBox::backgroundColourId, Colours::bgSurface);
    combo.setColour(juce::ComboBox::textColourId, Colours::white85);
    combo.setColour(juce::ComboBox::outlineColourId, Colours::white10);
    combo.setColour(juce::ComboBox::arrowColourId, Colours::neonGreen);
}

void configureValueLabel(juce::Label& label)
{
    configureLabel(label, 11.0f, Colours::white50, juce::Justification::centredLeft, true);
}

void styleComponentTree(juce::Component& component)
{
    if (auto* combo = dynamic_cast<juce::ComboBox*>(&component))
    {
        combo->setColour(juce::ComboBox::backgroundColourId, Colours::bgSurface);
        combo->setColour(juce::ComboBox::textColourId, Colours::white85);
        combo->setColour(juce::ComboBox::outlineColourId, Colours::white10);
        combo->setColour(juce::ComboBox::arrowColourId, Colours::neonGreen);
    }
    else if (auto* label = dynamic_cast<juce::Label*>(&component))
    {
        label->setColour(juce::Label::textColourId, Colours::white85);
        label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    }
    else if (auto* toggle = dynamic_cast<juce::ToggleButton*>(&component))
    {
        toggle->setColour(juce::ToggleButton::textColourId, Colours::white85);
        toggle->setColour(juce::ToggleButton::tickColourId, Colours::neonGreen);
        toggle->setColour(juce::ToggleButton::tickDisabledColourId, Colours::white50);
    }
    else if (auto* textButton = dynamic_cast<juce::TextButton*>(&component))
    {
        textButton->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        textButton->setColour(juce::TextButton::buttonOnColourId, Colours::bgHover.brighter(0.06f));
        textButton->setColour(juce::TextButton::textColourOffId, Colours::white85);
        textButton->setColour(juce::TextButton::textColourOnId, Colours::white);
    }
    else if (auto* listBox = dynamic_cast<juce::ListBox*>(&component))
    {
        listBox->setColour(juce::ListBox::backgroundColourId, Colours::bgSurface);
        listBox->setColour(juce::ListBox::outlineColourId, Colours::white10);
    }
    else if (auto* slider = dynamic_cast<juce::Slider*>(&component))
    {
        slider->setColour(juce::Slider::trackColourId, Colours::white10);
        slider->setColour(juce::Slider::thumbColourId, Colours::neonGreen);
        slider->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
        slider->setColour(juce::Slider::textBoxTextColourId, Colours::white85);
        slider->setColour(juce::Slider::textBoxBackgroundColourId, Colours::bgSurface);
        slider->setColour(juce::Slider::textBoxOutlineColourId, Colours::white10);
    }

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        styleComponentTree(*component.getChildComponent(i));
}

} // namespace

WorkspacePanel::WorkspacePanel()
{
    configureLabel(titleLabel, 28.0f, Colours::white, juce::Justification::centredLeft);
    configureLabel(bodyLabel, 14.0f, Colours::white85, juce::Justification::topLeft);
    bodyLabel.setMinimumHorizontalScale(1.0f);

    configureLabel(presetHintLabel, 11.0f, Colours::white50, juce::Justification::centredLeft);
    configureLabel(presetDetailTitle, 24.0f, Colours::white, juce::Justification::centredLeft, true);
    configureLabel(presetDetailMeta, 12.0f, Colours::neonGreen, juce::Justification::centredLeft, true);
    configureLabel(presetDetailBody, 13.0f, Colours::white85, juce::Justification::topLeft);
    configureLabel(presetSaveLabel, 11.0f, Colours::white50, juce::Justification::centredLeft);
    configureLabel(presetInfoTitleA, 12.0f, Colours::neonGreen, juce::Justification::topLeft, true);
    configureLabel(presetInfoBodyA, 12.0f, Colours::white85, juce::Justification::topLeft);
    configureLabel(presetInfoTitleB, 12.0f, Colours::neonGreen, juce::Justification::topLeft, true);
    configureLabel(presetInfoBodyB, 12.0f, Colours::white85, juce::Justification::topLeft);

    configureLabel(settingsLeadLabel, 13.0f, Colours::white85, juce::Justification::topLeft);
    configureLabel(settingsDeviceTitle, 13.0f, Colours::neonGreen, juce::Justification::topLeft, true);
    configureLabel(settingsProductTitle, 13.0f, Colours::neonGreen, juce::Justification::topLeft, true);
    configureLabel(settingsNotesTitle, 13.0f, Colours::neonGreen, juce::Justification::topLeft, true);
    configureLabel(settingsNotesBody, 12.0f, Colours::white85, juce::Justification::topLeft);
    configureLabel(standaloneMuteLabel, 11.0f, Colours::white50, juce::Justification::centredLeft, true);
    configureValueLabel(dryWetLabel);
    configureValueLabel(outputGainLabel);
    configureValueLabel(tempoLabel);
    configureValueLabel(mixModeLabel);
    configureValueLabel(clockSourceLabel);
    configureValueLabel(stepResolutionLabel);

    presetList.setModel(this);
    presetList.setRowHeight(52);
    presetList.setColour(juce::ListBox::backgroundColourId, Colours::displayBezel);
    presetList.setColour(juce::ListBox::outlineColourId, Colours::white10);
    presetList.setOutlineThickness(1);

    presetSearchEditor.setTextToShowWhenEmpty("Search presets", Colours::white50);
    presetSearchEditor.setColour(juce::TextEditor::backgroundColourId, Colours::bgSurface);
    presetSearchEditor.setColour(juce::TextEditor::textColourId, Colours::white85);
    presetSearchEditor.setColour(juce::TextEditor::outlineColourId, Colours::white10);
    presetSearchEditor.onTextChange = [this] { rebuildPresetFilter(); };

    presetCategoryBox.addItem("ALL CATEGORIES", 1);
    presetCategoryBox.setSelectedId(1, juce::dontSendNotification);
    configureCombo(presetCategoryBox);
    presetCategoryBox.onChange = [this] { rebuildPresetFilter(); };

    presetNameEditor.setTextToShowWhenEmpty("New user preset name", Colours::white50);
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, Colours::bgSurface);
    presetNameEditor.setColour(juce::TextEditor::textColourId, Colours::white85);
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, Colours::white10);

    configureFilterButton(allFilterButton);
    configureFilterButton(factoryFilterButton);
    configureFilterButton(userFilterButton);
    configureFilterButton(favoriteFilterButton);
    allFilterButton.onClick = [this] { setPresetSourceFilter(PresetSourceFilter::All); };
    factoryFilterButton.onClick = [this] { setPresetSourceFilter(PresetSourceFilter::Factory); };
    userFilterButton.onClick = [this] { setPresetSourceFilter(PresetSourceFilter::User); };
    favoriteFilterButton.onClick = [this] { setPresetSourceFilter(PresetSourceFilter::Favorites); };
    allFilterButton.setToggleState(true, juce::dontSendNotification);

    configureActionButton(saveButton, Colours::neonGreen);
    configureActionButton(loadButton, Colours::laneFX2);
    configureActionButton(deleteButton, Colours::warning);
    configureActionButton(favoritePresetButton, Colours::bgAccent.brighter(0.08f));

    saveButton.onClick = [this]
    {
        if (onSavePreset)
            onSavePreset(presetNameEditor.getText());
    };

    loadButton.onClick = [this]
    {
        if (selectedPresetRow >= 0 && onLoadPreset)
            onLoadPreset(selectedPresetRow);
    };

    deleteButton.onClick = [this]
    {
        if (selectedPresetRow >= 0 && onDeletePreset)
            onDeletePreset(selectedPresetRow);
    };

    favoritePresetButton.onClick = [this]
    {
        if (selectedPresetRow < 0)
            return;

        if (onToggleFavoritePreset)
            onToggleFavoritePreset(selectedPresetRow);
    };

    settingsLeadLabel.setText("Standalone audio and MIDI device configuration now lives inside this Settings tab, so the old external host dialog is no longer the only place to change devices, buffer size, sample rate, and MIDI inputs.", juce::dontSendNotification);
    settingsDeviceTitle.setText("AUDIO / MIDI DEVICE SETUP", juce::dontSendNotification);
    settingsProductTitle.setText("PRODUCT SETTINGS", juce::dontSendNotification);
    settingsNotesTitle.setText("SETTINGS NOTES", juce::dontSendNotification);
    settingsNotesBody.setText("The upper panel is the real JUCE standalone device selector. The lower product settings keep global editor behavior, clocking, and output controls close to the host-device setup instead of scattering them through the editor.", juce::dontSendNotification);
    standaloneMuteLabel.setText("FEEDBACK LOOP", juce::dontSendNotification);
    standaloneMuteButton.setColour(juce::ToggleButton::textColourId, Colours::white85);
    standaloneMuteButton.setClickingTogglesState(true);
    dryWetLabel.setText("GLOBAL DRY/WET", juce::dontSendNotification);
    outputGainLabel.setText("OUTPUT GAIN", juce::dontSendNotification);
    tempoLabel.setText("FREE TEMPO", juce::dontSendNotification);
    mixModeLabel.setText("MIX MODE", juce::dontSendNotification);
    clockSourceLabel.setText("CLOCK SOURCE", juce::dontSendNotification);
    stepResolutionLabel.setText("STEP RESOLUTION", juce::dontSendNotification);

    configureSlider(dryWetSlider);
    configureSlider(outputGainSlider);
    configureSlider(tempoSlider);
    dryWetSlider.setRange(0.0, 100.0, 0.1);
    outputGainSlider.setRange(-24.0, 24.0, 0.1);
    tempoSlider.setRange(20.0, 300.0, 0.1);

    configureCombo(mixModeBox);
    mixModeBox.addItemList({"Linear", "Ducking", "Sidechain", "Multiply", "Screen", "Difference"}, 1);
    configureCombo(clockSourceBox);
    clockSourceBox.addItemList({"Host", "Free"}, 1);
    configureCombo(stepResolutionBox);
    stepResolutionBox.addItemList({"1/8", "1/4", "1/2"}, 1);

    bypassToggle.setColour(juce::ToggleButton::textColourId, Colours::white85);

    rebuildStandaloneSettingsComponent();

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(bodyLabel);
    addAndMakeVisible(presetHintLabel);
    addAndMakeVisible(presetDetailTitle);
    addAndMakeVisible(presetDetailMeta);
    addAndMakeVisible(presetDetailBody);
    addAndMakeVisible(presetSaveLabel);
    addAndMakeVisible(presetInfoTitleA);
    addAndMakeVisible(presetInfoBodyA);
    addAndMakeVisible(presetInfoTitleB);
    addAndMakeVisible(presetInfoBodyB);
    addAndMakeVisible(presetSearchEditor);
    addAndMakeVisible(presetCategoryBox);
    addAndMakeVisible(presetList);
    addAndMakeVisible(presetNameEditor);
    addAndMakeVisible(allFilterButton);
    addAndMakeVisible(factoryFilterButton);
    addAndMakeVisible(userFilterButton);
    addAndMakeVisible(favoriteFilterButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(loadButton);
    addAndMakeVisible(deleteButton);
    addAndMakeVisible(favoritePresetButton);

    addAndMakeVisible(settingsLeadLabel);
    addAndMakeVisible(settingsDeviceTitle);
    addAndMakeVisible(settingsProductTitle);
    addAndMakeVisible(settingsNotesTitle);
    addAndMakeVisible(settingsNotesBody);
    addAndMakeVisible(standaloneMuteLabel);
    addAndMakeVisible(standaloneMuteButton);
    addAndMakeVisible(dryWetLabel);
    addAndMakeVisible(outputGainLabel);
    addAndMakeVisible(tempoLabel);
    addAndMakeVisible(mixModeLabel);
    addAndMakeVisible(clockSourceLabel);
    addAndMakeVisible(stepResolutionLabel);
    addAndMakeVisible(bypassToggle);
    addAndMakeVisible(dryWetSlider);
    addAndMakeVisible(outputGainSlider);
    addAndMakeVisible(tempoSlider);
    addAndMakeVisible(mixModeBox);
    addAndMakeVisible(clockSourceBox);
    addAndMakeVisible(stepResolutionBox);

    refreshCopy();
}

void WorkspacePanel::bindToParameters(juce::AudioProcessorValueTreeState& apvts)
{
    dryWetAttachment = std::make_unique<SliderAttachment>(apvts, ParameterIDs::dryWet, dryWetSlider);
    outputGainAttachment = std::make_unique<SliderAttachment>(apvts, ParameterIDs::outputGain, outputGainSlider);
    tempoAttachment = std::make_unique<SliderAttachment>(apvts, ParameterIDs::tempo, tempoSlider);
    mixModeAttachment = std::make_unique<ComboBoxAttachment>(apvts, ParameterIDs::mixMode, mixModeBox);
    clockSourceAttachment = std::make_unique<ComboBoxAttachment>(apvts, ParameterIDs::clockSource, clockSourceBox);
    stepResolutionAttachment = std::make_unique<ComboBoxAttachment>(apvts, ParameterIDs::stepResolution, stepResolutionBox);
    bypassAttachment = std::make_unique<ButtonAttachment>(apvts, ParameterIDs::bypass, bypassToggle);
    rebuildStandaloneSettingsComponent();
}

void WorkspacePanel::setMode(Mode newMode)
{
    if (mode == newMode)
        return;

    mode = newMode;
    refreshCopy();
    repaint();
}

void WorkspacePanel::setPresetItems(std::vector<PresetManager::PresetItem> newItems)
{
    presetItems = std::move(newItems);

    const auto previousCategory = presetCategoryBox.getText();
    presetCategoryBox.clear(juce::dontSendNotification);
    presetCategoryBox.addItem("ALL CATEGORIES", 1);

    juce::StringArray categories;
    for (const auto& item : presetItems)
        if (item.category.isNotEmpty() && !categories.contains(item.category))
            categories.add(item.category);

    categories.sort(true);
    for (int i = 0; i < categories.size(); ++i)
        presetCategoryBox.addItem(categories[i], i + 2);

    int restoredId = 0;
    for (int i = 0; i < presetCategoryBox.getNumItems(); ++i)
    {
        const int itemId = presetCategoryBox.getItemId(i);
        if (presetCategoryBox.getItemText(i) == previousCategory)
        {
            restoredId = itemId;
            break;
        }
    }
    presetCategoryBox.setSelectedId(restoredId > 0 ? restoredId : 1, juce::dontSendNotification);

    rebuildPresetFilter();
}

void WorkspacePanel::refreshCopy()
{
    if (mode == Mode::Presets)
    {
        titleLabel.setText("PRESET BROWSER", juce::dontSendNotification);
        bodyLabel.setText("Search, filter, load, and save full sequencer snapshots from a denser browser-first layout.", juce::dontSendNotification);
        presetHintLabel.setText("Filter the library, then load the selected snapshot or save the current state as a user preset.", juce::dontSendNotification);
        presetSaveLabel.setText("SAVE CURRENT STATE", juce::dontSendNotification);
        presetInfoTitleA.setText("LIBRARY SNAPSHOT", juce::dontSendNotification);
        presetInfoTitleB.setText("USER STORAGE", juce::dontSendNotification);
    }
    else
    {
        titleLabel.setText("SETTINGS", juce::dontSendNotification);
        bodyLabel.setText("Standalone audio device, sample rate, buffer size, and MIDI routing now live directly inside the editor.", juce::dontSendNotification);
    }

    applyVisibility();
    updatePresetInfoPanels();
    updatePresetDetail();
}

void WorkspacePanel::applyVisibility()
{
    const bool showPresets = mode == Mode::Presets;

    presetHintLabel.setVisible(showPresets);
    presetDetailTitle.setVisible(showPresets);
    presetDetailMeta.setVisible(showPresets);
    presetDetailBody.setVisible(showPresets);
    presetSaveLabel.setVisible(showPresets);
    presetInfoTitleA.setVisible(showPresets);
    presetInfoBodyA.setVisible(showPresets);
    presetInfoTitleB.setVisible(showPresets);
    presetInfoBodyB.setVisible(showPresets);
    presetSearchEditor.setVisible(showPresets);
    presetCategoryBox.setVisible(showPresets);
    presetList.setVisible(showPresets);
    presetNameEditor.setVisible(showPresets);
    allFilterButton.setVisible(showPresets);
    factoryFilterButton.setVisible(showPresets);
    userFilterButton.setVisible(showPresets);
    favoriteFilterButton.setVisible(showPresets);
    saveButton.setVisible(showPresets);
    loadButton.setVisible(showPresets);
    deleteButton.setVisible(showPresets);
    favoritePresetButton.setVisible(showPresets);

    settingsLeadLabel.setVisible(!showPresets);
    settingsDeviceTitle.setVisible(!showPresets);
    settingsProductTitle.setVisible(!showPresets);
    settingsNotesTitle.setVisible(!showPresets);
    settingsNotesBody.setVisible(!showPresets);
    standaloneMuteLabel.setVisible(!showPresets && standaloneDeviceSelector != nullptr);
    standaloneMuteButton.setVisible(!showPresets && standaloneDeviceSelector != nullptr);
    dryWetLabel.setVisible(!showPresets);
    outputGainLabel.setVisible(!showPresets);
    tempoLabel.setVisible(!showPresets);
    mixModeLabel.setVisible(!showPresets);
    clockSourceLabel.setVisible(!showPresets);
    stepResolutionLabel.setVisible(!showPresets);
    bypassToggle.setVisible(!showPresets);
    dryWetSlider.setVisible(!showPresets);
    outputGainSlider.setVisible(!showPresets);
    tempoSlider.setVisible(!showPresets);
    mixModeBox.setVisible(!showPresets);
    clockSourceBox.setVisible(!showPresets);
    stepResolutionBox.setVisible(!showPresets);
    if (standaloneDeviceSelector != nullptr)
        standaloneDeviceSelector->setVisible(!showPresets);
}

void WorkspacePanel::rebuildStandaloneSettingsComponent()
{
    if (standaloneDeviceSelector != nullptr)
        removeChildComponent(standaloneDeviceSelector.get());

    standaloneDeviceSelector.reset();

   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        int maxNumInputs = 0;
        int maxNumOutputs = 0;

        if (auto* processor = holder->processor.get())
        {
            if (auto* inputBus = processor->getBus(true, 0))
                maxNumInputs = juce::jmax(0, inputBus->getDefaultLayout().size());
            if (auto* outputBus = processor->getBus(false, 0))
                maxNumOutputs = juce::jmax(0, outputBus->getDefaultLayout().size());

            if (maxNumInputs == 0)
                maxNumInputs = processor->getMainBusNumInputChannels();
            if (maxNumOutputs == 0)
                maxNumOutputs = processor->getMainBusNumOutputChannels();
        }

        standaloneDeviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(holder->deviceManager,
                                                                                         0, maxNumInputs,
                                                                                         0, maxNumOutputs,
                                                                                         true,
                                                                                         holder->processor != nullptr && holder->processor->producesMidi(),
                                                                                         true,
                                                                                         false);
        addAndMakeVisible(*standaloneDeviceSelector);
        styleStandaloneSettingsComponent();
        muteInputValue.referTo(holder->getMuteInputValue());
        standaloneMuteButton.getToggleStateValue().referTo(muteInputValue);
        const bool hasFeedbackToggle = holder->getProcessorHasPotentialFeedbackLoop();
        standaloneMuteLabel.setVisible(hasFeedbackToggle);
        standaloneMuteButton.setVisible(hasFeedbackToggle);
    }
   #endif
}

void WorkspacePanel::styleStandaloneSettingsComponent()
{
    if (standaloneDeviceSelector == nullptr)
        return;

    standaloneDeviceSelector->setColour(juce::ListBox::backgroundColourId, Colours::bgSurface);
    standaloneDeviceSelector->setColour(juce::ListBox::outlineColourId, Colours::white10);
    styleComponentTree(*standaloneDeviceSelector);
}

void WorkspacePanel::setPresetSourceFilter(PresetSourceFilter filter)
{
    presetSourceFilter = filter;
    allFilterButton.setToggleState(filter == PresetSourceFilter::All, juce::dontSendNotification);
    factoryFilterButton.setToggleState(filter == PresetSourceFilter::Factory, juce::dontSendNotification);
    userFilterButton.setToggleState(filter == PresetSourceFilter::User, juce::dontSendNotification);
    favoriteFilterButton.setToggleState(filter == PresetSourceFilter::Favorites, juce::dontSendNotification);
    rebuildPresetFilter();
}

void WorkspacePanel::updatePresetInfoPanels()
{
    const int total = static_cast<int>(presetItems.size());
    int factoryCount = 0;
    int userCount = 0;

    for (const auto& item : presetItems)
        item.isFactory ? ++factoryCount : ++userCount;

    presetInfoBodyA.setText("Total presets: " + juce::String(total)
                                + "\nFactory: " + juce::String(factoryCount)
                                + "\nUser: " + juce::String(userCount)
                                + "\nVisible with current filters: " + juce::String(static_cast<int>(filteredPresetIndices.size())),
                            juce::dontSendNotification);

    const auto storagePath = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                 .getChildFile("ZIKADARATOR")
                                 .getChildFile("Presets")
                                 .getFullPathName();
    presetInfoBodyB.setText("User presets are written to:\n" + storagePath
                                + "\n\nFactory presets remain embedded in the binary for fast browsing and instant restore.",
                            juce::dontSendNotification);

    if (selectedPresetRow >= 0 && selectedPresetRow < static_cast<int>(presetItems.size()))
        favoritePresetButton.setButtonText(presetItems[static_cast<size_t>(selectedPresetRow)].isFavorite ? "UNSTAR" : "STAR");
    else
        favoritePresetButton.setButtonText("STAR");
}

void WorkspacePanel::rebuildPresetFilter()
{
    filteredPresetIndices.clear();

    const auto query = presetSearchEditor.getText().trim().toLowerCase();
    const auto category = presetCategoryBox.getSelectedId() > 1 ? presetCategoryBox.getText() : juce::String();

    for (int i = 0; i < static_cast<int>(presetItems.size()); ++i)
    {
        const auto& item = presetItems[static_cast<size_t>(i)];

        if (presetSourceFilter == PresetSourceFilter::Factory && !item.isFactory)
            continue;
        if (presetSourceFilter == PresetSourceFilter::User && item.isFactory)
            continue;
        if (presetSourceFilter == PresetSourceFilter::Favorites && !item.isFavorite)
            continue;
        if (category.isNotEmpty() && item.category != category)
            continue;

        const auto haystack = (item.name + " " + item.category + " " + item.subtitle).toLowerCase();
        if (query.isNotEmpty() && !haystack.contains(query))
            continue;

        filteredPresetIndices.push_back(i);
    }

    presetList.updateContent();

    if (filteredPresetIndices.empty())
    {
        presetList.selectRow(-1);
        selectedPresetRow = -1;
    }
    else
    {
        int targetFilteredRow = 0;
        for (int row = 0; row < static_cast<int>(filteredPresetIndices.size()); ++row)
        {
            if (filteredPresetIndices[static_cast<size_t>(row)] == selectedPresetRow)
            {
                targetFilteredRow = row;
                break;
            }
        }

        presetList.selectRow(targetFilteredRow, false, true);
        selectedPresetRow = filteredPresetIndices[static_cast<size_t>(targetFilteredRow)];
    }

    updatePresetDetail();
    updatePresetInfoPanels();
}

void WorkspacePanel::updatePresetDetail()
{
    if (selectedPresetRow < 0 || selectedPresetRow >= static_cast<int>(presetItems.size()))
    {
        presetDetailTitle.setText("NO PRESET SELECTED", juce::dontSendNotification);
        presetDetailMeta.setText("Choose a preset from the browser", juce::dontSendNotification);
        presetDetailBody.setText("The detail panel shows the selected preset's source, category, and storage location. It also hosts the main load and delete actions so the browser stays readable.", juce::dontSendNotification);
        loadButton.setEnabled(false);
        deleteButton.setEnabled(false);
        loadButton.setAlpha(0.45f);
        deleteButton.setAlpha(0.45f);
        return;
    }

    const auto& item = presetItems[static_cast<size_t>(selectedPresetRow)];
    presetDetailTitle.setText(item.name, juce::dontSendNotification);
    presetDetailMeta.setText((item.isFactory ? "FACTORY" : "USER") + juce::String(" / ") + item.category, juce::dontSendNotification);

    juce::String detail = item.subtitle;
    if (!item.isFactory && item.file.existsAsFile())
        detail << "\n\nStored at:\n" << item.file.getFullPathName();
    else if (item.isFactory)
        detail << "\n\nEmbedded factory preset ready to load instantly.";

    presetDetailBody.setText(detail, juce::dontSendNotification);
    loadButton.setEnabled(true);
    loadButton.setAlpha(1.0f);
    const bool canDelete = !item.isFactory;
    deleteButton.setEnabled(canDelete);
    deleteButton.setAlpha(canDelete ? 1.0f : 0.45f);
}

void WorkspacePanel::paint(juce::Graphics& g)
{
    ZikadaLookAndFeel::drawPremiumPanel(g, getLocalBounds(), true);

    auto hero = heroZone.toFloat();
    juce::ColourGradient glow(Colours::bgAccent.brighter(0.20f), hero.getX(), hero.getY(),
                              Colours::bgPrimary, hero.getRight(), hero.getBottom(), true);
    g.setGradientFill(glow);
    g.fillRoundedRectangle(hero, PanelMetrics::kCorner);
    g.setColour(Colours::neonGreen.withAlpha(0.28f));
    g.drawRoundedRectangle(hero.reduced(0.5f), PanelMetrics::kCorner, 1.0f);

    if (mode == Mode::Presets)
    {
        ZikadaLookAndFeel::drawDeviceDisplay(g, presetBrowserZone);
        ZikadaLookAndFeel::drawDeviceDisplay(g, presetDetailZone);
        ZikadaLookAndFeel::drawDeviceDisplay(g, presetInfoZoneA);
        ZikadaLookAndFeel::drawDeviceDisplay(g, presetInfoZoneB);
    }
    else
    {
        ZikadaLookAndFeel::drawDeviceDisplay(g, settingsDeviceZone);
        ZikadaLookAndFeel::drawDeviceDisplay(g, settingsProductZone);
        ZikadaLookAndFeel::drawDeviceDisplay(g, settingsNotesZone);
    }
}

void WorkspacePanel::resized()
{
    auto bounds = getLocalBounds().reduced(28);
    heroZone = bounds.removeFromTop(92);
    bounds.removeFromTop(12);

    titleLabel.setBounds(heroZone.reduced(18, 14).removeFromTop(34));
    auto heroText = heroZone.reduced(18, 16);
    heroText.removeFromTop(38);
    bodyLabel.setBounds(heroText.removeFromTop(30));

    if (mode == Mode::Presets)
    {
        const int browserWidth = juce::jlimit(380, 460, bounds.getWidth() / 3);
        presetBrowserZone = bounds.removeFromLeft(browserWidth);
        bounds.removeFromLeft(14);
        auto right = bounds;
        const int detailHeight = juce::jlimit(170, 250, right.getHeight() / 2);
        presetDetailZone = right.removeFromTop(detailHeight);
        right.removeFromTop(12);
        presetInfoZoneA = right.removeFromLeft((right.getWidth() - 12) / 2);
        right.removeFromLeft(12);
        presetInfoZoneB = right;

        auto browserInner = presetBrowserZone.reduced(14);
        presetHintLabel.setBounds(browserInner.removeFromTop(18));
        browserInner.removeFromTop(10);

        auto filterRow = browserInner.removeFromTop(30);
        presetSearchEditor.setBounds(filterRow.removeFromLeft(162));
        filterRow.removeFromLeft(8);
        presetCategoryBox.setBounds(filterRow.removeFromLeft(132));
        filterRow.removeFromLeft(8);
        allFilterButton.setBounds(filterRow.removeFromLeft(46));
        filterRow.removeFromLeft(6);
        factoryFilterButton.setBounds(filterRow.removeFromLeft(72));
        filterRow.removeFromLeft(6);
        userFilterButton.setBounds(filterRow.removeFromLeft(58));
        filterRow.removeFromLeft(6);
        favoriteFilterButton.setBounds(filterRow.removeFromLeft(48));

        browserInner.removeFromTop(10);
        auto saveArea = browserInner.removeFromBottom(74);
        presetList.setBounds(browserInner);

        presetSaveLabel.setBounds(saveArea.removeFromTop(16));
        saveArea.removeFromTop(8);
        presetNameEditor.setBounds(saveArea.removeFromLeft(juce::jmax(180, saveArea.getWidth() - 130)));
        saveArea.removeFromLeft(10);
        saveButton.setBounds(saveArea.removeFromLeft(120));

        auto detailInner = presetDetailZone.reduced(18, 16);
        presetDetailTitle.setBounds(detailInner.removeFromTop(36));
        presetDetailMeta.setBounds(detailInner.removeFromTop(18));
        detailInner.removeFromTop(10);
        auto detailActionRow = detailInner.removeFromBottom(36);
        presetDetailBody.setBounds(detailInner);
        loadButton.setBounds(detailActionRow.removeFromLeft(128));
        detailActionRow.removeFromLeft(10);
        favoritePresetButton.setBounds(detailActionRow.removeFromLeft(84));
        detailActionRow.removeFromLeft(10);
        deleteButton.setBounds(detailActionRow.removeFromLeft(118));

        auto infoAInner = presetInfoZoneA.reduced(18, 16);
        presetInfoTitleA.setBounds(infoAInner.removeFromTop(20));
        infoAInner.removeFromTop(8);
        presetInfoBodyA.setBounds(infoAInner);

        auto infoBInner = presetInfoZoneB.reduced(18, 16);
        presetInfoTitleB.setBounds(infoBInner.removeFromTop(20));
        infoBInner.removeFromTop(8);
        presetInfoBodyB.setBounds(infoBInner);
    }
    else
    {
        const int topHeight = juce::jlimit(320, 520, bounds.getHeight() * 2 / 3);
        settingsDeviceZone = bounds.removeFromTop(topHeight);
        bounds.removeFromTop(14);
        settingsProductZone = bounds.removeFromTop(190);
        bounds.removeFromTop(14);
        settingsNotesZone = bounds;

        auto deviceInner = settingsDeviceZone.reduced(18, 16);
        settingsDeviceTitle.setBounds(deviceInner.removeFromTop(20));
        deviceInner.removeFromTop(10);
        settingsLeadLabel.setBounds(deviceInner.removeFromTop(38));
        deviceInner.removeFromTop(10);

        if (standaloneDeviceSelector != nullptr)
        {
            auto muteRow = deviceInner.removeFromTop(28);
            standaloneMuteLabel.setBounds(muteRow.removeFromLeft(130));
            standaloneMuteButton.setBounds(muteRow.removeFromLeft(220));
            deviceInner.removeFromTop(10);
            standaloneDeviceSelector->setBounds(deviceInner);
        }

        auto productInner = settingsProductZone.reduced(18, 16);
        settingsProductTitle.setBounds(productInner.removeFromTop(20));
        productInner.removeFromTop(10);

        auto row1 = productInner.removeFromTop(28);
        clockSourceLabel.setBounds(row1.removeFromLeft(130));
        clockSourceBox.setBounds(row1);
        productInner.removeFromTop(10);

        auto row2 = productInner.removeFromTop(28);
        stepResolutionLabel.setBounds(row2.removeFromLeft(130));
        stepResolutionBox.setBounds(row2);
        productInner.removeFromTop(10);

        auto row3 = productInner.removeFromTop(28);
        tempoLabel.setBounds(row3.removeFromLeft(130));
        tempoSlider.setBounds(row3);
        productInner.removeFromTop(10);

        auto row4 = productInner.removeFromTop(28);
        dryWetLabel.setBounds(row4.removeFromLeft(130));
        dryWetSlider.setBounds(row4);
        productInner.removeFromTop(10);

        auto row5 = productInner.removeFromTop(28);
        outputGainLabel.setBounds(row5.removeFromLeft(130));
        outputGainSlider.setBounds(row5);
        productInner.removeFromTop(10);

        auto row6 = productInner.removeFromTop(28);
        mixModeLabel.setBounds(row6.removeFromLeft(130));
        mixModeBox.setBounds(row6);
        productInner.removeFromTop(12);
        bypassToggle.setBounds(productInner.removeFromTop(24));

        auto notesInner = settingsNotesZone.reduced(18, 16);
        settingsNotesTitle.setBounds(notesInner.removeFromTop(20));
        notesInner.removeFromTop(8);
        settingsNotesBody.setBounds(notesInner);
    }
}

int WorkspacePanel::getNumRows()
{
    return static_cast<int>(filteredPresetIndices.size());
}

void WorkspacePanel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(filteredPresetIndices.size()))
        return;

    const auto& item = presetItems[static_cast<size_t>(filteredPresetIndices[static_cast<size_t>(rowNumber)])];
    auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(4, 4);

    g.setColour(rowIsSelected ? Colours::neonGreen.withAlpha(0.20f) : Colours::bgSurface.withAlpha(0.85f));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(rowIsSelected ? Colours::neonGreen : Colours::white10);
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.0f);

    auto inner = bounds.reduced(10, 7);
        auto top = inner.removeFromTop(18);
        g.setColour(item.isFactory ? Colours::neonGreen : Colours::laneFX2);
        g.setFont(juce::Font(juce::FontOptions().withHeight(16.0f).withStyle("Bold")));
        g.drawText(item.name, top, juce::Justification::centredLeft, false);

        auto badge = top.removeFromRight(76);
        g.setColour(Colours::bgAccent.withAlpha(0.9f));
        g.fillRoundedRectangle(badge.toFloat(), 5.0f);
        g.setColour(item.isFactory ? Colours::neonGreen : Colours::laneFX2);
        g.drawRoundedRectangle(badge.toFloat(), 5.0f, 1.0f);
        g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
        g.drawText(item.category.toUpperCase(), badge, juce::Justification::centred, false);

        g.setColour(Colours::white50);
        g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
        g.drawText(item.subtitle, inner.removeFromTop(16), juce::Justification::centredLeft, false);
}

void WorkspacePanel::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected < 0 || lastRowSelected >= static_cast<int>(filteredPresetIndices.size()))
        selectedPresetRow = -1;
    else
        selectedPresetRow = filteredPresetIndices[static_cast<size_t>(lastRowSelected)];

    updatePresetDetail();
}

} // namespace zikada
