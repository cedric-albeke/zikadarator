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
    vcrFont = juce::Font(juce::Typeface::createSystemTypefaceFor(
        BinaryData::VCROSDMONO_ttf, BinaryData::VCROSDMONO_ttfSize));
    
    spaceMonoFont = juce::Font(juce::Typeface::createSystemTypefaceFor(
        BinaryData::SpaceMonoRegular_ttf, BinaryData::SpaceMonoRegular_ttfSize));
    
    spaceMonoBoldFont = juce::Font(juce::Typeface::createSystemTypefaceFor(
        BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize));
    
    antaFont = juce::Font(juce::Typeface::createSystemTypefaceFor(
        BinaryData::AntaRegular_ttf, BinaryData::AntaRegular_ttfSize));
    
    interFont = juce::Font(juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::plain);
}

void ZikadaLookAndFeel::initialiseColours()
{
    setColour(juce::ResizableWindow::backgroundColourId, Colours::bgPrimary);
    setColour(juce::TextButton::buttonColourId, Colours::neonGreen);
    setColour(juce::TextButton::buttonOnColourId, Colours::thirdGreen);
    setColour(juce::TextButton::textColourOffId, Colours::bgPrimary);
    setColour(juce::TextButton::textColourOnId, Colours::white);
    setColour(juce::Slider::thumbColourId, Colours::neonGreen);
    setColour(juce::Slider::trackColourId, Colours::darkGrey);
    setColour(juce::Slider::backgroundColourId, Colours::bgSurface);
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
