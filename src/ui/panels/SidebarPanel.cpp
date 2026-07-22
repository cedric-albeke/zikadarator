#include "ui/panels/SidebarPanel.h"
#include "ui/components/PresetIcons.h"

namespace zikada {

namespace {
    constexpr int kFactoryGridCols = 4;
    constexpr int kFactoryPresetCount = 16;
    constexpr int kUserSlotCols = 4;
    constexpr int kButtonGap = 6;
    constexpr int kHeaderH = 80;
    constexpr int kInfoH = 84;
    constexpr int kUserSlotSectionGap = 18;
    constexpr int kUserSlotLabelH = 14;

    bool isUserSlotButton(size_t index)
    {
        return index >= static_cast<size_t>(kFactoryPresetCount);
    }

    void rebuildSidebarLayoutAsync(juce::Component::SafePointer<SidebarPanel> panel)
    {
        juce::MessageManager::callAsync([panel]
        {
            if (panel == nullptr)
                return;

            panel->resized();
            panel->repaint();
        });
    }
}

SidebarPanel::SidebarPanel()
{
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setTitle("Lane preset palette");
    setDescription("Preset effects for the selected sequencer step.");
    setHelpText("Use arrow keys to move through presets, then Space or Enter to assign one.");

    infoTitleLabel.setJustificationType(juce::Justification::centredLeft);
    infoTitleLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    infoTitleLabel.setMinimumHorizontalScale(0.82f);
    infoTitleLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(infoTitleLabel);

    infoDetailLabel.setJustificationType(juce::Justification::centredLeft);
    infoDetailLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    infoDetailLabel.setMinimumHorizontalScale(0.78f);
    infoDetailLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(infoDetailLabel);

    setInfoText("NO STEP SELECTED", "Choose a sequencer step to inspect lane presets.", false);
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
                {"SC",  "Scatter",          "Scatter slice playback order", 18},
                {"RP",  "Repeat",           "Repeat selected slice pattern", 19},
                {"ST",  "Stutter",          "Rapid-fire slice repeats", 20},
            };
            break;
        case 1:
            presets = {
                {"F16",  "Forward 1/16",   "Loop forward over a 1/16 note window", 5},
                {"F8",   "Forward 1/8",    "Loop forward over a 1/8 note window", 6},
                {"F4",   "Forward 1/4",    "Loop forward over a 1/4 note window", 7},
                {"F2",   "Forward 1/2",    "Loop forward over a 1/2 note window", 8},
                {"R16",  "Reverse 1/16",   "Loop reversed over a 1/16 note window", 9},
                {"R8",   "Reverse 1/8",    "Loop reversed over a 1/8 note window", 10},
                {"R4",   "Reverse 1/4",    "Loop reversed over a 1/4 note window", 11},
                {"R2",   "Reverse 1/2",    "Loop reversed over a 1/2 note window", 12},
                {"X2",   "Speed x2",       "Forward loop at double playback speed", 13},
                {"X4",   "Speed x4",       "Forward loop at quadruple playback speed", 14},
                {"S2",   "Slow x1/2",      "Forward loop at half playback speed", 15},
                {"S4",   "Slow x1/4",      "Forward loop at quarter playback speed", 16},
                {"RX2",  "Reverse x2",     "Reverse loop at double playback speed", 17},
                {"RX4",  "Reverse x4",     "Reverse loop at quadruple playback speed", 18},
                {"T4",   "Tail 4 Beat",    "Four-beat forward loop, supported down to 20 BPM", 19},
                {"MAX",  "Tail Max",       "Eight-beat forward loop, capped at 12 seconds", 20},
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
                {"DLY-I",  "Delay Clean",     "Clean stereo delay", 5},
                {"DLY-II", "Delay Mod",       "Modulated stereo delay", 6},
                {"REV-I",  "Reverb Room",     "Small room reverb", 7},
                {"REV-II", "Reverb Hall",     "Large hall reverb", 8},
                {"CRS-I",  "Chorus Light",    "Subtle chorus thickening", 9},
                {"CRS-II", "Chorus Deep",     "Heavy deep chorus", 10},
                {"FLG-I",  "Flanger Slow",    "Slow sweeping flanger", 11},
                {"FLG-II", "Flanger Fast",    "Fast resonant flanger", 12},
                {"CMB-S",  "Comb Sweep",      "Step-synced resonant comb sweep", 13},
                {"NCH-S",  "Notch Sweep",     "Step-synced resonant notch sweep", 14},
                {"TRE-I",  "Tremolo Smooth",  "Smooth sine tremolo", 15},
                {"TRE-II", "Tremolo Chop",    "Choppy square tremolo", 16},
                {"DST-I",  "Distortion Warm", "Warm soft-clipping drive", 17},
                {"DST-II", "Distortion Hard", "Aggressive hard distortion", 18},
                {"PCL-I",  "Pitch Color",     "Simple experimental pitch-color wash", 19},
                {"PCR-I",  "Crush Pitch",     "Bitcrush texture with experimental pitch color", 20},
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
                {"LPX",  "Low Pass X",      "Alternate 24dB low-pass color", 12},
                {"LPA",  "Low Soft",        "Alternate 12dB low-pass color", 13},
                {"HPA",  "High Soft",       "Alternate 12dB high-pass color", 14},
                {"BPA",  "Band Focus",      "Alternate band-pass color", 15},
                {"BRA",  "Notch Focus",     "Alternate band-reject color", 16},
                {"CMB2", "Comb 2",          "Alternate comb filter color", 17},
                {"LPB",  "Low Deep",        "Alternate 24dB low-pass color", 18},
                {"BPB",  "Band Narrow",     "Alternate band-pass color", 19},
                {"LPC",  "Low Warm",        "Alternate 12dB low-pass color", 20},
            };
            break;
        case 5:
            presets = {
                {"BTC-I",  "Bitcrush Light",  "Light 8-bit bitcrush", 5},
                {"BTC-II", "Bitcrush Heavy",  "Heavy 4-bit destruction", 6},
                {"PIT-I",  "Pitch Color Up",  "Experimental pitch-color shift upward", 7},
                {"PIT-II", "Pitch Color Down","Experimental pitch-color shift downward", 8},
                {"PDR-I",  "Pitch Drop",      "Simple pitch-color drop into low-pass tone", 9},
                {"WOB-I",  "Short Wobble",    "Short modulated delay wobble", 10},
                {"DLY-L",  "Delay Long",      "Long delay texture", 11},
                {"DLY-X",  "Delay Dense",     "Dense long-delay texture", 12},
                {"RNG-I",  "RingMod Subtle",  "Subtle ring modulation", 13},
                {"RNG-II", "RingMod Extreme", "Extreme ring-mod distortion", 14},
                {"BND-I",  "Band Focus",      "Resonant band-pass tone focus", 15},
                {"LOW-I",  "Dark Lowpass",    "Dark resonant low-pass tone", 16},
                {"CRS-I",  "Crush Texture",   "Crushed digital texture", 17},
                {"PSW-I",  "Pitch Sweep",     "Step-phase pitch-color sweep", 18},
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
        auto btn = std::make_unique<KeyboardTextButton>("");
        btn->setTooltip(presets[i].tooltip);
        btn->setTitle(presets[i].tooltip);
        btn->setDescription(presets[i].infoText);
        btn->setHelpText("Assign " + presets[i].tooltip + " to the selected step.");
        btn->setExplicitFocusOrder(300 + static_cast<int>(i));
        int pidx = presets[i].presetIndex;
        const int buttonIndex = static_cast<int>(i);
        btn->onKeyboardFocus = [safePanel = juce::Component::SafePointer<SidebarPanel>(this),
                                buttonIndex]
        {
            if (safePanel == nullptr)
                return;

            safePanel->focusedPresetButtonIndex = buttonIndex;
            safePanel->updateAccessiblePresetDescription(buttonIndex);
            safePanel->repaint();
        };
        btn->onClick = [this, pidx]
        {
            notifyPresetAssigned(pidx);
            selectedPresetIndex = pidx;
            focusedPresetButtonIndex = findPresetButtonIndex(pidx);
            highlightPresetButton(pidx);
            updateInfoForSelection();
        };
        btn->setClickingTogglesState(true);
        btn->setColour(juce::TextButton::buttonColourId, Colours::bgSurface);
        btn->setColour(juce::TextButton::buttonOnColourId, laneColour);
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
        btn->setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
        btn->onStateChange = [safePanel = juce::Component::SafePointer<SidebarPanel>(this)]
        {
            if (safePanel != nullptr)
                safePanel->repaint();
        };
        addAndMakeVisible(btn.get());
        btn->addMouseListener(this, true);
        presetButtons.push_back(std::move(btn));
    }

    hoveredPresetIndex = -1;
    focusedPresetButtonIndex = presetButtons.empty() ? -1 : 0;
    updateInfoForSelection();
    rebuildSidebarLayoutAsync(juce::Component::SafePointer<SidebarPanel>(this));
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

int SidebarPanel::findPresetButtonIndex(int presetIndex) const
{
    auto presets = getPresetsForLane(currentLane);
    for (size_t i = 0; i < presets.size() && i < presetButtons.size(); ++i)
        if (presets[i].presetIndex == presetIndex)
            return static_cast<int>(i);

    return -1;
}

void SidebarPanel::setInfoText(const juce::String& title, const juce::String& detail, bool active)
{
    infoTitleLabel.setText(title, juce::dontSendNotification);
    infoDetailLabel.setText(detail, juce::dontSendNotification);

    infoTitleLabel.setColour(juce::Label::textColourId, active ? Colours::white : Colours::white50);
    infoDetailLabel.setColour(juce::Label::textColourId, active ? Colours::white.withAlpha(0.68f) : Colours::white50);
}

void SidebarPanel::updateInfoForHover(int presetIndex)
{
    if (presetIndex < 0)
    {
        updateInfoForSelection();
        return;
    }

    auto presets = getPresetsForLane(currentLane);
    for (const auto& p : presets)
    {
        if (p.presetIndex == presetIndex)
        {
            setInfoText(p.tooltip.toUpperCase(), p.infoText, true);
            return;
        }
    }

}

void SidebarPanel::updateInfoForSelection()
{
    if (currentLane < 0 || !hasSelection)
    {
        setInfoText("NO STEP SELECTED", "Choose a sequencer step to inspect lane presets.", false);
        return;
    }

    if (selectedPresetIndex <= 0)
    {
        setInfoText("NO PRESET", "Choose a preset icon to arm this lane.", false);
        return;
    }

    auto presets = getPresetsForLane(currentLane);
    for (const auto& p : presets)
    {
        if (p.presetIndex == selectedPresetIndex)
        {
            setInfoText(p.tooltip.toUpperCase(), p.infoText, true);
            return;
        }
    }

    setInfoText("PRESET UNAVAILABLE", "This lane cannot use the selected preset.", false);
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
    auto headerBounds = bounds.removeFromTop(static_cast<float>(kHeaderH));

    ZikadaLookAndFeel::drawDeviceDisplay(g, headerBounds.toNearestInt());

    auto headerInner = headerBounds.reduced(12, 8).toFloat();
    g.setColour(laneColour.withAlpha(0.90f));
    g.fillRoundedRectangle(headerInner.withWidth(4.0f).withTrimmedTop(6.0f).withTrimmedBottom(6.0f), 2.0f);

    // Subtle gradient overlay on header
    juce::ColourGradient headerGrad(
        Colours::white.withAlpha(0.06f), headerBounds.getX(), headerBounds.getY(),
        juce::Colours::transparentBlack, headerBounds.getX(), headerBounds.getBottom(), false);
    g.setGradientFill(headerGrad);
    g.fillRoundedRectangle(headerBounds.reduced(1.0f), 2.0f);

    float iconSize = headerInner.getHeight() * 0.55f;
    auto iconBounds = juce::Rectangle<float>(
        headerInner.getX() + 12.0f,
        headerInner.getCentreY() - iconSize * 0.5f,
        iconSize, iconSize);
    PresetIcons::drawLaneIcon(g, currentLane, iconBounds, laneColour, 2.0f);

    auto textBounds = headerInner.withTrimmedLeft(iconSize + 24.0f);
    g.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("Bold")));
    g.setColour(Colours::white);
    g.drawText(laneInfos[currentLane].name,
               textBounds.withHeight(textBounds.getHeight() * 0.55f),
               juce::Justification::centredLeft, false);

    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    g.setColour(Colours::white50);
    g.drawText("SELECT PRESET",
               textBounds.withY(textBounds.getY() + textBounds.getHeight() * 0.48f)
                         .withHeight(textBounds.getHeight() * 0.45f),
               juce::Justification::centredLeft, false);

    auto infoBounds = getLocalBounds().removeFromBottom(kInfoH).toFloat().reduced(8, 4);
    ZikadaLookAndFeel::drawDeviceDisplay(g, infoBounds.toNearestInt());

    if (currentLane >= 0)
    {
        auto accent = infoBounds.reduced(8.0f, 10.0f).withWidth(3.0f);
        g.setColour(laneColour.withAlpha(0.78f));
        g.fillRoundedRectangle(accent, 1.5f);
    }
}

void SidebarPanel::paintOverChildren(juce::Graphics& g)
{
    if (currentLane < 0 || presetButtons.empty())
        return;

    auto presets = getPresetsForLane(currentLane);
    auto laneColour = laneInfos[currentLane].colour;
    auto labelFont = juce::Font(juce::FontOptions().withHeight(9.0f).withStyle("Bold"));

    auto drawPresetLabel = [&](juce::Rectangle<float> bounds, const juce::String& label, juce::Colour colour)
    {
        g.setFont(labelFont);
        g.setColour(colour);
        g.drawText(label, bounds, juce::Justification::centred, true);
    };

    for (size_t i = 0; i < presetButtons.size() && i < presets.size(); ++i)
    {
        auto fullBounds = presetButtons[i]->getBounds().toFloat().reduced(4.0f);
        bool active = presetButtons[i]->getToggleState();
        bool hovered = (presets[i].presetIndex == hoveredPresetIndex);
        bool focused = hasKeyboardFocus(false) && static_cast<int>(i) == focusedPresetButtonIndex;
        auto iconColour = active ? Colours::bgPrimary : (hovered ? laneColour.brighter(0.3f) : laneColour);
        auto iconBounds = fullBounds.withTrimmedBottom(12.0f).reduced(isUserSlotButton(i) ? 6.0f : 4.0f);
        PresetIcons::drawPresetIcon(g, currentLane, presets[i].presetIndex,
                                    iconBounds, iconColour);
        drawPresetLabel(fullBounds.removeFromBottom(11.0f), presets[i].label,
                        active ? Colours::bgPrimary : Colours::white.withAlpha(hovered ? 0.92f : 0.68f));

        if (focused)
        {
            auto focusBounds = presetButtons[i]->getBounds().toFloat().reduced(2.0f);
            g.setColour(Colours::white.withAlpha(0.38f));
            g.drawRoundedRectangle(focusBounds, 4.0f, 1.4f);
            g.setColour(laneColour.withAlpha(0.32f));
            g.drawRoundedRectangle(focusBounds.expanded(2.0f), 5.0f, 1.0f);
        }
    }

    if (presetButtons.size() > static_cast<size_t>(kFactoryPresetCount))
    {
        const auto firstUserBounds = presetButtons[static_cast<size_t>(kFactoryPresetCount)]->getBounds();
        const auto labelBounds = juce::Rectangle<int>(firstUserBounds.getX(),
                                                      firstUserBounds.getY() - kUserSlotLabelH - 3,
                                                      getWidth() - firstUserBounds.getX() - 10,
                                                      kUserSlotLabelH).toFloat();
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f).withStyle("Bold")));
        g.setColour(Colours::white50);
        g.drawText("USER SLOTS", labelBounds, juce::Justification::centredLeft, false);
        g.setColour(laneColour.withAlpha(0.22f));
        g.drawLine(labelBounds.getX() + 68.0f, labelBounds.getCentreY(),
                   labelBounds.getRight(), labelBounds.getCentreY(), 1.0f);
    }
}

void SidebarPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10, 10);

    bounds.removeFromTop(kHeaderH);
    bounds.removeFromTop(8);

    auto infoArea = bounds.removeFromBottom(kInfoH);
    auto infoTextArea = infoArea.reduced(18, 11);
    infoTextArea.removeFromLeft(5);
    infoTitleLabel.setBounds(infoTextArea.removeFromTop(24));
    infoTextArea.removeFromTop(3);
    infoDetailLabel.setBounds(infoTextArea);

    auto gridArea = bounds;
    const int cols = kFactoryGridCols;
    const int factoryCount = juce::jmin(static_cast<int>(presetButtons.size()), kFactoryPresetCount);
    const int userCount = juce::jmax(0, static_cast<int>(presetButtons.size()) - kFactoryPresetCount);
    const int factoryRows = factoryCount > 0 ? (factoryCount + cols - 1) / cols : 0;
    const int userRows = userCount > 0 ? (userCount + kUserSlotCols - 1) / kUserSlotCols : 0;
    const int totalRows = factoryRows + userRows;
    const int availW = gridArea.getWidth();
    const int sectionExtraH = userCount > 0 ? kUserSlotSectionGap + kUserSlotLabelH : 0;
    const int availH = gridArea.getHeight() - sectionExtraH;
    const int cellW = juce::jmax(40, (availW - (cols - 1) * kButtonGap) / cols);
    const int cellH = juce::jmax(40, totalRows > 0 ? (availH - (totalRows - 1) * kButtonGap) / totalRows : availH);
    const int cellSize = juce::jmin(cellW, cellH);

    for (size_t i = 0; i < presetButtons.size(); ++i)
    {
        const bool userSlot = isUserSlotButton(i);
        const int localIndex = userSlot ? static_cast<int>(i) - kFactoryPresetCount : static_cast<int>(i);
        const int layoutCols = userSlot ? kUserSlotCols : kFactoryGridCols;
        int c = localIndex % layoutCols;
        int r = localIndex / layoutCols;
        int x = gridArea.getX() + c * (cellSize + kButtonGap);
        int y = gridArea.getY() + r * (cellSize + kButtonGap);
        if (userSlot)
            y += factoryRows * (cellSize + kButtonGap) + kUserSlotSectionGap + kUserSlotLabelH;
        presetButtons[i]->setBounds(x, y, cellSize, cellSize);
    }
}

void SidebarPanel::mouseMove(const juce::MouseEvent& e)
{
    if (currentLane < 0)
        return;

    auto localPos = e.getEventRelativeTo(this).getPosition();
    int prevHover = hoveredPresetIndex;
    hoveredPresetIndex = -1;
    auto presets = getPresetsForLane(currentLane);
    for (size_t i = 0; i < presetButtons.size() && i < presets.size(); ++i)
    {
        if (presetButtons[i]->getBounds().contains(localPos))
        {
            hoveredPresetIndex = presets[i].presetIndex;
            break;
        }
    }
    if (hoveredPresetIndex != prevHover)
    {
        if (hoveredPresetIndex > 0)
            focusedPresetButtonIndex = findPresetButtonIndex(hoveredPresetIndex);
        updateInfoForHover(hoveredPresetIndex);
        repaint();
    }
}

void SidebarPanel::mouseExit(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getEventRelativeTo(this).getPosition()))
        return;

    if (hoveredPresetIndex != -1)
    {
        hoveredPresetIndex = -1;
        updateInfoForSelection();
        repaint();
    }
}

void SidebarPanel::setSelectedStep(int lane, int step, const StepData& stepData)
{
    bool laneChanged = (lane != currentLane);
    currentLane = lane;
    currentStep = step;
    hasSelection = true;
    selectedPresetIndex = stepData.active ? stepData.presetIndex : -1;

    if (laneChanged)
        buildPresetGrid();
    else
        updateInfoForSelection();

    const int selectedButtonIndex = findPresetButtonIndex(stepData.presetIndex);
    if (selectedButtonIndex >= 0)
        focusedPresetButtonIndex = selectedButtonIndex;
    else if (focusedPresetButtonIndex < 0 && !presetButtons.empty())
        focusedPresetButtonIndex = 0;

    highlightPresetButton(stepData.presetIndex);
}

void SidebarPanel::notifyPresetAssigned(int presetIndex)
{
    if (!onPresetAssigned || !hasSelection)
        return;

    onPresetAssigned(currentLane, currentStep, presetIndex);
}

void SidebarPanel::moveFocusedPresetBy(int columnDelta, int rowDelta)
{
    if (presetButtons.empty())
        return;

    const int current = focusedPresetButtonIndex >= 0 ? focusedPresetButtonIndex : 0;
    const bool userSlot = isUserSlotButton(static_cast<size_t>(current));
    const int sectionStart = userSlot ? kFactoryPresetCount : 0;
    const int sectionEnd = userSlot ? static_cast<int>(presetButtons.size()) : juce::jmin(kFactoryPresetCount, static_cast<int>(presetButtons.size()));
    const int cols = userSlot ? kUserSlotCols : kFactoryGridCols;
    const int local = current - sectionStart;
    const int row = local / cols;
    const int col = local % cols;
    int target = sectionStart + (row + rowDelta) * cols + col + columnDelta;

    if (!userSlot && rowDelta > 0 && target >= sectionEnd && sectionEnd < static_cast<int>(presetButtons.size()))
        target = sectionEnd + juce::jlimit(0, kUserSlotCols - 1, col);
    else if (userSlot && rowDelta < 0 && target < sectionStart)
        target = juce::jlimit(0, sectionStart - 1, (kFactoryPresetCount - kFactoryGridCols) + col);

    target = juce::jlimit(0, static_cast<int>(presetButtons.size()) - 1, target);
    focusedPresetButtonIndex = target;

    if (auto* targetButton = presetButtons[static_cast<size_t>(focusedPresetButtonIndex)].get())
        targetButton->grabKeyboardFocus();

    auto presets = getPresetsForLane(currentLane);
    if (static_cast<size_t>(focusedPresetButtonIndex) < presets.size())
        updateInfoForHover(presets[static_cast<size_t>(focusedPresetButtonIndex)].presetIndex);
    updateAccessiblePresetDescription(focusedPresetButtonIndex);
    repaint();
}

void SidebarPanel::updateAccessiblePresetDescription(int buttonIndex)
{
    const auto presets = getPresetsForLane(currentLane);
    if (buttonIndex < 0 || static_cast<size_t>(buttonIndex) >= presets.size())
        return;

    const auto& preset = presets[static_cast<size_t>(buttonIndex)];
    setTitle("Lane preset palette: " + preset.tooltip);
    setDescription("Focused preset " + preset.tooltip + ". " + preset.infoText);
    updateInfoForHover(preset.presetIndex);

    if (auto* handler = getAccessibilityHandler())
        handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
}

void SidebarPanel::activateFocusedPreset()
{
    auto presets = getPresetsForLane(currentLane);
    if (focusedPresetButtonIndex < 0 || static_cast<size_t>(focusedPresetButtonIndex) >= presets.size())
        return;

    const auto presetIndex = presets[static_cast<size_t>(focusedPresetButtonIndex)].presetIndex;
    notifyPresetAssigned(presetIndex);
    selectedPresetIndex = presetIndex;
    highlightPresetButton(presetIndex);
    updateInfoForSelection();
}

bool SidebarPanel::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::leftKey)  { moveFocusedPresetBy(-1, 0); return true; }
    if (key == juce::KeyPress::rightKey) { moveFocusedPresetBy(1, 0); return true; }
    if (key == juce::KeyPress::upKey)    { moveFocusedPresetBy(0, -1); return true; }
    if (key == juce::KeyPress::downKey)  { moveFocusedPresetBy(0, 1); return true; }
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
    {
        activateFocusedPreset();
        return true;
    }

    return false;
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
