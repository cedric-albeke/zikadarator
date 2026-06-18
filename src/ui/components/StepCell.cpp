#include "ui/components/StepCell.h"
#include "ui/components/PresetIcons.h"

namespace zikada {

namespace {
    constexpr float kPlayingGlowAlpha = 0.26f;
    constexpr float kActiveGlowAlpha = 0.12f;
    constexpr float kActiveHoverGlowAlpha = 0.24f;
    constexpr float kInactiveHoverBorderAlpha = 0.38f;
}

StepCell::StepCell(int lane, int step)
    : Button(""), laneIndex(lane), stepIndex(step)
{
    setClickingTogglesState(true);
    onClick = [this]() { if (onSelected) onSelected(); };
}

void StepCell::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    juce::ignoreUnused(highlighted);

    const auto bounds       = getLocalBounds().toFloat().reduced(1.5f);
    const auto corner       = 4.0f;
    const bool isCellActive = active || getToggleState();
    const int  groupIndex   = stepIndex / 4;
    const bool isGroupStart = (stepIndex % 4 == 0);
    const auto laneColour   = laneInfos[laneIndex].colour;

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    // ── Playing state: outer glow ring ───────────────────────────────
    if (playing)
    {
        const auto glowBounds = bounds.expanded(4.0f);
        juce::ColourGradient playGlow(
            Colours::neonGreen.withAlpha(kPlayingGlowAlpha), glowBounds.getCentreX(), glowBounds.getCentreY(),
            Colours::neonGreen.withAlpha(0.0f),  glowBounds.getRight(),   glowBounds.getBottom(), true);
        g.setGradientFill(playGlow);
        g.fillRoundedRectangle(glowBounds, corner + 3.0f);

        g.setColour(Colours::neonGreen.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds, corner, 2.0f);
    }

    // ── Active state ───────────────────────────────────────────────
    if (isCellActive)
    {
        auto cellColour = laneColour;
        if (hovered) cellColour = cellColour.brighter(0.20f);
        if (down)    cellColour = cellColour.darker(0.20f);

        // Outer glow (subtle)
        const auto glowBounds = bounds.expanded(hovered ? 3.0f : 1.5f);
        juce::ColourGradient glowGrad(
            cellColour.withAlpha(hovered ? kActiveHoverGlowAlpha : kActiveGlowAlpha), glowBounds.getCentreX(), glowBounds.getCentreY(),
            cellColour.withAlpha(0.0f),  glowBounds.getRight(),   glowBounds.getBottom(), true);
        g.setGradientFill(glowGrad);
        g.fillRoundedRectangle(glowBounds, corner + 2.0f);

        // Body fill (gradient for depth)
        juce::ColourGradient fillGrad(
            cellColour.brighter(0.08f), bounds.getX(), bounds.getY(),
            cellColour.darker(0.35f),   bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(fillGrad);
        g.fillRoundedRectangle(bounds, corner);

        // Top highlight (simulates light)
        g.setColour(cellColour.brighter(0.50f).withAlpha(0.70f));
        g.fillRect(bounds.getX() + corner * 0.75f,
                   bounds.getY() + 0.5f,
                   bounds.getWidth() - corner * 1.5f,
                   1.5f);

        // Bottom shadow
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillRect(bounds.getX() + corner * 0.75f,
                   bounds.getBottom() - 1.5f,
                   bounds.getWidth() - corner * 1.5f,
                   1.0f);

        // Border (lane color, strong)
        g.setColour(cellColour.withAlpha(0.68f));
        g.drawRoundedRectangle(bounds, corner, 1.5f);

        if (hovered)
        {
            g.setColour(Colours::white.withAlpha(0.28f));
            g.drawRoundedRectangle(bounds, corner, 1.8f);
        }

        // Step number (bottom-right, 9px, white50)
        g.setColour(Colours::white.withAlpha(0.45f));
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(9.0f)
                                 : juce::Font(juce::FontOptions().withHeight(9.0f)));
        auto numBounds = juce::Rectangle<float>(
            bounds.getRight() - 18.0f, bounds.getBottom() - 14.0f, 16.0f, 12.0f);
        g.drawText(juce::String(stepIndex + 1), numBounds, juce::Justification::centredRight, false);

        // Preset icon (centered, when active)
        if (presetIndex > 0)
        {
            auto iconBounds = bounds.withTrimmedTop(bounds.getHeight() * 0.15f)
                                    .withTrimmedBottom(bounds.getHeight() * 0.25f)
                                    .reduced(bounds.getWidth() * 0.15f, 0);
            PresetIcons::drawPresetIcon(g, laneIndex, presetIndex, iconBounds, Colours::white.withAlpha(0.90f));
        }
    }
    // ── Inactive state ─────────────────────────────────────────────
    else
    {
        const bool isOddGroup = (groupIndex % 2 != 0);
        const juce::Colour baseBg = isOddGroup
            ? Colours::bgAccent.withAlpha(0.85f)
            : Colours::displayBezel.withAlpha(0.90f);

        juce::ColourGradient bgGrad(
            hovered ? baseBg.brighter(0.18f) : baseBg.brighter(0.05f), bounds.getX(), bounds.getY(),
            baseBg,                 bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(bgGrad);
        g.fillRoundedRectangle(bounds, corner);

        // Top highlight
        g.setColour(Colours::white.withAlpha(0.04f));
        g.fillRect(bounds.getX() + corner * 0.75f,
                   bounds.getY() + 0.5f,
                   bounds.getWidth() - corner * 1.5f,
                   1.0f);

        // Border (subtle, group start slightly stronger)
        const float borderAlpha = hovered ? kInactiveHoverBorderAlpha
                                : (isGroupStart ? 0.18f : 0.10f);
        g.setColour(Colours::white.withAlpha(borderAlpha));
        g.drawRoundedRectangle(bounds, corner, hovered ? 2.0f : 1.0f);

        if (hovered)
        {
            g.setColour(Colours::white.withAlpha(0.10f));
            g.fillRoundedRectangle(bounds, corner);
        }

        // Step number (centered, larger for inactive)
        const float numAlpha = hovered ? 0.85f : (isGroupStart ? 0.55f : 0.35f);
        g.setColour(Colours::white.withAlpha(numAlpha));
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(11.0f)
                                 : juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText(juce::String(stepIndex + 1), bounds, juce::Justification::centred, false);
    }

    // ── Tie animation (chain links) ───────────────────────────────
    if (tied)
    {
        const float tieY = bounds.getCentreY();
        const int   numLinks = juce::jmax(2, static_cast<int>(bounds.getWidth() / 10.0f));
        const float spacing  = bounds.getWidth() / numLinks;
        const float phase    = static_cast<float>(chainAnimPhase) * 0.125f;

        for (int i = 0; i < numLinks; ++i)
        {
            float t = phase + (i / static_cast<float>(numLinks));
            float alpha = 0.35f + 0.65f * std::abs(std::sin(t * juce::MathConstants<float>::pi));
            float linkSize = 5.0f + 2.0f * alpha;
            float cx = bounds.getX() + (i + 0.5f) * spacing;

            g.setColour(laneColour.withAlpha(alpha));
            g.fillEllipse(cx - linkSize * 0.5f, tieY - linkSize * 0.5f, linkSize, linkSize);

            g.setColour(laneColour.brighter(0.45f).withAlpha(alpha));
            g.drawEllipse(cx - linkSize * 0.5f + 1.5f, tieY - linkSize * 0.5f + 1.5f,
                          linkSize - 3.0f, linkSize - 3.0f, 1.5f);
        }

        g.setColour(laneColour.withAlpha(0.40f));
        g.fillRoundedRectangle(bounds.getX() - 2.0f, tieY - 1.0f,
                               bounds.getWidth() + 4.0f, 2.0f, 1.0f);
    }

    // ── Selected state ────────────────────────────────────────────
    if (selected)
    {
        g.setColour(Colours::neonGreen.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds, corner, 2.5f);
        g.setColour(Colours::neonGreen.withAlpha(0.70f));
        g.fillEllipse(bounds.getRight() - 6.0f, bounds.getY() + 1.5f, 4.5f, 4.5f);
    }

    // ── Chainable indicator (+ badge) ─────────────────────────────
    if (hovered && chainable)
    {
        const float plusSize = 16.0f;
        const float plusX = bounds.getX() + 3.0f;
        const float plusY = bounds.getY() + 3.0f;
        juce::Rectangle<float> plusBounds(plusX, plusY, plusSize, plusSize);

        g.setColour(Colours::neonGreen.withAlpha(0.30f));
        g.fillEllipse(plusBounds.expanded(2.0f));

        g.setColour(Colours::neonGreen);
        g.fillEllipse(plusBounds);

        g.setColour(Colours::white);
        const float stroke = 2.0f;
        const float cx = plusBounds.getCentreX();
        const float cy = plusBounds.getCentreY();
        const float half = plusSize * 0.22f;
        g.drawLine(cx - half, cy, cx + half, cy, stroke);
        g.drawLine(cx, cy - half, cx, cy + half, stroke);
    }
}

void StepCell::setActive(bool a)
{
    if (active == a)
        return;

    active = a;
    repaint();
}

void StepCell::setTied(bool t)
{
    if (tied == t)
        return;

    tied = t;
    repaint();
}

void StepCell::setChained(bool c)
{
    if (chained == c)
        return;

    chained = c;
    repaint();
}

void StepCell::setChainable(bool c)
{
    if (chainable == c)
        return;

    chainable = c;
    repaint();
}

void StepCell::setHovered(bool h)
{
    if (hovered == h)
        return;

    hovered = h;
    repaint();
}

void StepCell::setChainAnimPhase(int phase)
{
    chainAnimPhase = phase;
}

void StepCell::setPlaying(bool p)
{
    if (playing == p)
        return;

    playing = p;
    repaint();
}

void StepCell::setSelected(bool s)
{
    if (selected == s)
        return;

    selected = s;
    repaint();
}

void StepCell::setPresetIndex(int index)
{
    if (presetIndex == index)
        return;

    presetIndex = index;
    repaint();
}

}
