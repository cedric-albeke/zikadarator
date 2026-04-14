#include "ui/panels/HeaderPanel.h"
#include "BinaryData.h"

namespace zikada {

HeaderPanel::HeaderPanel()
{
    logoImage = juce::ImageCache::getFromMemory(BinaryData::zikadacicada128_png,
                                                BinaryData::zikadacicada128_pngSize);

    addAndMakeVisible(presetButton);
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);
    addAndMakeVisible(randomButton);

    presetButton.setColour(juce::TextButton::buttonColourId,  Colours::neonGreen);
    presetButton.setColour(juce::TextButton::buttonOnColourId, Colours::neonGreen);
    presetButton.setColour(juce::TextButton::textColourOnId,   Colours::bgPrimary);

    undoButton.setColour(juce::TextButton::buttonColourId,  Colours::white50);
    undoButton.setColour(juce::TextButton::buttonOnColourId, Colours::white85);
    undoButton.setColour(juce::TextButton::textColourOnId,   Colours::bgPrimary);

    redoButton.setColour(juce::TextButton::buttonColourId,  Colours::white50);
    redoButton.setColour(juce::TextButton::buttonOnColourId, Colours::white85);
    redoButton.setColour(juce::TextButton::textColourOnId,   Colours::bgPrimary);

    randomButton.setColour(juce::TextButton::buttonColourId,  Colours::laneLoop);
    randomButton.setColour(juce::TextButton::buttonOnColourId, Colours::laneLoop);
    randomButton.setColour(juce::TextButton::textColourOnId,   Colours::bgPrimary);
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

    auto content   = getLocalBounds().reduced(16, 8);
    auto brandArea = content.removeFromLeft(320);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    auto wordmarkFont = laf != nullptr ? laf->getAntaFont(26.0f)
                                       : juce::Font(juce::FontOptions().withHeight(26.0f));
    auto monoSmallFont = laf != nullptr ? laf->getSpaceMonoFont(10.0f, true)
                                        : juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold"));

    auto logoBounds = brandArea.removeFromLeft(52).reduced(2, 4);
    if (logoImage.isValid())
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageWithin(logoImage,
                          logoBounds.getX(), logoBounds.getY(),
                          logoBounds.getWidth(), logoBounds.getHeight(),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                          false);
    }

    auto textArea = brandArea.reduced(6, 0).toFloat();
    auto wordmarkRow = textArea.removeFromTop(30.0f);

    g.setFont(wordmarkFont);

    juce::GlyphArrangement gaZ, gaFx;
    gaZ.addLineOfText(wordmarkFont, "ZIKADA", 0.0f, 0.0f);
    gaFx.addLineOfText(wordmarkFont, " FX", 0.0f, 0.0f);
    float zikadaWidth = gaZ.getBoundingBox(0, gaZ.getNumGlyphs(), true).getWidth();
    float fxWidth     = gaFx.getBoundingBox(0, gaFx.getNumGlyphs(), true).getWidth();

    g.setColour(Colours::white);
    g.drawText("ZIKADA",
               juce::Rectangle<float>(wordmarkRow.getX(), wordmarkRow.getY(),
                                      zikadaWidth + 2.0f, wordmarkRow.getHeight()),
               juce::Justification::bottomLeft, false);

    g.setColour(Colours::neonGreen);
    g.drawText(" FX",
               juce::Rectangle<float>(wordmarkRow.getX() + zikadaWidth, wordmarkRow.getY(),
                                      fxWidth + 4.0f, wordmarkRow.getHeight()),
               juce::Justification::bottomLeft, false);

    g.setFont(laf != nullptr ? laf->getSpaceMonoFont(13.0f, true) : monoSmallFont);
    g.setColour(Colours::neonGreen.withAlpha(0.82f));
    g.drawText("MULTI-FX SEQUENCER  ·  LOOP PERFORMANCE ENGINE",
               textArea, juce::Justification::topLeft, false);

    if (presetButton.getWidth() > 0 && undoButton.getWidth() > 0)
    {
        float sepX   = static_cast<float>(presetButton.getRight()) +
                       (static_cast<float>(undoButton.getX() - presetButton.getRight())) * 0.5f;
        float sepTop = static_cast<float>(getHeight()) * 0.28f;
        float sepBot = static_cast<float>(getHeight()) * 0.72f;
        g.setColour(Colours::white.withAlpha(0.12f));
        g.drawLine(sepX, sepTop, sepX, sepBot, 1.0f);
    }

    auto statusBounds = getLocalBounds().removeFromRight(156).reduced(10, 14).toFloat();
    g.setColour(Colours::bgSurface.brighter(0.05f));
    g.fillRoundedRectangle(statusBounds, 8.0f);
    g.setColour(Colours::neonGreen.withAlpha(0.35f));
    g.drawRoundedRectangle(statusBounds, 8.0f, 1.1f);

    float dotX = statusBounds.getX() + 16.0f;
    float dotY = statusBounds.getCentreY();

    g.setColour(Colours::neonGreen.withAlpha(0.18f));
    g.fillEllipse(dotX - 7.5f, dotY - 7.5f, 15.0f, 15.0f);
    g.setColour(Colours::neonGreen);
    g.fillEllipse(dotX - 3.5f, dotY - 3.5f, 7.0f, 7.0f);

    g.setFont(monoSmallFont);
    g.setColour(Colours::white85);
    g.drawText("SYSTEM ONLINE",
               juce::Rectangle<int>(static_cast<int>(dotX + 10.0f),
                                    static_cast<int>(statusBounds.getY()),
                                    112,
                                    static_cast<int>(statusBounds.getHeight())),
               juce::Justification::centredLeft, false);
}

void HeaderPanel::resized()
{
    const int brandClearance  = 324;
    const int statusClearance = 172;

    auto bounds = getLocalBounds().reduced(16, 14);
    bounds.removeFromLeft(brandClearance);
    bounds.removeFromRight(statusClearance);

    const int buttonWidth  = 96;
    const int buttonHeight = 28;
    const int spacing      = 10;
    auto buttonArea = bounds.removeFromRight(buttonWidth * 4 + spacing * 3);
    buttonArea = buttonArea.withHeight(buttonHeight).withCentre(buttonArea.getCentre());

    randomButton.setBounds(buttonArea.removeFromRight(buttonWidth));
    buttonArea.removeFromRight(spacing);
    redoButton.setBounds(buttonArea.removeFromRight(buttonWidth));
    buttonArea.removeFromRight(spacing);
    undoButton.setBounds(buttonArea.removeFromRight(buttonWidth));
    buttonArea.removeFromRight(spacing);
    presetButton.setBounds(buttonArea.removeFromRight(buttonWidth));
}

}
