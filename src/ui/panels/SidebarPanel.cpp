#include "ui/panels/SidebarPanel.h"
#include "ui/components/PresetIcons.h"

namespace zikada {

namespace {
    constexpr int kGridCols = 5;
    constexpr int kButtonGap = 5;
    constexpr int kHeaderH = 98;
    constexpr int kInfoH = 56;
}

SidebarPanel::SidebarPanel()
{
    infoLabel.setJustificationType(juce::Justification::centredLeft);
    infoLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    infoLabel.setColour(juce::Label::textColourId, Colours::white85);
    infoLabel.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(infoLabel);
}

std::vector<LanePresetDef> SidebarPanel::getPresetsForLane(int lane) const
{
    std::vector<LanePresetDef> presets;

    switch (lane)
    {
        case 0:
            presets = {
                {"1",   "1 slice",          "Slice the audio into 1 part (no slicing)", 5},
                {"2",   "2 slices",         "Slice the audio into 2 equal parts", 6},
                {"3",   "3 slices",         "Slice the audio into 3 equal parts", 7},
                {"4",   "4 slices",         "Slice the audio into 4 equal parts", 8},
                {"5",   "5 slices",         "Slice the audio into 5 equal parts", 9},
                {"6",   "6 slices",         "Slice the audio into 6 equal parts", 10},
                {"8",   "8 slices",         "Slice the audio into 8 equal parts", 11},
                {"10",  "10 slices",        "Slice the audio into 10 equal parts", 12},
                {"12",  "12 slices",        "Slice the audio into 12 equal parts", 13},
                {"14",  "14 slices",        "Slice the audio into 14 equal parts", 14},
                {"16",  "16 slices",        "Slice the audio into 16 equal parts", 15},
                {"FWD", "Forward",          "Play slices in forward order", 16},
                {"REV", "Reverse",          "Play slices in reverse order", 17},
                {"SC",  "Scatter",          "Randomize slice playback order", 18},
                {"RP",  "Repeat",           "Repeat selected slice pattern", 19},
                {"ST",  "Stutter",          "Rapid-fire slice repeats", 20},
            };
            break;
        case 1:
            presets = {
                {">",    "Forward 1/8",    "Loop forward at 1/8 note resolution", 5},
                {">>",   "Forward 1/4",    "Loop forward at 1/4 note resolution", 6},
                {">>>",  "Forward 1/2",    "Loop forward at 1/2 note resolution", 7},
                {"2",    "2 beat",         "2-beat forward loop", 8},
                {"4",    "4 beat",         "4-beat forward loop", 9},
                {"8",    "8 beat",         "8-beat forward loop", 10},
                {"16",   "16 beat",        "16-beat forward loop", 11},
                {"<",    "Reverse 1/8",    "Loop reversed at 1/8 note resolution", 12},
                {"<<",   "Reverse 1/4",    "Loop reversed at 1/4 note resolution", 13},
                {"<<<",  "Reverse 1/2",    "Loop reversed at 1/2 note resolution", 14},
                {"ST16", "Stutter 1/16",   "1/16-note stutter effect", 15},
                {"ST8",  "Stutter 1/8",    "1/8-note stutter effect", 16},
                {"ST4",  "Stutter 1/4",    "1/4-note stutter effect", 17},
                {"FRZ",  "Freeze",         "Freeze and hold loop buffer", 18},
                {"RND",  "Random",         "Random loop start points", 19},
                {"PIT",  "Pitch",          "Pitch-shifted loop playback", 20},
            };
            break;
        case 2:
            presets = {
                {"A",     "Attack",         "Rising attack envelope", 5},
                {"D",     "Decay",          "Falling decay envelope", 6},
                {"S",     "Sustain",        "Sustain-hold envelope", 7},
                {"R",     "Release",        "Long release tail envelope", 8},
                {"PLK",   "Pluck",          "Short plucked envelope", 9},
                {"PAD",   "Pad",            "Slow swelling pad envelope", 10},
                {"RISE",  "Rise",           "Saw-up rising envelope", 11},
                {"GATE",  "Gate",           "Hard gate / square envelope", 12},
                {"SWELL", "Swell",          "Smooth volume swell", 13},
                {"FADE",  "Fade",           "Linear fade in/out", 14},
                {"TREM",  "Tremolo",        "Amplitude tremolo envelope", 15},
                {"WOBBLE","Wobble",         "Wobbling modulation envelope", 16},
                {"PUNCH", "Punch",          "Punchy transient envelope", 17},
                {"SNAP",  "Snap",           "Snappy attack envelope", 18},
                {"GLIDE", "Glide",          "Gliding volume envelope", 19},
                {"HOLD",  "Hold",           "Hold-sustain envelope", 20},
            };
            break;
        case 3:
            presets = {
                {"DLY-I",  "Delay Mono",      "Clean mono delay", 5},
                {"DLY-II", "Delay Mod",       "Modulated stereo delay", 6},
                {"REV-I",  "Reverb Room",     "Small room reverb", 7},
                {"REV-II", "Reverb Hall",     "Large hall reverb", 8},
                {"CRS-I",  "Chorus Light",    "Subtle chorus thickening", 9},
                {"CRS-II", "Chorus Deep",     "Heavy deep chorus", 10},
                {"FLG-I",  "Flanger Slow",    "Slow sweeping flanger", 11},
                {"FLG-II", "Flanger Fast",    "Fast resonant flanger", 12},
                {"PHS-I",  "Phaser Light",    "Gentle 2-stage phaser", 13},
                {"PHS-II", "Phaser Heavy",    "Intense 8-stage phaser", 14},
                {"TRE-I",  "Tremolo Smooth",  "Smooth sine tremolo", 15},
                {"TRE-II", "Tremolo Chop",    "Choppy square tremolo", 16},
                {"DST-I",  "Distortion Warm", "Warm soft-clipping drive", 17},
                {"DST-II", "Distortion Hard", "Aggressive hard distortion", 18},
                {"GRN-I",  "Grain Fine",      "Fine granular texture", 19},
                {"GRN-II", "Grain Coarse",    "Coarse heavy grain effect", 20},
            };
            break;
        case 4:
            presets = {
                {"LP12", "Low Pass 12",     "12dB/octave low-pass filter", 5},
                {"LP24", "Low Pass 24",     "24dB/octave low-pass filter", 6},
                {"HP12", "High Pass 12",    "12dB/octave high-pass filter", 7},
                {"HP24", "High Pass 24",    "24dB/octave high-pass filter", 8},
                {"BP",   "Band Pass",       "Band-pass filter", 9},
                {"BR",   "Band Reject",     "Band-reject / notch filter", 10},
                {"COMB", "Comb",            "Comb filter resonance", 11},
                {"FRM",  "Formant",         "Vocal formant filter", 12},
                {"A",    "Vowel A",         "Vowel A formant", 13},
                {"E",    "Vowel E",         "Vowel E formant", 14},
                {"I",    "Vowel I",         "Vowel I formant", 15},
                {"O",    "Vowel O",         "Vowel O formant", 16},
                {"U",    "Vowel U",         "Vowel U formant", 17},
                {"MRP",  "Morph",           "Morphing vowel filter", 18},
                {"TLK",  "Talk",            "Talk-box style filter", 19},
                {"WRM",  "Warm",            "Warm vintage filter", 20},
            };
            break;
        case 5:
            presets = {
                {"BTC-I",  "Bitcrush Light",  "Light 8-bit bitcrush", 5},
                {"BTC-II", "Bitcrush Heavy",  "Heavy 4-bit destruction", 6},
                {"PIT-I",  "Pitch Up",        "Pitch shift up +7 semitones", 7},
                {"PIT-II", "Pitch Down",      "Pitch shift down -7 semitones", 8},
                {"VIN-I",  "Vinyl Stop",      "Vinyl stop / pitch-down effect", 9},
                {"VIN-II", "Vinyl Scratch",   "Vinyl scratch / pitch wobble", 10},
                {"STR-I",  "Stretch Tight",   "Tight time-stretch", 11},
                {"STR-II", "Stretch Loose",   "Loose granular stretch", 12},
                {"RNG-I",  "RingMod Subtle",  "Subtle ring modulation", 13},
                {"RNG-II", "RingMod Extreme", "Extreme ring-mod distortion", 14},
                {"TON-I",  "Tonalizer Bright","Bright tuned delay tails", 15},
                {"TON-II", "Tonalizer Dark",  "Dark moody tonalizer", 16},
                {"CHS-I",  "Chaos Random",    "Random chaotic parameters", 17},
                {"CHS-II", "Chaos Structured","Patterned chaotic motion", 18},
                {"SPC-I",  "Space Wash",      "Long washed-out reverb", 19},
                {"SPC-II", "Space Shimmer",   "Shimmering ethereal space", 20},
            };
            break;
        default:
            break;
    }

    presets.push_back({"U1", "User slot 1", "Custom user-defined configuration 1", 1});
    presets.push_back({"U2", "User slot 2", "Custom user-defined configuration 2", 2});
    presets.push_back({"U3", "User slot 3", "Custom user-defined configuration 3", 3});
    presets.push_back({"U4", "User slot 4", "Custom user-defined configuration 4", 4});

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

    hoveredPresetIndex = -1;
    updateInfoForHover(-1);
    resized();
}

void SidebarPanel::highlightPresetButton(int presetIndex)
{
    auto presets = getPresetsForLane(currentLane);
    for (size_t i = 0; i < presets.size() && i < presetButtons.size(); ++i)
    {
        bool active = presets[i].presetIndex == presetIndex;
        presetButtons[i]->setToggleState(active, juce::dontSendNotification);
    }
    repaint();
}

void SidebarPanel::updateInfoForHover(int presetIndex)
{
    if (presetIndex < 0)
    {
        infoLabel.setText("Hover a preset to see details...", juce::dontSendNotification);
        infoLabel.setColour(juce::Label::textColourId, Colours::white50);
        return;
    }

    auto presets = getPresetsForLane(currentLane);
    for (const auto& p : presets)
    {
        if (p.presetIndex == presetIndex)
        {
            infoLabel.setText(p.infoText, juce::dontSendNotification);
            infoLabel.setColour(juce::Label::textColourId, Colours::white85);
            return;
        }
    }
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

    ZikadaLookAndFeel::drawDeviceDisplay(g, headerBounds.toNearestInt());

    auto headerInner = headerBounds.reduced(6.0f, 6.0f);
    g.setColour(laneColour.withAlpha(0.90f));
    g.fillRoundedRectangle(headerInner.withWidth(4.0f).withTrimmedTop(8.0f).withTrimmedBottom(8.0f), 2.0f);

    float iconSize = headerInner.getHeight() * 0.55f;
    auto iconBounds = juce::Rectangle<float>(
        headerInner.getX() + 14.0f,
        headerInner.getCentreY() - iconSize * 0.5f,
        iconSize, iconSize);
    PresetIcons::drawLaneIcon(g, currentLane, iconBounds, laneColour, 2.0f);

    auto textBounds = headerInner.withTrimmedLeft(iconSize + 28.0f);
    g.setFont(juce::Font(juce::FontOptions().withHeight(20.0f).withStyle("Bold")));
    g.setColour(Colours::white);
    g.drawText(laneInfos[currentLane].name,
               textBounds.withHeight(textBounds.getHeight() * 0.55f),
               juce::Justification::centredLeft, false);

    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.setColour(Colours::white50);
    g.drawText("SELECT PRESET",
               textBounds.withY(textBounds.getY() + textBounds.getHeight() * 0.48f)
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
        bool hovered = (presets[i].presetIndex == hoveredPresetIndex);
        auto iconColour = active ? Colours::bgPrimary : (hovered ? laneColour.brighter(0.3f) : laneColour);
        PresetIcons::drawPresetIcon(g, currentLane, presets[i].presetIndex,
                                    btnBounds, iconColour);
    }
}

void SidebarPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8, 8);

    bounds.removeFromTop(kHeaderH);
    bounds.removeFromTop(10);

    auto infoArea = bounds.removeFromBottom(kInfoH);
    infoArea.removeFromTop(6);
    infoLabel.setBounds(infoArea);

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

void SidebarPanel::mouseMove(const juce::MouseEvent& e)
{
    int prevHover = hoveredPresetIndex;
    hoveredPresetIndex = -1;
    auto presets = getPresetsForLane(currentLane);
    for (size_t i = 0; i < presetButtons.size() && i < presets.size(); ++i)
    {
        if (presetButtons[i]->getBounds().contains(e.getPosition()))
        {
            hoveredPresetIndex = presets[i].presetIndex;
            break;
        }
    }
    if (hoveredPresetIndex != prevHover)
    {
        updateInfoForHover(hoveredPresetIndex);
        repaint();
    }
}

void SidebarPanel::mouseExit(const juce::MouseEvent&)
{
    if (hoveredPresetIndex != -1)
    {
        hoveredPresetIndex = -1;
        updateInfoForHover(-1);
        repaint();
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

juce::String SidebarPanel::getPresetTooltip(int lane, int presetIndex) const
{
    auto presets = getPresetsForLane(lane);
    for (const auto& p : presets)
        if (p.presetIndex == presetIndex)
            return p.tooltip;
    return "";
}

juce::String SidebarPanel::getPresetInfo(int lane, int presetIndex) const
{
    auto presets = getPresetsForLane(lane);
    for (const auto& p : presets)
        if (p.presetIndex == presetIndex)
            return p.infoText;
    return "";
}

}
