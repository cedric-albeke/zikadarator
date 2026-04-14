#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zikada {

namespace Colours {
    inline const juce::Colour neonGreen      = juce::Colour::fromRGB (0x00, 0xFF, 0x85);
    inline const juce::Colour secondaryGreen = juce::Colour::fromRGB (0x00, 0xDE, 0x74);
    inline const juce::Colour thirdGreen     = juce::Colour::fromRGB (0x00, 0xAF, 0x5B);
    inline const juce::Colour darkGreen      = juce::Colour::fromRGB (0x00, 0xB7, 0x5B);
    inline const juce::Colour bgPrimary      = juce::Colour::fromRGB (0x00, 0x0C, 0x0D);
    inline const juce::Colour bgAccent       = juce::Colour::fromRGB (0x01, 0x1C, 0x1A);
    inline const juce::Colour bgSurface      = juce::Colour::fromRGB (0x0D, 0x1F, 0x1E);
    inline const juce::Colour bgHover        = juce::Colour::fromRGBA(0x07, 0x12, 0x12, 0xCC);
    inline const juce::Colour white          = juce::Colour::fromRGB (0xFF, 0xFF, 0xFF);
    inline const juce::Colour white85        = juce::Colour::fromRGBA(0xFF, 0xFF, 0xFF, 0xD9);
    inline const juce::Colour white50        = juce::Colour::fromRGBA(0xFF, 0xFF, 0xFF, 0x80);
    inline const juce::Colour white10        = juce::Colour::fromRGBA(0xFF, 0xFF, 0xFF, 0x1A);
    inline const juce::Colour darkGrey       = juce::Colour::fromRGB (0x33, 0x33, 0x33);
    inline const juce::Colour laneInput      = juce::Colour::fromRGB (0xFF, 0xFF, 0xFF);
    inline const juce::Colour laneSlice      = juce::Colour::fromRGB (0x00, 0xDE, 0xFF);
    inline const juce::Colour laneLoop       = juce::Colour::fromRGB (0xC0, 0x40, 0xC0);
    inline const juce::Colour laneEnvelope   = juce::Colour::fromRGB (0x8A, 0x40, 0xFF);
    inline const juce::Colour laneFX1        = juce::Colour::fromRGB (0x00, 0xFF, 0x85);
    inline const juce::Colour laneFilter     = juce::Colour::fromRGB (0xBF, 0xFF, 0x00);
    inline const juce::Colour laneFX2        = juce::Colour::fromRGB (0x00, 0xFF, 0xB3);
    inline const juce::Colour success        = juce::Colour::fromRGB (0x00, 0xFF, 0x85);
    inline const juce::Colour warning        = juce::Colour::fromRGB (0xFF, 0xB8, 0x00);
    inline const juce::Colour error          = juce::Colour::fromRGB (0xFF, 0x44, 0x44);
    inline const juce::Colour waveform       = juce::Colour::fromRGB (0xC0, 0x40, 0xC0);

    // ── Premium panel framing tokens ────────────────────────────────
    // Outermost chassis / shell background — deeper than bgPrimary
    inline const juce::Colour shellBg        = juce::Colour::fromRGB (0x00, 0x07, 0x08);
    // Lifted module surface — slightly brighter than bgAccent
    inline const juce::Colour panelRaised    = juce::Colour::fromRGB (0x02, 0x22, 0x20);
    // Device display bezel — near-black teal for inset displays
    inline const juce::Colour displayBezel   = juce::Colour::fromRGB (0x00, 0x10, 0x0F);
}

namespace PanelMetrics {
    inline constexpr float kCorner      = 8.0f;
    inline constexpr float kInnerCorner = 5.0f;
    inline constexpr int   kShellInset  = 8;
    inline constexpr int   kModuleGap   = 6;
    inline constexpr int   kPadding     = 14;
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
    
    juce::Font getVcrFont(float size = 12.0f) const;
    juce::Font getSpaceMonoFont(float size = 14.0f, bool bold = false) const;
    juce::Font getAntaFont(float size = 24.0f) const;
    juce::Font getInterFont(float size = 16.0f) const;
    
    void initialiseColours();

    static void drawPremiumPanel    (juce::Graphics& g, juce::Rectangle<int> bounds, bool accentTopEdge = false);
    static void drawDeviceDisplay   (juce::Graphics& g, juce::Rectangle<int> bounds);
    static void drawModuleSeparator (juce::Graphics& g, int x, int y, int length, bool horizontal = true);
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
        const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style,
        juce::Slider& slider) override;
    
private:
    juce::Font vcrFont;
    juce::Font spaceMonoFont;
    juce::Font spaceMonoBoldFont;
    juce::Font antaFont;
    juce::Font interFont;
    
    void loadFonts();
};

} // namespace zikada
