#include "ui/ZikadaLookAndFeel.h"
#include "BinaryData.h"

namespace zikada {

ZikadaLookAndFeel::ZikadaLookAndFeel()
{
    loadFonts();
    initialiseColours();
}

void ZikadaLookAndFeel::loadFonts()
{
    auto vcrFace = juce::Typeface::createSystemTypefaceFor(
        BinaryData::VCROSDMONO_ttf, BinaryData::VCROSDMONO_ttfSize);
    vcrFont = juce::Font(juce::FontOptions(vcrFace).withHeight(14.0f));

    auto spaceMonoFace = juce::Typeface::createSystemTypefaceFor(
        BinaryData::SpaceMonoRegular_ttf, BinaryData::SpaceMonoRegular_ttfSize);
    spaceMonoFont = juce::Font(juce::FontOptions(spaceMonoFace).withHeight(16.0f));

    auto spaceMonoBoldFace = juce::Typeface::createSystemTypefaceFor(
        BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
    spaceMonoBoldFont = juce::Font(juce::FontOptions(spaceMonoBoldFace).withHeight(16.0f));

    auto antaFace = juce::Typeface::createSystemTypefaceFor(
        BinaryData::AntaRegular_ttf, BinaryData::AntaRegular_ttfSize);
    antaFont = juce::Font(juce::FontOptions(antaFace).withHeight(28.0f));

    interFont = juce::Font(juce::FontOptions().withName(juce::Font::getDefaultSansSerifFontName()).withHeight(18.0f));
}

void ZikadaLookAndFeel::initialiseColours()
{
    setColour(juce::ResizableWindow::backgroundColourId, Colours::bgPrimary);
    setColour(juce::TextButton::buttonColourId, Colours::neonGreen);
    setColour(juce::TextButton::buttonOnColourId, Colours::neonGreen);
    setColour(juce::TextButton::textColourOffId, Colours::white85);
    setColour(juce::TextButton::textColourOnId, Colours::white);
    setColour(juce::Slider::thumbColourId, Colours::neonGreen);
    setColour(juce::Slider::trackColourId, Colours::neonGreen);
    setColour(juce::Slider::backgroundColourId, Colours::darkGrey);
    setColour(juce::Label::textColourId, Colours::white);
}

void ZikadaLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds         = button.getLocalBounds().toFloat().reduced(1.0f);
    const auto corner   = 4.0f;
    const bool isActive = button.getToggleState() || shouldDrawButtonAsDown;

    if (isActive)
    {
        auto fill = button.findColour(juce::TextButton::buttonOnColourId);
        g.setColour(shouldDrawButtonAsDown ? fill.darker(0.08f) : fill);
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(fill.brighter(0.35f).withAlpha(0.90f));
        g.drawRoundedRectangle(bounds, corner, 1.4f);
    }
    else if (button.isEnabled())
    {
        if (shouldDrawButtonAsHighlighted)
        {
            g.setColour(backgroundColour.withAlpha(0.09f));
            g.fillRoundedRectangle(bounds, corner);
            g.setColour(backgroundColour.withAlpha(0.72f));
            g.drawRoundedRectangle(bounds, corner, 1.3f);
        }
        else
        {
            g.setColour(Colours::bgSurface.withAlpha(0.50f));
            g.fillRoundedRectangle(bounds, corner);
            g.setColour(Colours::white.withAlpha(0.20f));
            g.drawRoundedRectangle(bounds, corner, 1.0f);
        }
    }
    else
    {
        g.setColour(Colours::bgSurface.withAlpha(0.25f));
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(bounds, corner, 1.0f);
    }
}

void ZikadaLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    g.setFont(getSpaceMonoFont(13.0f, true));

    juce::Colour textColour;
    if (!button.isEnabled())
        textColour = Colours::white.withAlpha(0.28f);
    else if (button.getToggleState() || shouldDrawButtonAsDown)
        textColour = button.findColour(juce::TextButton::textColourOnId);
    else if (shouldDrawButtonAsHighlighted)
        textColour = Colours::white.withAlpha(0.95f);
    else
        textColour = Colours::white.withAlpha(0.72f);

    g.setColour(textColour);
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

void ZikadaLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos);
    
    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
    {
        auto trackBounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y + height / 2 - 3), static_cast<float>(width), 6.0f);
        
        g.setColour(findColour(juce::Slider::backgroundColourId));
        g.fillRoundedRectangle(trackBounds, 3.0f);
        
        auto fillBounds = trackBounds.withRight(sliderPos);
        g.setColour(findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle(fillBounds, 3.0f);
        
        auto thumbBounds = juce::Rectangle<float>(sliderPos - 5.0f, static_cast<float>(y + height / 2 - 6), 10.0f, 12.0f);
        g.setColour(findColour(juce::Slider::thumbColourId));
        g.fillRoundedRectangle(thumbBounds, 3.0f);
        g.setColour(Colours::white);
        g.drawRoundedRectangle(thumbBounds, 3.0f, 1.0f);
    }
}

void ZikadaLookAndFeel::drawPremiumPanel(juce::Graphics& g, juce::Rectangle<int> bounds, bool accentTopEdge)
{
    const float corner = PanelMetrics::kCorner;
    auto bf = bounds.toFloat();

    g.setColour(Colours::bgAccent);
    g.fillRoundedRectangle(bf, corner);

    juce::ColourGradient depthGrad(
        Colours::panelRaised.withAlpha(0.32f), bf.getX(), bf.getY(),
        Colours::bgPrimary.withAlpha(0.12f), bf.getX(), bf.getBottom(), false);
    g.setGradientFill(depthGrad);
    g.fillRoundedRectangle(bf.reduced(1.0f), corner - 0.5f);

    g.setColour(Colours::white.withAlpha(0.10f));
    g.drawRoundedRectangle(bf.reduced(0.5f), corner, 1.0f);

    if (accentTopEdge)
    {
        const float inset = corner * 0.7f;
        g.setColour(Colours::neonGreen.withAlpha(0.55f));
        g.drawLine(bf.getX() + inset, bf.getY() + 0.75f,
                   bf.getRight() - inset, bf.getY() + 0.75f, 1.5f);
    }
}

void ZikadaLookAndFeel::drawDeviceDisplay(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const float corner = PanelMetrics::kInnerCorner;
    auto bf = bounds.toFloat();

    g.setColour(Colours::displayBezel);
    g.fillRoundedRectangle(bf, corner);

    auto innerBf = bf.reduced(3.0f, 3.0f);
    g.setColour(Colours::bgSurface.withAlpha(0.88f));
    g.fillRoundedRectangle(innerBf, corner - 1.0f);

    g.setColour(Colours::white.withAlpha(0.06f));
    g.drawLine(innerBf.getX() + (corner - 1.0f), innerBf.getY() + 0.5f,
               innerBf.getRight() - (corner - 1.0f), innerBf.getY() + 0.5f, 1.0f);

    g.setColour(Colours::neonGreen.withAlpha(0.20f));
    g.drawRoundedRectangle(bf.reduced(0.5f), corner, 1.0f);
}

void ZikadaLookAndFeel::drawModuleSeparator(juce::Graphics& g, int x, int y, int length, bool horizontal)
{
    if (horizontal)
    {
        g.setColour(Colours::neonGreen.withAlpha(0.22f));
        g.drawLine(static_cast<float>(x), static_cast<float>(y),
                   static_cast<float>(x + length), static_cast<float>(y), 1.0f);
        g.setColour(Colours::shellBg.withAlpha(0.80f));
        g.drawLine(static_cast<float>(x), static_cast<float>(y + 1),
                   static_cast<float>(x + length), static_cast<float>(y + 1), 1.0f);
    }
    else
    {
        g.setColour(Colours::white.withAlpha(0.10f));
        g.drawLine(static_cast<float>(x), static_cast<float>(y),
                   static_cast<float>(x), static_cast<float>(y + length), 1.0f);
    }
}

juce::Font ZikadaLookAndFeel::getVcrFont(float size) const
{
    return vcrFont.withHeight(size);
}

juce::Font ZikadaLookAndFeel::getSpaceMonoFont(float size, bool bold) const
{
    return (bold ? spaceMonoBoldFont : spaceMonoFont).withHeight(size);
}

juce::Font ZikadaLookAndFeel::getAntaFont(float size) const
{
    return antaFont.withHeight(size);
}

juce::Font ZikadaLookAndFeel::getInterFont(float size) const
{
    return interFont.withHeight(size);
}

}
