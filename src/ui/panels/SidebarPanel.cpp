#include "ui/panels/SidebarPanel.h"

namespace zikada {

namespace {
    constexpr int kGridCols = 4;
    constexpr int kButtonGap = 4;
}

SidebarPanel::SidebarPanel()
{
    laneHeaderLabel.setJustificationType(juce::Justification::centred);
    laneHeaderLabel.setFont(juce::Font(juce::FontOptions().withHeight(16.0f).withStyle("Bold")));
    addAndMakeVisible(laneHeaderLabel);
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
    for (auto& btn : presetButtons)
        if (btn != nullptr)
            removeChildComponent(btn.get());
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
            notifyPresetAssigned(pidx);
            highlightPresetButton(pidx);
        };
        btn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        btn->setColour(juce::TextButton::buttonOnColourId, laneColour);
        btn->setColour(juce::TextButton::textColourOffId, Colours::white85);
        btn->setColour(juce::TextButton::textColourOnId, Colours::bgPrimary);
        addAndMakeVisible(btn.get());
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

    auto gridArea = bounds;
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

void SidebarPanel::notifyPresetAssigned(int presetIndex)
{
    if (!onPresetAssigned || !hasSelection)
        return;

    onPresetAssigned(currentLane, currentStep, presetIndex);
}

}
