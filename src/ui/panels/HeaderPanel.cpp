#include "ui/panels/HeaderPanel.h"
#include "BinaryData.h"

namespace zikada {

HeaderPanel::HeaderPanel()
{
    logoImage = juce::ImageCache::getFromMemory(BinaryData::zikadacicada_png,
                                                BinaryData::zikadacicada_pngSize);

    static constexpr auto undoSvg = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#D9E3E7" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
  <path d="M9 14 4 9l5-5" />
  <path d="M4 9h10.5a5.5 5.5 0 0 1 5.5 5.5a5.5 5.5 0 0 1-5.5 5.5H11" />
</svg>
)svg";

    static constexpr auto redoSvg = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#D9E3E7" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
  <path d="m15 14 5-5-5-5" />
  <path d="M20 9H9.5A5.5 5.5 0 0 0 4 14.5A5.5 5.5 0 0 0 9.5 20H13" />
</svg>
)svg";

    undoIcon = juce::Drawable::createFromImageData(undoSvg, sizeof(undoSvg) - 1);
    redoIcon = juce::Drawable::createFromImageData(redoSvg, sizeof(redoSvg) - 1);

    auto configureTab = [this](juce::TextButton& button, Page page)
    {
        button.setClickingTogglesState(true);
        button.setRadioGroupId(1001);
        button.setColour(juce::TextButton::buttonColourId, Colours::bgHover.brighter(0.02f));
        button.setColour(juce::TextButton::buttonOnColourId, Colours::bgHover.brighter(0.08f));
        button.setColour(juce::TextButton::textColourOffId, Colours::white85);
        button.setColour(juce::TextButton::textColourOnId, Colours::white);
        button.onClick = [this, page] {
            setSelectedPage(page);
            if (onPageSelected)
                onPageSelected(page);
        };
        addAndMakeVisible(button);
    };

    configureTab(sequencerTab, Page::Sequencer);
    configureTab(presetsTab, Page::Presets);
    configureTab(settingsTab, Page::Settings);

    auto configureAction = [](juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, Colours::bgHover.brighter(0.02f));
        button.setColour(juce::TextButton::buttonOnColourId, Colours::bgHover.brighter(0.08f));
        button.setColour(juce::TextButton::textColourOffId, Colours::white85);
        button.setColour(juce::TextButton::textColourOnId, Colours::white);
    };

    configureAction(undoButton);
    configureAction(redoButton);
    undoButton.setButtonText("");
    redoButton.setButtonText("");
    undoButton.onClick = [this] { if (onUndoRequested) onUndoRequested(); };
    redoButton.onClick = [this] { if (onRedoRequested) onRedoRequested(); };
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);

    presetSelectButton.setButtonText(currentPresetName + " ▼");
    presetSelectButton.setColour(juce::TextButton::buttonColourId, Colours::bgHover.brighter(0.02f));
    presetSelectButton.setColour(juce::TextButton::buttonOnColourId, Colours::bgHover.brighter(0.06f));
    presetSelectButton.setColour(juce::TextButton::textColourOffId, Colours::white);
    presetSelectButton.setColour(juce::TextButton::textColourOnId, Colours::white);
    presetSelectButton.onClick = [this]
    {
        if (onPresetBrowserRequested)
            onPresetBrowserRequested();
    };
    addAndMakeVisible(presetSelectButton);

    presetMetaLabel.setJustificationType(juce::Justification::centredLeft);
    presetMetaLabel.setColour(juce::Label::textColourId, Colours::laneFX2.withAlpha(0.88f));
    addAndMakeVisible(presetMetaLabel);

    configureAction(presetPrevButton);
    configureAction(presetNextButton);
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
}

void HeaderPanel::setUndoEnabled(bool enabled)
{
    undoButton.setEnabled(enabled);
    undoButton.setAlpha(enabled ? 1.0f : 0.82f);
}

void HeaderPanel::setRedoEnabled(bool enabled)
{
    redoButton.setEnabled(enabled);
    redoButton.setAlpha(enabled ? 1.0f : 0.82f);
}

void HeaderPanel::setPresetDisplay(const juce::String& presetName, const juce::String& presetMeta, bool dirty)
{
    currentPresetName = presetName;
    currentPresetMeta = presetMeta;
    presetDirty = dirty;
    presetSelectButton.setButtonText(currentPresetName + (presetDirty ? " * ▼" : " ▼"));
    presetMetaLabel.setText(currentPresetMeta, juce::dontSendNotification);
}

void HeaderPanel::setPresetStepEnabled(bool previousEnabled, bool nextEnabled)
{
    presetPrevButton.setEnabled(previousEnabled);
    presetPrevButton.setAlpha(previousEnabled ? 1.0f : 0.45f);
    presetNextButton.setEnabled(nextEnabled);
    presetNextButton.setAlpha(nextEnabled ? 1.0f : 0.45f);
}

void HeaderPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient headerGradient(Colours::bgHover.brighter(0.08f), 0.0f, 0.0f,
                                        Colours::bgAccent, 0.0f, bounds.getBottom(), false);
    g.setGradientFill(headerGradient);
    g.fillAll();

    g.setColour(Colours::white10);
    g.drawLine(0.0f, 0.5f, bounds.getRight(), 0.5f, 1.0f);
    g.setColour(Colours::neonGreen);
    g.drawLine(0.0f, bounds.getBottom() - 1.0f, bounds.getRight(), bounds.getBottom() - 1.0f, 2.0f);

    auto content   = getLocalBounds().reduced(12, 8);
    auto brandArea = content.removeFromLeft(278);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    auto wordmarkFont = laf != nullptr ? laf->getAntaFont(30.0f)
                                       : juce::Font(juce::FontOptions().withHeight(30.0f));
    auto monoSmallFont = laf != nullptr ? laf->getSpaceMonoFont(9.0f, true)
                                        : juce::Font(juce::FontOptions().withHeight(9.0f).withStyle("Bold"));
    auto monoMetaFont = laf != nullptr ? laf->getSpaceMonoFont(8.0f, true)
                                       : juce::Font(juce::FontOptions().withHeight(8.0f).withStyle("Bold"));

    g.setColour(Colours::bgHover.brighter(0.03f));
    g.fillRoundedRectangle(brandArea.toFloat(), 2.0f);
    g.setColour(Colours::white10);
    g.drawRoundedRectangle(brandArea.toFloat().reduced(0.5f), 2.0f, 1.0f);

    auto brandInner = brandArea.reduced(12, 8);
    auto logoArea = brandInner.removeFromLeft(66);
    auto logoFloat = logoArea.toFloat();
    g.setColour(Colours::neonGreen.withAlpha(0.14f));
    g.fillRoundedRectangle(logoFloat.reduced(1.0f), 10.0f);
    g.setColour(Colours::bgHover.brighter(0.10f));
    g.fillRoundedRectangle(logoFloat.reduced(4.0f), 10.0f);
    g.setColour(Colours::neonGreen.withAlpha(0.34f));
    g.drawRoundedRectangle(logoFloat.reduced(4.5f), 10.0f, 1.0f);
    auto logoBounds = logoArea.reduced(5, 5);
    if (logoImage.isValid())
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageWithin(logoImage,
                          logoBounds.getX(), logoBounds.getY(),
                          logoBounds.getWidth(), logoBounds.getHeight(),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                          false);
    }

    auto textArea = brandInner.reduced(10, 2).toFloat();
    auto wordmarkRow = textArea.removeFromTop(30.0f);
    auto sublineRow = textArea.removeFromTop(12.0f);

    g.setFont(wordmarkFont);

    juce::GlyphArrangement gaLeft, gaRight;
    gaLeft.addLineOfText(wordmarkFont, "ZIKADA", 0.0f, 0.0f);
    gaRight.addLineOfText(wordmarkFont, "RATOR", 0.0f, 0.0f);
    float leftWidth = gaLeft.getBoundingBox(0, gaLeft.getNumGlyphs(), true).getWidth();
    float rightWidth = gaRight.getBoundingBox(0, gaRight.getNumGlyphs(), true).getWidth();

    g.setColour(Colours::white);
    g.drawText("ZIKADA",
                juce::Rectangle<float>(wordmarkRow.getX(), wordmarkRow.getY(),
                                       leftWidth + 2.0f, wordmarkRow.getHeight()),
                juce::Justification::bottomLeft, false);

    g.setColour(Colours::neonGreen);
    g.drawText("RATOR",
               juce::Rectangle<float>(wordmarkRow.getX() + leftWidth - 1.0f, wordmarkRow.getY(),
                                      rightWidth + 4.0f, wordmarkRow.getHeight()),
               juce::Justification::bottomLeft, false);

    g.setFont(monoSmallFont);
    g.setColour(Colours::laneFX2.withAlpha(0.70f));
    g.drawText("SEQUENCE THE SIGNAL / V1",
               juce::Rectangle<float>(sublineRow.getX(), sublineRow.getY(), 240.0f, sublineRow.getHeight()),
               juce::Justification::topLeft, false);

    if (presetSelectButton.getWidth() > 0)
    {
        auto presetFrame = presetMetaLabel.getBounds().getUnion(presetSelectButton.getBounds()).toFloat();
        g.setColour(Colours::bgHover.brighter(0.02f));
        g.fillRoundedRectangle(presetFrame, 2.0f);
        g.setColour(Colours::white10);
        g.drawRoundedRectangle(presetFrame.reduced(0.5f), 2.0f, 1.0f);

        g.setColour(Colours::white10);
        const float separatorY = static_cast<float>(presetSelectButton.getY()) - 2.0f;
        g.drawLine(presetFrame.getX() + 1.0f, separatorY, presetFrame.getRight() - 1.0f, separatorY, 1.0f);

        const auto navGroup = presetPrevButton.getBounds().getUnion(presetNextButton.getBounds()).toFloat();
        g.setColour(Colours::bgHover.brighter(0.02f));
        g.fillRoundedRectangle(navGroup, 2.0f);
        g.setColour(Colours::white10);
        g.drawRoundedRectangle(navGroup.reduced(0.5f), 2.0f, 1.0f);
        const float navSepX = static_cast<float>(presetNextButton.getX()) - 3.0f;
        g.drawLine(navSepX, navGroup.getY() + 5.0f, navSepX, navGroup.getBottom() - 5.0f, 1.0f);

        const auto drawChevron = [&g](juce::Rectangle<float> area, bool left, bool enabled)
        {
            juce::Path path;
            const float cx = area.getCentreX();
            const float cy = area.getCentreY();
            const float size = 5.0f;
            if (left)
            {
                path.startNewSubPath(cx + size * 0.5f, cy - size);
                path.lineTo(cx - size * 0.5f, cy);
                path.lineTo(cx + size * 0.5f, cy + size);
            }
            else
            {
                path.startNewSubPath(cx - size * 0.5f, cy - size);
                path.lineTo(cx + size * 0.5f, cy);
                path.lineTo(cx - size * 0.5f, cy + size);
            }

            g.setColour((enabled ? Colours::white85 : Colours::white50).withAlpha(enabled ? 0.95f : 0.45f));
            g.strokePath(path, juce::PathStrokeType(1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };

        drawChevron(presetPrevButton.getBounds().toFloat(), true, presetPrevButton.isEnabled());
        drawChevron(presetNextButton.getBounds().toFloat(), false, presetNextButton.isEnabled());

        const auto drawDrawableIcon = [&g](juce::Drawable* drawable, juce::Rectangle<float> area, bool enabled)
        {
            if (drawable == nullptr)
                return;

            g.saveState();
            g.setOpacity(enabled ? 0.96f : 0.62f);
            drawable->drawWithin(g, area.reduced(12.0f, 12.0f), juce::RectanglePlacement::centred, 1.0f);
            g.restoreState();
        };

        drawDrawableIcon(undoIcon.get(), undoButton.getBounds().toFloat(), undoButton.isEnabled());
        drawDrawableIcon(redoIcon.get(), redoButton.getBounds().toFloat(), redoButton.isEnabled());

        g.setFont(monoMetaFont);
        g.setColour(selectedPage == Page::Sequencer ? Colours::bgPrimary.withAlpha(0.90f) : Colours::white85.withAlpha(0.78f));
        g.drawText(sequencerTab.getButtonText(), sequencerTab.getBounds(), juce::Justification::centred, false);
        g.drawText(presetsTab.getButtonText(), presetsTab.getBounds(), juce::Justification::centred, false);
        g.drawText(settingsTab.getButtonText(), settingsTab.getBounds(), juce::Justification::centred, false);
    }

    if (redoButton.getWidth() > 0 && sequencerTab.getWidth() > 0)
    {
        float sepX   = static_cast<float>(redoButton.getRight()) +
                       (static_cast<float>(sequencerTab.getX() - redoButton.getRight())) * 0.5f;
        float sepTop = static_cast<float>(getHeight()) * 0.28f;
        float sepBot = static_cast<float>(getHeight()) * 0.72f;
        g.setColour(Colours::white.withAlpha(0.12f));
        g.drawLine(sepX, sepTop, sepX, sepBot, 1.0f);
    }
}

void HeaderPanel::resized()
{
    const int brandClearance  = 544;

    auto bounds = getLocalBounds().reduced(12, 10);
    bounds.removeFromLeft(brandClearance);

    const int tabWidth = 94;
    const int tabHeight = 50;
    const int actionWidth = 52;
    const int navWidth = 34;
    const int spacing = 6;

    auto tabArea = bounds.removeFromLeft(tabWidth * 3 + spacing * 2);
    tabArea = tabArea.withHeight(tabHeight).withCentre(tabArea.getCentre());
    sequencerTab.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(spacing);
    presetsTab.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(spacing);
    settingsTab.setBounds(tabArea.removeFromLeft(tabWidth));

    bounds.removeFromLeft(12);

    auto actionsArea = bounds.removeFromRight(actionWidth * 2 + spacing);
    actionsArea = actionsArea.withHeight(tabHeight).withCentre(actionsArea.getCentre());
    undoButton.setBounds(actionsArea.removeFromLeft(actionWidth));
    actionsArea.removeFromLeft(spacing);
    redoButton.setBounds(actionsArea.removeFromLeft(actionWidth));

    bounds.removeFromRight(10);

    auto presetNavArea = bounds.removeFromRight(navWidth * 2 + spacing);
    presetNavArea = presetNavArea.withHeight(tabHeight).withCentre(presetNavArea.getCentre());
    presetPrevButton.setBounds(presetNavArea.removeFromLeft(navWidth));
    presetNavArea.removeFromLeft(spacing);
    presetNextButton.setBounds(presetNavArea.removeFromLeft(navWidth));

    bounds.removeFromRight(4);

    auto presetArea = bounds.removeFromRight(388);
    presetArea = presetArea.withHeight(tabHeight).withCentre(presetArea.getCentre());
    auto presetTop = presetArea.removeFromTop(14);
    presetMetaLabel.setBounds(presetTop.reduced(10, 0));
    presetArea.removeFromTop(4);
    presetSelectButton.setBounds(presetArea.reduced(6, 0));
}

}
