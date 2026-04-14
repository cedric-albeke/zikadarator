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
    vcrFont = juce::Font(juce::FontOptions(vcrFace).withHeight(12.0f));
    
    auto spaceMonoFace = juce::Typeface::createSystemTypefaceFor(
        BinaryData::SpaceMonoRegular_ttf, BinaryData::SpaceMonoRegular_ttfSize);
    spaceMonoFont = juce::Font(juce::FontOptions(spaceMonoFace).withHeight(14.0f));
    
    auto spaceMonoBoldFace = juce::Typeface::createSystemTypefaceFor(
        BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
    spaceMonoBoldFont = juce::Font(juce::FontOptions(spaceMonoBoldFace).withHeight(14.0f));
    
    auto antaFace = juce::Typeface::createSystemTypefaceFor(
        BinaryData::AntaRegular_ttf, BinaryData::AntaRegular_ttfSize);
    antaFont = juce::Font(juce::FontOptions(antaFace).withHeight(24.0f));
    
    interFont = juce::Font(juce::FontOptions().withName(juce::Font::getDefaultSansSerifFontName()).withHeight(16.0f));
}

void ZikadaLookAndFeel::initialiseColours()
{
    setColour(juce::ResizableWindow::backgroundColourId, Colours::bgPrimary);
    setColour(juce::TextButton::buttonColourId, Colours::neonGreen);
    setColour(juce::TextButton::buttonOnColourId, Colours::thirdGreen);
    setColour(juce::TextButton::textColourOffId, Colours::bgPrimary);
    setColour(juce::TextButton::textColourOnId, Colours::white);
    setColour(juce::Slider::thumbColourId, Colours::neonGreen);
    setColour(juce::Slider::trackColourId, Colours::neonGreen);
    setColour(juce::Slider::backgroundColourId, Colours::darkGrey);
    setColour(juce::Label::textColourId, Colours::white);
}

void ZikadaLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto cornerSize = 4.0f;
    
    juce::Colour baseColour = backgroundColour;
    
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.2f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.1f);
    
    if (button.getToggleState())
    {
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, cornerSize);
        g.setColour(Colours::white);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
    else if (button.isEnabled())
    {
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, cornerSize);
        
        if (shouldDrawButtonAsHighlighted)
        {
            g.setColour(Colours::neonGreen.withAlpha(0.5f));
            g.drawRoundedRectangle(bounds, cornerSize, 1.5f);
        }
        else
        {
            g.setColour(Colours::neonGreen.withAlpha(0.3f));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        }
    }
    else
    {
        g.setColour(Colours::bgSurface);
        g.fillRoundedRectangle(bounds, cornerSize);
        g.setColour(Colours::white50);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
}

void ZikadaLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    auto font = getSpaceMonoFont(11.0f, true);
    g.setFont(font);
    
    juce::Colour textColour;
    if (!button.isEnabled())
        textColour = Colours::white50;
    else if (button.getToggleState())
        textColour = Colours::bgPrimary;
    else
        textColour = Colours::neonGreen;
    
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
