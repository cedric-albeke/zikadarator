#include "ui/components/StepCell.h"
#include "ui/components/PresetIcons.h"

namespace zikada {

StepCell::StepCell(int lane, int step)
    : Button(""), laneIndex(lane), stepIndex(step)
{
    setClickingTogglesState(true);
    onClick = [this]() { if (onSelected) onSelected(); };
}

void StepCell::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds       = getLocalBounds().toFloat().reduced(1.5f);
    const auto corner       = 4.0f;
    const bool isCellActive = active || getToggleState();
    const int  groupIndex   = stepIndex / 4;
    const bool isGroupStart = (stepIndex % 4 == 0);

    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());

    if (isCellActive)
    {
        auto laneColour = laneInfos[laneIndex].colour;
        if (highlighted) laneColour = laneColour.brighter(0.18f);
        if (down)        laneColour = laneColour.darker(0.25f);

        const auto glowBounds = bounds.expanded(3.0f);
        juce::ColourGradient glowGrad(
            laneColour.withAlpha(0.28f), glowBounds.getCentreX(), glowBounds.getCentreY(),
            laneColour.withAlpha(0.0f),  glowBounds.getRight(),   glowBounds.getBottom(), true);
        g.setGradientFill(glowGrad);
        g.fillRoundedRectangle(glowBounds, corner + 2.0f);

        juce::ColourGradient fillGrad(
            laneColour.brighter(0.12f), bounds.getX(), bounds.getY(),
            laneColour.darker(0.42f),   bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(fillGrad);
        g.fillRoundedRectangle(bounds, corner);

        g.setColour(juce::Colour::fromRGBA(0, 0, 0, 32));
        g.fillRoundedRectangle(bounds.withTop(bounds.getCentreY()), corner * 0.5f);

        g.setColour(laneColour.brighter(0.65f).withAlpha(0.88f));
        g.fillRect(bounds.getX() + corner * 0.75f,
                   bounds.getY() + 0.5f,
                   bounds.getWidth() - corner * 1.5f,
                   1.5f);

        g.setColour(laneColour.withAlpha(0.82f));
        g.drawRoundedRectangle(bounds, corner, 1.5f);

        g.setColour(Colours::bgPrimary.withAlpha(0.90f));
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(12.0f, true)
                                 : juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
        auto numBounds = bounds.withHeight(bounds.getHeight() * 0.35f);
        g.drawText(juce::String(stepIndex + 1), numBounds, juce::Justification::centred, false);

        if (presetIndex > 0)
        {
            auto iconBounds = bounds.withTrimmedTop(bounds.getHeight() * 0.30f)
                                    .withTrimmedBottom(bounds.getHeight() * 0.10f)
                                    .reduced(bounds.getWidth() * 0.10f, 0);
            PresetIcons::drawPresetIcon(g, laneIndex, presetIndex, iconBounds, Colours::white.withAlpha(0.95f));
        }
    }
    else
    {
        const bool isOddGroup = (groupIndex % 2 != 0);
        const juce::Colour baseBg = isOddGroup
            ? Colours::bgAccent.withAlpha(0.88f)
            : Colours::displayBezel.withAlpha(0.95f);

        juce::ColourGradient bgGrad(
            baseBg.brighter(0.07f), bounds.getX(), bounds.getY(),
            baseBg,                 bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(bgGrad);
        g.fillRoundedRectangle(bounds, corner);

        g.setColour(Colours::white.withAlpha(0.04f));
        g.fillRect(bounds.getX() + corner * 0.75f,
                   bounds.getY() + 0.5f,
                   bounds.getWidth() - corner * 1.5f,
                   1.0f);

        const float borderAlpha = highlighted ? 0.38f
                                : (isGroupStart ? 0.22f : 0.12f);
        g.setColour(Colours::white.withAlpha(borderAlpha));
        g.drawRoundedRectangle(bounds, corner, 1.0f);

        const float numAlpha = isGroupStart ? 0.68f : 0.48f;
        g.setColour(Colours::white.withAlpha(numAlpha));
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(12.0f)
                                 : juce::Font(juce::FontOptions().withHeight(12.0f)));
        g.drawText(juce::String(stepIndex + 1), bounds, juce::Justification::centred, false);
    }

    if (playing)
    {
        g.setColour(Colours::white.withAlpha(0.18f));
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(Colours::white.withAlpha(0.75f));
        g.drawRoundedRectangle(bounds, corner, 2.0f);
    }

    if (tied)
    {
        const auto laneColour = laneInfos[laneIndex].colour;
        g.setColour(laneColour.withAlpha(0.85f));
        const float tieY = bounds.getCentreY();
        const float tieH = 4.0f;
        g.fillRoundedRectangle(bounds.getX() - 3.0f, tieY - tieH * 0.5f,
                               bounds.getWidth() + 6.0f, tieH, 2.0f);
        g.setColour(laneColour.brighter(0.4f).withAlpha(0.60f));
        g.fillRoundedRectangle(bounds.getX() - 3.0f, tieY - tieH * 0.5f,
                               bounds.getWidth() + 6.0f, tieH * 0.4f, 1.5f);
    }

    if (selected)
    {
        g.setColour(Colours::neonGreen);
        g.drawRoundedRectangle(bounds, corner, 2.5f);
        g.setColour(Colours::neonGreen.withAlpha(0.85f));
        g.fillEllipse(bounds.getRight() - 6.0f, bounds.getY() + 1.5f, 4.5f, 4.5f);
    }
}

void StepCell::setActive(bool a)
{
    active = a;
    repaint();
}

void StepCell::setTied(bool t)
{
    tied = t;
    repaint();
}

void StepCell::setPlaying(bool p)
{
    playing = p;
    repaint();
}

void StepCell::setSelected(bool s)
{
    selected = s;
    repaint();
}

void StepCell::setPresetIndex(int index)
{
    presetIndex = index;
    repaint();
}

}
