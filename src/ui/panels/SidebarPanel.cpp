#include "ui/panels/SidebarPanel.h"
#include "ui/components/PresetIcons.h"

namespace zikada {

namespace {
    constexpr int kGridCols = 5;
    constexpr int kButtonGap = 5;
    constexpr int kHeaderH = 98;
}

SidebarPanel::SidebarPanel()
{
}

std::vector<LanePresetDef> SidebarPanel::getPresetsForLane(int lane) const
{
    std::vector<LanePresetDef> presets;

    switch (lane)
    {
        case 0:
            presets = {
                {"1",   "1 slice",          5},
                {"2",   "2 slices",         6},
                {"3",   "3 slices",         7},
                {"4",   "4 slices",         8},
                {"5",   "5 slices",         9},
                {"6",   "6 slices",        10},
                {"8",   "8 slices",        11},
                {"10",  "10 slices",       12},
                {"12",  "12 slices",       13},
                {"14",  "14 slices",       14},
                {"16",  "16 slices",       15},
                {"FWD", "Forward",         16},
                {"REV", "Reverse",         17},
                {"SC",  "Scatter",         18},
                {"RP",  "Repeat",          19},
                {"ST",  "Stutter",         20},
            };
            break;
        case 1:
            presets = {
                {">",    "Forward 1/8",    5},
                {">>",   "Forward 1/4",    6},
                {">>>",  "Forward 1/2",    7},
                {"2",    "2 beat",         8},
                {"4",    "4 beat",         9},
                {"8",    "8 beat",        10},
                {"16",   "16 beat",       11},
                {"<",    "Reverse 1/8",   12},
                {"<<",   "Reverse 1/4",   13},
                {"<<<",  "Reverse 1/2",   14},
                {"ST16", "Stutter 1/16",  15},
                {"ST8",  "Stutter 1/8",   16},
                {"ST4",  "Stutter 1/4",   17},
                {"FRZ",  "Freeze",        18},
                {"RND",  "Random",        19},
                {"PIT",  "Pitch",         20},
            };
            break;
        case 2:
            presets = {
                {"A",     "Attack",         5},
                {"D",     "Decay",          6},
                {"S",     "Sustain",        7},
                {"R",     "Release",        8},
                {"PLK",   "Pluck",          9},
                {"PAD",   "Pad",           10},
                {"RISE",  "Rise",          11},
                {"GATE",  "Gate",          12},
                {"SWELL", "Swell",         13},
                {"FADE",  "Fade",          14},
                {"TREM",  "Tremolo",       15},
                {"WOBBLE","Wobble",        16},
                {"PUNCH", "Punch",         17},
                {"SNAP",  "Snap",          18},
                {"GLIDE", "Glide",         19},
                {"HOLD",  "Hold",          20},
            };
            break;
        case 3:
            presets = {
                {"DLY", "Delay",            5},
                {"REV", "Reverb",           6},
                {"CRS", "Chorus",           7},
                {"BTC", "Bitcrush",         8},
                {"PIT", "Pitch",            9},
                {"FLG", "Flanger",         10},
                {"PHS", "Phaser",          11},
                {"TRE", "Tremolo",         12},
                {"DST", "Distortion",      13},
                {"GRN", "Grain",           14},
                {"VIN", "Vinyl",           15},
                {"STR", "Stretch",         16},
                {"RNG", "RingMod",         17},
                {"TON", "Tonalizer",       18},
                {"CHS", "Chaos",           19},
                {"PH2", "Phaser II",       20},
            };
            break;
        case 4:
            presets = {
                {"LP12", "Low Pass 12",     5},
                {"LP24", "Low Pass 24",     6},
                {"HP12", "High Pass 12",    7},
                {"HP24", "High Pass 24",    8},
                {"BP",   "Band Pass",       9},
                {"BR",   "Band Reject",    10},
                {"COMB", "Comb",           11},
                {"FRM",  "Formant",        12},
                {"A",    "Vowel A",        13},
                {"E",    "Vowel E",        14},
                {"I",    "Vowel I",        15},
                {"O",    "Vowel O",        16},
                {"U",    "Vowel U",        17},
                {"MRP",  "Morph",          18},
                {"TLK",  "Talk",           19},
                {"WRM",  "Warm",           20},
            };
            break;
        case 5:
            presets = {
                {"DLY", "Delay",            5},
                {"REV", "Reverb",           6},
                {"CRS", "Chorus",           7},
                {"BTC", "Bitcrush",         8},
                {"PIT", "Pitch",            9},
                {"FLG", "Flanger",         10},
                {"PHS", "Phaser",          11},
                {"TRE", "Tremolo",         12},
                {"DST", "Distortion",      13},
                {"GRN", "Grain",           14},
                {"VIN", "Vinyl",           15},
                {"STR", "Stretch",         16},
                {"RNG", "RingMod",         17},
                {"TON", "Tonalizer",       18},
                {"CHS", "Chaos",           19},
                {"PH2", "Phaser II",       20},
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
        auto btn = std::make_unique<juce::TextButton>("");
        btn->setTooltip(presets[i].tooltip);
        int pidx = presets[i].presetIndex;
        btn->onClick = [this, pidx]
        {
            notifyPresetAssigned(pidx);
            highlightPresetButton(pidx);
        };
        btn->setClickingTogglesState(true);
        btn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        btn->setColour(juce::TextButton::buttonOnColourId, laneColour);
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
        btn->setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
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
    repaint();
}

void SidebarPanel::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgPrimary);

    auto bounds = getLocalBounds().toFloat();
    g.setColour(Colours::neonGreen.withAlpha(0.25f));
    g.drawLine(bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom(), 2.0f);

    if (currentLane < 0)
        return;

    auto laneColour = laneInfos[currentLane].colour;
    auto headerBounds = bounds.removeFromTop(static_cast<float>(kHeaderH)).reduced(8.0f, 8.0f);

    g.setColour(Colours::displayBezel);
    g.fillRoundedRectangle(headerBounds, 6.0f);
    g.setColour(laneColour.withAlpha(0.22f));
    g.drawRoundedRectangle(headerBounds.reduced(0.5f), 6.0f, 1.0f);

    g.setColour(laneColour.withAlpha(0.90f));
    g.fillRoundedRectangle(headerBounds.withWidth(4.0f).withTrimmedTop(10.0f).withTrimmedBottom(10.0f), 2.0f);

    float iconSize = headerBounds.getHeight() * 0.55f;
    auto iconBounds = juce::Rectangle<float>(
        headerBounds.getX() + 14.0f,
        headerBounds.getCentreY() - iconSize * 0.5f,
        iconSize, iconSize);
    PresetIcons::drawLaneIcon(g, currentLane, iconBounds, laneColour, 2.0f);

    auto textBounds = headerBounds.withTrimmedLeft(iconSize + 26.0f);
    g.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("Bold")));
    g.setColour(Colours::white);
    g.drawText(laneInfos[currentLane].name,
               textBounds.withHeight(textBounds.getHeight() * 0.55f),
               juce::Justification::centredLeft, false);

    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.setColour(Colours::white50);
    g.drawText("SELECT PRESET",
               textBounds.withY(textBounds.getY() + textBounds.getHeight() * 0.45f)
                         .withHeight(textBounds.getHeight() * 0.45f),
               juce::Justification::centredLeft, false);
}

void SidebarPanel::paintOverChildren(juce::Graphics& g)
{
    if (currentLane < 0 || presetButtons.empty())
        return;

    auto presets = getPresetsForLane(currentLane);
    auto laneColour = laneInfos[currentLane].colour;

    for (size_t i = 0; i < presetButtons.size() && i < presets.size(); ++i)
    {
        auto btnBounds = presetButtons[i]->getBounds().toFloat().reduced(4.0f);
        bool active = presetButtons[i]->getToggleState();
        auto iconColour = active ? Colours::bgPrimary : laneColour;
        PresetIcons::drawPresetIcon(g, currentLane, presets[i].presetIndex,
                                    btnBounds, iconColour);
    }
}

void SidebarPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8, 8);

    bounds.removeFromTop(kHeaderH);
    bounds.removeFromTop(10);

    auto gridArea = bounds;
    const int cols = kGridCols;
    const int rows = static_cast<int>((presetButtons.size() + cols - 1) / cols);
    const int availW = gridArea.getWidth();
    const int availH = gridArea.getHeight();
    const int cellW = juce::jmax(40, (availW - (cols - 1) * kButtonGap) / cols);
    const int cellH = juce::jmax(40, rows > 0 ? (availH - (rows - 1) * kButtonGap) / rows : availH);
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

juce::String SidebarPanel::getPresetLabel(int lane, int presetIndex) const
{
    if (presetIndex >= 1 && presetIndex <= 4)
        return "U" + juce::String(presetIndex);

    auto presets = getPresetsForLane(lane);
    for (const auto& p : presets)
        if (p.presetIndex == presetIndex)
            return p.label;

    return "";
}

}
