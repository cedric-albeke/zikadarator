#include "ui/panels/SidebarPanel.h"

namespace zikada {

namespace {
    constexpr std::array<double, 7> kKnobMin  = { 20.0, 0.1,   0.0, 0.0, 0.0, 0.0, -1.0 };
    constexpr std::array<double, 7> kKnobMax  = { 20000.0, 10.0, 1.0, 1.0, 1.0, 2.0,  1.0 };
    constexpr std::array<double, 7> kKnobDef  = { 2000.0, 0.707, 0.25, 0.3, 0.5, 1.0,  0.0 };
    constexpr std::array<const char*, 7> kKnobLabel = {
        "CUTOFF", "RESON", "DELAY", "FEEDBK", "MIX", "VOL", "PAN"
    };
    constexpr int kGridCols = 4;
    constexpr int kButtonGap = 4;
    constexpr int kButtonSize = 48;
}

SidebarPanel::SidebarPanel()
{
    addAndMakeVisible(presetGridContainer);

    laneHeaderLabel.setJustificationType(juce::Justification::centred);
    laneHeaderLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f).withStyle("Bold")));
    addAndMakeVisible(laneHeaderLabel);

    for (int i = 0; i < 4; ++i)
    {
        slotTabs[i].setButtonText("U" + juce::String(i + 1));
        slotTabs[i].setClickingTogglesState(true);
        slotTabs[i].onClick = [this, i]
        {
            currentSlot = i;
            for (int t = 0; t < 4; ++t)
                slotTabs[t].setToggleState(t == i, juce::dontSendNotification);
            repaint();
        };
        slotTabs[i].setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        slotTabs[i].setColour(juce::TextButton::buttonOnColourId, Colours::neonGreen);
        slotTabs[i].setColour(juce::TextButton::textColourOffId, Colours::white85);
        slotTabs[i].setColour(juce::TextButton::textColourOnId, Colours::bgPrimary);
        addAndMakeVisible(slotTabs[i]);
    }
    slotTabs[0].setToggleState(true, juce::dontSendNotification);

    for (int i = 0; i < 7; ++i)
    {
        auto knob = std::make_unique<Knob>();
        knob->setRange(kKnobMin[i], kKnobMax[i]);
        knob->setDefaultValue(kKnobDef[i]);
        knob->setValue(kKnobDef[i]);
        knob->setLabel(kKnobLabel[i]);
        knob->setColour(Colours::neonGreen);
        knob->onValueChange = [this] { notifyUserSlotChanged(); };
        addAndMakeVisible(knob.get());
        paramKnobs[i] = std::move(knob);
    }

    for (int i = 0; i < kNumModSlots; ++i)
    {
        auto targetBtn = std::make_unique<juce::TextButton>("OFF");
        targetBtn->onClick = [this, i] { cycleModTarget(i); };
        targetBtn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        targetBtn->setColour(juce::TextButton::textColourOffId, Colours::white85);
        addAndMakeVisible(targetBtn.get());
        modTargetButtons[i] = std::move(targetBtn);

        auto sourceBtn = std::make_unique<juce::TextButton>("STATIC");
        sourceBtn->onClick = [this, i] { cycleModSource(i); };
        sourceBtn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        sourceBtn->setColour(juce::TextButton::textColourOffId, Colours::white85);
        addAndMakeVisible(sourceBtn.get());
        modSourceButtons[i] = std::move(sourceBtn);

        auto amountSlider = std::make_unique<juce::Slider>();
        amountSlider->setSliderStyle(juce::Slider::LinearHorizontal);
        amountSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        amountSlider->setRange(0.0, 100.0, 1.0);
        amountSlider->setValue(0.0);
        amountSlider->setColour(juce::Slider::thumbColourId, Colours::neonGreen);
        amountSlider->setColour(juce::Slider::trackColourId, Colours::white50);
        amountSlider->setColour(juce::Slider::backgroundColourId, Colours::bgSurface);
        amountSlider->onValueChange = [this] { notifyUserSlotChanged(); };
        addAndMakeVisible(amountSlider.get());
        modAmountSliders[i] = std::move(amountSlider);

        auto paramSlider = std::make_unique<juce::Slider>();
        paramSlider->setSliderStyle(juce::Slider::LinearHorizontal);
        paramSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        paramSlider->setRange(0.0, 7.0, 1.0);
        paramSlider->setValue(0.0);
        paramSlider->setColour(juce::Slider::thumbColourId, Colours::neonGreen);
        paramSlider->setColour(juce::Slider::trackColourId, Colours::white50);
        paramSlider->setColour(juce::Slider::backgroundColourId, Colours::bgSurface);
        paramSlider->onValueChange = [this] { notifyUserSlotChanged(); };
        addAndMakeVisible(paramSlider.get());
        modParamSliders[i] = std::move(paramSlider);

        auto paramLabel = std::make_unique<juce::Label>();
        paramLabel->setText("SHAPE", juce::dontSendNotification);
        paramLabel->setJustificationType(juce::Justification::centred);
        paramLabel->setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        paramLabel->setColour(juce::Label::textColourId, Colours::white50);
        addAndMakeVisible(paramLabel.get());
        modParamLabels[i] = std::move(paramLabel);
    }
}

std::vector<LanePresetDef> SidebarPanel::getPresetsForLane(int lane) const
{
    std::vector<LanePresetDef> presets;

    switch (lane)
    {
        case 0:
            presets = {
                {"1",  "1 slice",      5},
                {"2",  "2 slices",     6},
                {"3",  "3 slices",     7},
                {"4",  "4 slices",     8},
                {"6",  "6 slices",     9},
                {"8",  "8 slices",    10},
                {"12", "12 slices",   11},
                {"16", "16 slices",   12},
            };
            break;
        case 1:
            presets = {
                {">",  "Forward",      5},
                {">>", "2x Forward",   6},
                {"2",  "2 beat",       7},
                {"4",  "4 beat",       8},
                {"<",  "Reverse",      9},
                {"<<", "2x Reverse",  10},
                {"8",  "8 beat",      11},
                {"16", "16 beat",     12},
            };
            break;
        case 2:
            presets = {
                {"A",    "Attack",       5},
                {"D",    "Decay",        6},
                {"S",    "Sustain",      7},
                {"R",    "Release",      8},
                {"PLK",  "Pluck",        9},
                {"PAD",  "Pad",         10},
                {"RISE", "Rise",        11},
                {"GATE", "Gate",        12},
            };
            break;
        case 3:
            presets = {
                {"DLY", "Delay",        5},
                {"REV", "Reverb",       6},
                {"CRS", "Chorus",       7},
                {"BTC", "Bitcrush",     8},
                {"PIT", "Pitch",        9},
                {"FLG", "Flanger",     10},
                {"PHS", "Phaser",      11},
                {"TRE", "Tremolo",     12},
            };
            break;
        case 4:
            presets = {
                {"LP", "Low Pass",      5},
                {"HP", "High Pass",     6},
                {"BP", "Band Pass",     7},
                {"NT", "Notch",         8},
                {"A",  "Vowel A",       9},
                {"E",  "Vowel E",      10},
                {"I",  "Vowel I",      11},
                {"O",  "Vowel O",      12},
            };
            break;
        case 5:
            presets = {
                {"DLY", "Delay",        5},
                {"REV", "Reverb",       6},
                {"CRS", "Chorus",       7},
                {"BTC", "Bitcrush",     8},
                {"PIT", "Pitch",        9},
                {"FLG", "Flanger",     10},
                {"PHS", "Phaser",      11},
                {"TRE", "Tremolo",     12},
            };
            break;
        default:
            break;
    }

    presets.push_back({"U1", "User slot 1", 1});
    presets.push_back({"U2", "User slot 2", 2});
    presets.push_back({"U3", "User slot 3", 3});
    presets.push_back({"U4", "User slot 4", 4});

    return presets;
}

void SidebarPanel::buildPresetGrid()
{
    presetButtons.clear();
    if (currentLane < 0)
        return;

    auto presets = getPresetsForLane(currentLane);
    auto laneColour = laneInfos[currentLane].colour;

    for (size_t i = 0; i < presets.size(); ++i)
    {
        auto btn = std::make_unique<juce::TextButton>(presets[i].label);
        btn->setTooltip(presets[i].tooltip);
        int pidx = presets[i].presetIndex;
        btn->onClick = [this, pidx]
        {
            if (pidx >= 1 && pidx <= 4)
            {
                currentSlot = pidx - 1;
                for (int t = 0; t < 4; ++t)
                    slotTabs[t].setToggleState(t == currentSlot, juce::dontSendNotification);
                if (onPresetAssigned)
                    onPresetAssigned(currentLane, currentStep, pidx);
            }
            else
            {
                notifyPresetAssigned(pidx);
            }
            highlightPresetButton(pidx);
        };
        btn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        btn->setColour(juce::TextButton::buttonOnColourId, laneColour);
        btn->setColour(juce::TextButton::textColourOffId, Colours::white85);
        btn->setColour(juce::TextButton::textColourOnId, Colours::bgPrimary);
        presetGridContainer.addAndMakeVisible(btn.get());
        presetButtons.push_back(std::move(btn));
    }

    resized();
}

void SidebarPanel::highlightPresetButton(int presetIndex)
{
    auto presets = getPresetsForLane(currentLane);
    for (size_t i = 0; i < presets.size(); ++i)
    {
        bool active = presets[i].presetIndex == presetIndex;
        presetButtons[i]->setToggleState(active, juce::dontSendNotification);
    }
}

void SidebarPanel::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgPrimary);

    auto bounds = getLocalBounds().toFloat();
    g.setColour(Colours::neonGreen.withAlpha(0.25f));
    g.drawLine(bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom(), 2.0f);
}

void SidebarPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8, 8);

    laneHeaderLabel.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(8);

    auto gridArea = bounds.removeFromTop(260);
    presetGridContainer.setBounds(gridArea);

    const int cols = kGridCols;
    const int rows = static_cast<int>(presetButtons.size() + cols - 1) / cols;
    const int availW = gridArea.getWidth();
    const int availH = gridArea.getHeight();
    const int cellW = juce::jmax(36, (availW - (cols - 1) * kButtonGap) / cols);
    const int cellH = juce::jmax(36, rows > 0 ? (availH - (rows - 1) * kButtonGap) / rows : availH);
    const int cellSize = juce::jmin(cellW, cellH);

    for (size_t i = 0; i < presetButtons.size(); ++i)
    {
        int c = static_cast<int>(i) % cols;
        int r = static_cast<int>(i) / cols;
        int x = gridArea.getX() + c * (cellSize + kButtonGap);
        int y = gridArea.getY() + r * (cellSize + kButtonGap);
        presetButtons[i]->setBounds(x, y, cellSize, cellSize);
    }

    bounds.removeFromTop(12);

    auto tabRow = bounds.removeFromTop(26);
    const int tabW = (tabRow.getWidth() - 12) / 4;
    for (int i = 0; i < 4; ++i)
    {
        slotTabs[i].setBounds(tabRow.removeFromLeft(tabW));
        tabRow.removeFromLeft(4);
    }
    bounds.removeFromTop(8);

    auto knobRow = bounds.removeFromTop(bounds.getHeight() * 40 / 100);
    constexpr int numKnobs = 7;
    const int knobGap = 3;
    const int totalGaps = (numKnobs - 1) * knobGap;
    const int knobW = (knobRow.getWidth() - totalGaps) / numKnobs;
    for (int i = 0; i < numKnobs; ++i)
    {
        int kx = knobRow.getX() + i * (knobW + knobGap);
        paramKnobs[i]->setBounds(kx, knobRow.getY(), knobW, knobRow.getHeight());
    }
    bounds.removeFromTop(8);

    auto modLabel = bounds.removeFromTop(16);
    auto modArea = bounds;

    const int slotGap = 6;
    const int slotW = (modArea.getWidth() - (kNumModSlots - 1) * slotGap) / kNumModSlots;
    const int btnH = 16;
    const int sldH = 12;
    const int lblH = 10;
    const int gap = 2;

    for (int i = 0; i < kNumModSlots; ++i)
    {
        int sx = modArea.getX() + i * (slotW + slotGap);
        int sy = modArea.getY();
        modTargetButtons[i]->setBounds(sx, sy, slotW, btnH);
        sy += btnH + gap;
        modSourceButtons[i]->setBounds(sx, sy, slotW, btnH);
        sy += btnH + gap;
        modAmountSliders[i]->setBounds(sx, sy, slotW, sldH);
        sy += sldH + gap;
        modParamLabels[i]->setBounds(sx, sy, slotW, lblH);
        sy += lblH + gap;
        modParamSliders[i]->setBounds(sx, sy, slotW, std::max(4, modArea.getBottom() - sy));
    }
}

void SidebarPanel::setSelectedStep(int lane, int step, const StepData& stepData)
{
    bool laneChanged = (lane != currentLane);
    currentLane = lane;
    currentStep = step;
    hasSelection = true;

    laneHeaderLabel.setText(juce::String(laneInfos[lane].name), juce::dontSendNotification);
    laneHeaderLabel.setColour(juce::Label::textColourId, laneInfos[lane].colour);

    if (laneChanged)
        buildPresetGrid();

    highlightPresetButton(stepData.presetIndex);
}

void SidebarPanel::setUserSlotData(int lane, int slotIndex, const UserSlotData& data)
{
    if (lane != currentLane)
        return;

    updatingFromState = true;
    currentSlot = slotIndex;
    for (int t = 0; t < 4; ++t)
        slotTabs[t].setToggleState(t == currentSlot, juce::dontSendNotification);

    updateParamKnobs(data);
    updateModulationControls(data.modulation);
    updatingFromState = false;
}

void SidebarPanel::updateParamKnobs(const UserSlotData& data)
{
    paramKnobs[0]->setValue(static_cast<double>(data.filterCutoff));
    paramKnobs[1]->setValue(static_cast<double>(data.filterResonance));
    paramKnobs[2]->setValue(static_cast<double>(data.delayTime));
    paramKnobs[3]->setValue(static_cast<double>(data.delayFeedback));
    paramKnobs[4]->setValue(static_cast<double>(data.delayMix));
    paramKnobs[5]->setValue(static_cast<double>(data.volume));
    paramKnobs[6]->setValue(static_cast<double>(data.pan));
}

void SidebarPanel::updateModulationControls(const ModulationData& modData)
{
    for (int i = 0; i < kNumModSlots; ++i)
    {
        const auto& slot = modData.slots[static_cast<size_t>(i)];
        modTargetButtons[i]->setButtonText(targetToString(slot.target));
        modSourceButtons[i]->setButtonText(sourceToString(slot.source));
        modAmountSliders[i]->setValue(static_cast<double>(slot.amount) * 100.0);
        updateModParamLabel(i);

        switch (slot.source)
        {
            case ModulationSource::Motion:
                modParamSliders[i]->setRange(0.0, 7.0, 1.0);
                modParamSliders[i]->setValue(static_cast<double>(slot.motionShape));
                break;
            case ModulationSource::EnvFollower:
                modParamSliders[i]->setRange(1.0, 500.0, 1.0);
                modParamSliders[i]->setValue(static_cast<double>(slot.envAttack));
                break;
            case ModulationSource::Random:
                modParamSliders[i]->setRange(1.0, 16.0, 1.0);
                modParamSliders[i]->setValue(static_cast<double>(slot.randomRate));
                break;
            default:
                modParamSliders[i]->setRange(0.0, 1.0, 0.01);
                modParamSliders[i]->setValue(0.0);
                break;
        }
    }
}

void SidebarPanel::cycleModTarget(int slot)
{
    if (updatingFromState)
        return;

    int current = static_cast<int>(targetFromString(modTargetButtons[slot]->getButtonText()));
    int mapped = current + 1;
    mapped = (mapped + 1) % (static_cast<int>(ModulationTarget::NumTargets) + 1);
    current = mapped - 1;

    auto t = static_cast<ModulationTarget>(current);
    modTargetButtons[slot]->setButtonText(targetToString(t));
    notifyUserSlotChanged();
}

void SidebarPanel::cycleModSource(int slot)
{
    if (updatingFromState)
        return;

    int current = static_cast<int>(sourceFromString(modSourceButtons[slot]->getButtonText()));
    current = (current + 1) % static_cast<int>(ModulationSource::NumSources);

    auto s = static_cast<ModulationSource>(current);
    modSourceButtons[slot]->setButtonText(sourceToString(s));
    updateModParamLabel(slot);

    switch (s)
    {
        case ModulationSource::Motion:
            modParamSliders[slot]->setRange(0.0, 7.0, 1.0);
            modParamSliders[slot]->setValue(0.0);
            break;
        case ModulationSource::EnvFollower:
            modParamSliders[slot]->setRange(1.0, 500.0, 1.0);
            modParamSliders[slot]->setValue(10.0);
            break;
        case ModulationSource::Random:
            modParamSliders[slot]->setRange(1.0, 16.0, 1.0);
            modParamSliders[slot]->setValue(1.0);
            break;
        default:
            modParamSliders[slot]->setRange(0.0, 1.0, 0.01);
            modParamSliders[slot]->setValue(0.0);
            break;
    }

    notifyUserSlotChanged();
}

void SidebarPanel::updateModParamLabel(int slot)
{
    auto s = sourceFromString(modSourceButtons[slot]->getButtonText());
    switch (s)
    {
        case ModulationSource::Motion:
            modParamLabels[slot]->setText("SHAPE", juce::dontSendNotification);
            break;
        case ModulationSource::EnvFollower:
            modParamLabels[slot]->setText("ATTACK", juce::dontSendNotification);
            break;
        case ModulationSource::Random:
            modParamLabels[slot]->setText("RATE", juce::dontSendNotification);
            break;
        default:
            modParamLabels[slot]->setText("", juce::dontSendNotification);
            break;
    }
}

juce::String SidebarPanel::targetToString(ModulationTarget t)
{
    switch (t)
    {
        case ModulationTarget::FilterCutoff:    return "CUTOFF";
        case ModulationTarget::FilterResonance: return "RESON";
        case ModulationTarget::DelayTime:       return "DELAY";
        case ModulationTarget::DelayFeedback:   return "FEEDBK";
        case ModulationTarget::DelayMix:        return "MIX";
        case ModulationTarget::Volume:          return "VOL";
        case ModulationTarget::Pan:             return "PAN";
        default:                                return "OFF";
    }
}

ModulationTarget SidebarPanel::targetFromString(const juce::String& s)
{
    if (s == "CUTOFF")   return ModulationTarget::FilterCutoff;
    if (s == "RESON")    return ModulationTarget::FilterResonance;
    if (s == "DELAY")    return ModulationTarget::DelayTime;
    if (s == "FEEDBK")   return ModulationTarget::DelayFeedback;
    if (s == "MIX")      return ModulationTarget::DelayMix;
    if (s == "VOL")      return ModulationTarget::Volume;
    if (s == "PAN")      return ModulationTarget::Pan;
    return ModulationTarget::None;
}

juce::String SidebarPanel::sourceToString(ModulationSource s)
{
    switch (s)
    {
        case ModulationSource::Motion:       return "MOTION";
        case ModulationSource::EnvFollower:  return "ENV";
        case ModulationSource::Random:       return "RAND";
        default:                             return "STATIC";
    }
}

ModulationSource SidebarPanel::sourceFromString(const juce::String& s)
{
    if (s == "MOTION")  return ModulationSource::Motion;
    if (s == "ENV")     return ModulationSource::EnvFollower;
    if (s == "RAND")    return ModulationSource::Random;
    return ModulationSource::Static;
}

void SidebarPanel::notifyUserSlotChanged()
{
    if (updatingFromState || !onUserSlotChanged)
        return;

    UserSlotData data;
    data.filterCutoff    = static_cast<float>(paramKnobs[0]->getValue());
    data.filterResonance = static_cast<float>(paramKnobs[1]->getValue());
    data.delayTime       = static_cast<float>(paramKnobs[2]->getValue());
    data.delayFeedback   = static_cast<float>(paramKnobs[3]->getValue());
    data.delayMix        = static_cast<float>(paramKnobs[4]->getValue());
    data.volume          = static_cast<float>(paramKnobs[5]->getValue());
    data.pan             = static_cast<float>(paramKnobs[6]->getValue());

    for (int i = 0; i < kNumModSlots; ++i)
    {
        auto& slot = data.modulation.slots[static_cast<size_t>(i)];
        slot.target = targetFromString(modTargetButtons[i]->getButtonText());
        slot.source = sourceFromString(modSourceButtons[i]->getButtonText());
        slot.amount = static_cast<float>(modAmountSliders[i]->getValue()) / 100.0f;

        switch (slot.source)
        {
            case ModulationSource::Motion:
                slot.motionShape = static_cast<int>(modParamSliders[i]->getValue());
                break;
            case ModulationSource::EnvFollower:
                slot.envAttack = static_cast<float>(modParamSliders[i]->getValue());
                break;
            case ModulationSource::Random:
                slot.randomRate = static_cast<int>(modParamSliders[i]->getValue());
                break;
            default:
                break;
        }
    }

    onUserSlotChanged(currentLane, currentSlot, data);
}

void SidebarPanel::notifyPresetAssigned(int presetIndex)
{
    if (!onPresetAssigned || !hasSelection)
        return;

    onPresetAssigned(currentLane, currentStep, presetIndex);
}

}
