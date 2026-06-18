#include "ui/panels/HeaderPanel.h"
#include "BinaryData.h"

namespace zikada {

namespace HeaderLayout {
    constexpr int kPadX          = 10;
    constexpr int kLogoSize      = 40;
    constexpr int kLogoGap       = 6;
    constexpr int kWordmarkW     = 150;
    constexpr int kSectionGap    = 10;
    constexpr int kTabW          = 90;
    constexpr int kTabGap        = 2;
    constexpr float kTabCorner   = 3.0f;
    constexpr int kPresetStripW  = 320;
    constexpr int kNavW          = 26;
    constexpr int kActionW       = 32;
    constexpr int kActionGap     = 2;
    constexpr float kTabCorner   = 3.0f;
}

static const juce::String kUndoSvg(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
    "<path d=\"M9 14L4 9l5-5\" fill=\"none\" stroke=\"#D9E3E7\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "<path d=\"M4 9h10.5a5.5 5.5 0 0 1 5.5 5.5a5.5 5.5 0 0 1-5.5 5.5H11\" fill=\"none\" stroke=\"#D9E3E7\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "</svg>");

static const juce::String kRedoSvg(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
    "<path d=\"M15 14L20 9L15 4\" fill=\"none\" stroke=\"#D9E3E7\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "<path d=\"M20 9H9.5A5.5 5.5 0 0 0 4 14.5A5.5 5.5 0 0 0 9.5 20H13\" fill=\"none\" stroke=\"#D9E3E7\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "</svg>");

static const juce::String kSaveSvg(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
    "<path d=\"M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z\" fill=\"none\" stroke=\"#8A9BA0\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "<path d=\"M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7\" fill=\"none\" stroke=\"#8A9BA0\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "<path d=\"M7 3v4a1 1 0 0 0 1 1h7\" fill=\"none\" stroke=\"#8A9BA0\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "</svg>");

static std::unique_ptr<juce::Drawable> parseSvgString(const juce::String& svgText)
{
    if (auto xml = juce::parseXML(svgText))
        return juce::Drawable::createFromSVG(*xml);
    return nullptr;
}

HeaderPanel::HeaderPanel()
{
    logoImage = juce::ImageCache::getFromMemory(
        BinaryData::zikatorlogo_png, BinaryData::zikatorlogo_pngSize);

    undoIcon = parseSvgString(kUndoSvg);
    redoIcon = parseSvgString(kRedoSvg);
    saveIcon = parseSvgString(kSaveSvg);

    auto configureTab = [this](juce::TextButton& button, Page page)
    {
        button.setClickingTogglesState(true);
        button.setRadioGroupId(1001);
        button.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
        button.setButtonText("");
        button.onClick = [this, page]
        {
            setSelectedPage(page);
            if (onPageSelected)
                onPageSelected(page);
        };
        addAndMakeVisible(button);
    };

    configureTab(sequencerTab, Page::Sequencer);
    configureTab(presetsTab, Page::Presets);
    configureTab(settingsTab, Page::Settings);

    auto configureHitZone = [](juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
    };

    configureHitZone(undoButton);
    configureHitZone(redoButton);
    undoButton.setButtonText("");
    redoButton.setButtonText("");
    undoButton.onClick = [this] { if (onUndoRequested) onUndoRequested(); };
    redoButton.onClick = [this] { if (onRedoRequested) onRedoRequested(); };
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);

    configureHitZone(presetSelectButton);
    presetSelectButton.setButtonText("");
    presetSelectButton.onClick = [this]
    {
        if (onPresetMenuRequested)
            onPresetMenuRequested();
    };
    addAndMakeVisible(presetSelectButton);

    presetMetaLabel.setJustificationType(juce::Justification::centredLeft);
    presetMetaLabel.setColour(juce::Label::textColourId, Colours::white50);
    addAndMakeVisible(presetMetaLabel);

    configureHitZone(presetPrevButton);
    configureHitZone(presetNextButton);
    presetPrevButton.setButtonText("");
    presetNextButton.setButtonText("");
    presetPrevButton.onClick = [this] { if (onPresetPreviousRequested) onPresetPreviousRequested(); };
    presetNextButton.onClick = [this] { if (onPresetNextRequested) onPresetNextRequested(); };
    addAndMakeVisible(presetPrevButton);
    addAndMakeVisible(presetNextButton);

    setSelectedPage(Page::Sequencer);
    setUndoEnabled(false);
    setRedoEnabled(false);
    setPresetStepEnabled(false, false);
    setPresetDisplay(currentPresetName, currentPresetMeta, false);
}

void HeaderPanel::setSelectedPage(Page page)
{
    selectedPage = page;
    sequencerTab.setToggleState(page == Page::Sequencer, juce::dontSendNotification);
    presetsTab.setToggleState(page == Page::Presets, juce::dontSendNotification);
    settingsTab.setToggleState(page == Page::Settings, juce::dontSendNotification);
    repaint();
}

void HeaderPanel::setUndoEnabled(bool enabled)
{
    undoButton.setEnabled(enabled);
}

void HeaderPanel::setRedoEnabled(bool enabled)
{
    redoButton.setEnabled(enabled);
}

void HeaderPanel::setPresetDisplay(const juce::String& presetName,
                                   const juce::String& presetMeta,
                                   bool dirty)
{
    currentPresetName = presetName;
    currentPresetMeta = presetMeta;
    presetDirty = dirty;
    repaint();
}

void HeaderPanel::setPresetStepEnabled(bool previousEnabled, bool nextEnabled)
{
    presetPrevButton.setEnabled(previousEnabled);
    presetNextButton.setEnabled(nextEnabled);
    repaint();
}

// ── Paint (background, logo, wordmark — drawn BEFORE children) ──────
void HeaderPanel::paint(juce::Graphics& g)
{
    namespace HL = HeaderLayout;
    const auto bounds = getLocalBounds().toFloat();
    const auto* laf   = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    ZikadaLookAndFeel::drawPremiumPanel(g, getLocalBounds(), true);

    g.setColour(Colours::white50);
    g.drawLine(0.0f, bounds.getBottom() - 1.5f,
               bounds.getRight(), bounds.getBottom() - 1.5f, 2.0f);

    auto wordmarkFont = laf ? laf->getAntaFont(22.0f)
                            : juce::Font(juce::FontOptions().withHeight(22.0f));
    auto versionFont  = laf ? laf->getAntaFont(14.0f)
                            : juce::Font(juce::FontOptions().withHeight(14.0f));
    auto sublineFont  = laf ? laf->getSpaceMonoFont(11.0f, false)
                            : juce::Font(juce::FontOptions().withHeight(11.0f));

    // ── Logo (128px pre-scaled asset) ───────────────────────────────
    const float logoSz = static_cast<float>(HL::kLogoSize);
    const float logoX  = static_cast<float>(HL::kPadX);
    const float logoY  = bounds.getCentreY() - logoSz * 0.5f;

    if (logoImage.isValid())
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageWithin(logoImage,
                          static_cast<int>(logoX), static_cast<int>(logoY),
                          static_cast<int>(logoSz), static_cast<int>(logoSz),
                          juce::RectanglePlacement::centred
                              | juce::RectanglePlacement::onlyReduceInSize,
                          false);

        g.setColour(Colours::neonGreen.withAlpha(0.06f));
        g.fillEllipse(logoX + 2.0f, logoY + 2.0f, logoSz - 4.0f, logoSz - 4.0f);
    }

    // ── Wordmark + subline ──────────────────────────────────────────
    const float wmX = logoX + logoSz + static_cast<float>(HL::kLogoGap);

    juce::GlyphArrangement gaZikada, gaRator, gaV1;
    gaZikada.addLineOfText(wordmarkFont, "ZIKADA", 0, 0);
    gaRator.addLineOfText(wordmarkFont, "RATOR", 0, 0);
    gaV1.addLineOfText(versionFont, "V1", 0, 0);

    const float wZikada = gaZikada.getBoundingBox(0, gaZikada.getNumGlyphs(), true).getWidth();
    const float wRator  = gaRator.getBoundingBox(0, gaRator.getNumGlyphs(), true).getWidth();
    const float wV1     = gaV1.getBoundingBox(0, gaV1.getNumGlyphs(), true).getWidth();
    const float totalWmW = wZikada + wRator + wV1 + 4.0f;

    const float wmY = bounds.getCentreY() - 11.0f;

    g.setFont(wordmarkFont);
    g.setColour(Colours::white);
    g.drawText("ZIKADA",
               juce::Rectangle<float>(wmX, wmY, wZikada + 2.0f, 20.0f),
               juce::Justification::centredLeft, false);

    g.setColour(Colours::neonGreen);
    g.drawText("RATOR",
               juce::Rectangle<float>(wmX + wZikada, wmY, wRator + 2.0f, 20.0f),
               juce::Justification::centredLeft, false);

    g.setFont(versionFont);
    g.setColour(Colours::neonGreen.withAlpha(0.65f));
    g.drawText("V1",
               juce::Rectangle<float>(wmX + wZikada + wRator + 2.0f, wmY + 2.0f, wV1 + 2.0f, 16.0f),
               juce::Justification::centredLeft, false);

    g.setFont(sublineFont);
    g.setColour(Colours::white50);
    g.drawText("SEQUENCE THE SIGNAL",
               juce::Rectangle<float>(wmX, wmY + 18.0f, totalWmW, 12.0f),
               juce::Justification::centredLeft, false);
}

// ── PaintOverChildren (tabs, preset strip, icons — drawn AFTER children) ──
void HeaderPanel::paintOverChildren(juce::Graphics& g)
{
    namespace HL = HeaderLayout;
    const auto bounds = getLocalBounds().toFloat();
    const auto* laf   = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    auto tabFont    = laf ? laf->getSpaceMonoFont(14.0f, true)
                          : juce::Font(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));
    auto presetFont = laf ? laf->getSpaceMonoFont(14.0f, false)
                          : juce::Font(juce::FontOptions().withHeight(14.0f));

    // ── Tab cells (underline indicator, no dots) ────────────────────
    auto drawTab = [&](const juce::TextButton& tab, const juce::String& label, bool active)
    {
        auto r = tab.getBounds().toFloat();

        if (active)
        {
            g.setColour(Colours::bgHover.brighter(0.08f));
            g.fillRoundedRectangle(r, HL::kTabCorner);
        }

        g.setColour(Colours::white.withAlpha(active ? 0.10f : 0.06f));
        g.drawRoundedRectangle(r.reduced(0.5f), HL::kTabCorner, 1.0f);

        // Underline indicator at bottom: 2.5px line, 8px wide, 3px from bottom
        if (active)
        {
            const float uLineW = 8.0f;
            const float uLineH = 2.5f;
            const float uLineY = r.getBottom() - 3.0f;
            const float uLineX = r.getCentreX() - uLineW * 0.5f;
            g.setColour(Colours::neonGreen);
            g.fillRoundedRectangle(uLineX, uLineY, uLineW, uLineH, 1.25f);
            // Glow under the line
            g.setColour(Colours::neonGreen.withAlpha(0.25f));
            g.fillRoundedRectangle(uLineX - 2.0f, uLineY + 1.0f, uLineW + 4.0f, uLineH + 2.0f, 2.0f);
        }

        g.setFont(tabFont);
        g.setColour(active ? Colours::white : Colours::white50);
        g.drawText(label, r, juce::Justification::centred, false);
    };

    drawTab(sequencerTab, "SEQUENCER", selectedPage == Page::Sequencer);
    drawTab(presetsTab,   "PRESETS",   selectedPage == Page::Presets);
    drawTab(settingsTab,  "SETTINGS",  selectedPage == Page::Settings);

    // ── Preset strip with integrated nav ────────────────────────────
    {
        auto prevR  = presetPrevButton.getBounds().toFloat();
        auto nextR  = presetNextButton.getBounds().toFloat();
        auto psRect = presetSelectButton.getBounds().toFloat();

        auto stripRect = juce::Rectangle<float>(
            prevR.getX() - 6.0f,
            psRect.getY() - 3.0f,
            nextR.getRight() - prevR.getX() + 12.0f,
            psRect.getHeight() + 6.0f);

        g.setColour(Colours::bgHover.brighter(0.03f));
        g.fillRoundedRectangle(stripRect, 3.0f);
        g.setColour(Colours::white.withAlpha(0.07f));
        g.drawRoundedRectangle(stripRect.reduced(0.5f), 3.0f, 1.0f);

        if (saveIcon != nullptr)
        {
            auto iconArea = juce::Rectangle<float>(
                prevR.getRight() + 6.0f,
                stripRect.getCentreY() - 9.0f,
                18.0f, 18.0f);
            g.setOpacity(0.7f);
            saveIcon->drawWithin(g, iconArea, juce::RectanglePlacement::centred, 1.0f);
            g.setOpacity(1.0f);
        }

        juce::String displayName = currentPresetName + (presetDirty ? " *" : "");
        g.setFont(presetFont);
        g.setColour(Colours::white);
        auto textArea = juce::Rectangle<float>(
            prevR.getRight() + 28.0f,
            stripRect.getY(),
            nextR.getX() - prevR.getRight() - 56.0f,
            stripRect.getHeight());
        g.drawText(displayName, textArea, juce::Justification::centredLeft, true);

        const float triX = nextR.getX() - 18.0f;
        const float triY = stripRect.getCentreY() - 3.0f;
        juce::Path tri;
        tri.addTriangle(triX, triY, triX + 8.0f, triY, triX + 4.0f, triY + 5.0f);
        g.setColour(Colours::white50);
        g.fillPath(tri);

        g.setColour(Colours::white.withAlpha(0.06f));
        g.drawLine(stripRect.getX() - 7.0f,
                   bounds.getY() + 10.0f,
                   stripRect.getX() - 7.0f,
                   bounds.getBottom() - 10.0f,
                   1.0f);
    }

    // ── Nav chevrons (inside strip) ─────────────────────────────────
    auto drawChevron = [&g](juce::Rectangle<float> area, bool left, bool enabled)
    {
        const float cx = area.getCentreX();
        const float cy = area.getCentreY();
        const float sz = 5.0f;
        juce::Path path;
        if (left)
        {
            path.startNewSubPath(cx + sz * 0.5f, cy - sz);
            path.lineTo(cx - sz * 0.5f, cy);
            path.lineTo(cx + sz * 0.5f, cy + sz);
        }
        else
        {
            path.startNewSubPath(cx - sz * 0.5f, cy - sz);
            path.lineTo(cx + sz * 0.5f, cy);
            path.lineTo(cx - sz * 0.5f, cy + sz);
        }
        g.setColour(enabled ? Colours::white50 : Colours::white.withAlpha(0.25f));
        g.strokePath(path, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    };

    drawChevron(presetPrevButton.getBounds().toFloat(), true,  presetPrevButton.isEnabled());
    drawChevron(presetNextButton.getBounds().toFloat(), false, presetNextButton.isEnabled());

    // ── Undo/redo icons (compact 60px header, tighter inset) ─────────
    auto drawActionIcon = [&g](juce::Drawable* icon, juce::Rectangle<float> area, bool enabled)
    {
        if (icon == nullptr) return;
        const float inset = 7.0f;
        g.setOpacity(enabled ? 0.85f : 0.30f);
        icon->drawWithin(g, area.reduced(inset), juce::RectanglePlacement::centred, 1.0f);
        g.setOpacity(1.0f);
    };

    drawActionIcon(undoIcon.get(), undoButton.getBounds().toFloat(), undoButton.isEnabled());
    drawActionIcon(redoIcon.get(), redoButton.getBounds().toFloat(), redoButton.isEnabled());
}

void HeaderPanel::resized()
{
    namespace HL = HeaderLayout;
    const auto bounds  = getLocalBounds();
    const int  totalH  = bounds.getHeight();

    auto row = bounds.reduced(HL::kPadX, 6);
    const int cellH = totalH - 14;

    const int logoBlockW = HL::kLogoSize + HL::kLogoGap + HL::kWordmarkW;
    row.removeFromLeft(logoBlockW);
    row.removeFromLeft(HL::kSectionGap);

    // ── Tabs (left) ─────────────────────────────────────────────────
    auto tabArea = row.removeFromLeft(HL::kTabW * 3 + HL::kTabGap * 2);
    tabArea = tabArea.withHeight(cellH).withCentre(tabArea.getCentre());

    sequencerTab.setBounds(tabArea.removeFromLeft(HL::kTabW));
    tabArea.removeFromLeft(HL::kTabGap);
    presetsTab.setBounds(tabArea.removeFromLeft(HL::kTabW));
    tabArea.removeFromLeft(HL::kTabGap);
    settingsTab.setBounds(tabArea.removeFromLeft(HL::kTabW));

    // ── Undo/redo (far right) ───────────────────────────────────────
    auto actionArea = row.removeFromRight(HL::kActionW * 2 + HL::kActionGap);
    actionArea = actionArea.withHeight(cellH).withCentre(actionArea.getCentre());
    undoButton.setBounds(actionArea.removeFromLeft(HL::kActionW));
    actionArea.removeFromLeft(HL::kActionGap);
    redoButton.setBounds(actionArea.removeFromLeft(HL::kActionW));

    row.removeFromRight(HL::kSectionGap);

    // ── Preset strip with integrated nav (RIGHT-aligned) ────────────
    const int stripTotalW = HL::kNavW + HL::kPresetStripW + HL::kNavW;
    auto stripArea = row.removeFromRight(juce::jmin(row.getWidth(), stripTotalW));
    stripArea = stripArea.withHeight(cellH).withCentre(stripArea.getCentre());

    presetPrevButton.setBounds(stripArea.removeFromLeft(HL::kNavW));
    presetNextButton.setBounds(stripArea.removeFromRight(HL::kNavW));
    presetSelectButton.setBounds(stripArea);

    presetMetaLabel.setBounds(0, 0, 0, 0);
}

} // namespace zikada
