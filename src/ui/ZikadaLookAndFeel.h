#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zikada {

namespace Colours {
    inline const juce::Colour neonGreen      = juce::Colour(0x00FF85);
    inline const juce::Colour secondaryGreen = juce::Colour(0x00DE74);
    inline const juce::Colour thirdGreen     = juce::Colour(0x00AF5B);
    inline const juce::Colour darkGreen      = juce::Colour(0x00B75B);
    inline const juce::Colour bgPrimary      = juce::Colour(0x000C0D);
    inline const juce::Colour bgAccent       = juce::Colour(0x011C1A);
    inline const juce::Colour bgSurface      = juce::Colour(0x0D1F1E);
    inline const juce::Colour bgHover        = juce::Colour(0x071212);
    inline const juce::Colour white          = juce::Colour(0xFFFFFF);
    inline const juce::Colour white85        = juce::Colour(0xFFFFFFD9);
    inline const juce::Colour white50        = juce::Colour(0xFFFFFF80);
    inline const juce::Colour white10        = juce::Colour(0xFFFFFF1A);
    inline const juce::Colour darkGrey       = juce::Colour(0x333333);
    inline const juce::Colour laneInput      = juce::Colour(0xFFFFFF);
    inline const juce::Colour laneSlice      = juce::Colour(0x00DEFF);
    inline const juce::Colour laneLoop       = juce::Colour(0xC040C0);
    inline const juce::Colour laneEnvelope   = juce::Colour(0x8A40FF);
    inline const juce::Colour laneFX1        = juce::Colour(0x00FF85);
    inline const juce::Colour laneFilter     = juce::Colour(0xBFFF00);
    inline const juce::Colour laneFX2        = juce::Colour(0x00FFB3);
    inline const juce::Colour success        = juce::Colour(0x00FF85);
    inline const juce::Colour warning        = juce::Colour(0xFFB800);
    inline const juce::Colour error          = juce::Colour(0xFF4444);
    inline const juce::Colour waveform       = juce::Colour(0xC040C0);
}

// Lane metadata
struct LaneInfo { const char* name; juce::Colour colour; };
inline const LaneInfo laneInfos[6] = {
    {"SLICE", Colours::laneSlice}, {"LOOP", Colours::laneLoop},
    {"ENVELOPE", Colours::laneEnvelope}, {"FX1", Colours::laneFX1},
    {"FILTER", Colours::laneFilter}, {"FX2", Colours::laneFX2}
};

class ZikadaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ZikadaLookAndFeel();
    
    // Font accessors
    juce::Font getVcrFont(float size = 12.0f) const;
    juce::Font getSpaceMonoFont(float size = 14.0f, bool bold = false) const;
    juce::Font getAntaFont(float size = 24.0f) const;
    juce::Font getInterFont(float size = 16.0f) const;
    
    // Override default colors
    void initialiseColours();
    
private:
    juce::Font vcrFont;
    juce::Font spaceMonoFont;
    juce::Font spaceMonoBoldFont;
    juce::Font antaFont;
    juce::Font interFont;
    
    void loadFonts();
};

} // namespace zikada
