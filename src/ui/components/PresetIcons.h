#pragma once

#include "../ZikadaLookAndFeel.h"

namespace zikada {

struct PresetIcons
{
    // ── Lane category icons ───────────────────────────────────────────
    static void drawSliceIcon(juce::Graphics& g, juce::Rectangle<float> bounds,
                              juce::Colour c, float stroke = 1.5f)
    {
        juce::Path p;
        const int bars = 4;
        const float gap = bounds.getWidth() * 0.15f;
        const float barW = (bounds.getWidth() - gap * (bars - 1)) / bars;
        for (int i = 0; i < bars; ++i)
        {
            float h = bounds.getHeight() * (0.4f + 0.15f * (i % 2));
            float x = bounds.getX() + i * (barW + gap);
            float y = bounds.getCentreY() - h * 0.5f;
            p.addRectangle(x, y, barW, h);
        }
        g.setColour(c);
        g.fillPath(p);
    }

    static void drawLoopIcon(juce::Graphics& g, juce::Rectangle<float> bounds,
                             juce::Colour c, float stroke = 1.5f)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float rx = bounds.getWidth() * 0.35f;
        float ry = bounds.getHeight() * 0.25f;
        juce::Path p;
        p.addCentredArc(cx, cy, rx, ry, 0.0f, 0.2f, 5.5f, true);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

        juce::Path arrow;
        float ax = cx + rx * std::cos(5.5f);
        float ay = cy + ry * std::sin(5.5f);
        arrow.addTriangle(ax, ay - 3.0f, ax + 5.0f, ay + 3.0f, ax - 5.0f, ay + 3.0f);
        g.fillPath(arrow);
    }

    static void drawEnvelopeIcon(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 juce::Colour c, float stroke = 1.5f)
    {
        juce::Path p;
        float x = bounds.getX();
        float y = bounds.getBottom() - bounds.getHeight() * 0.25f;
        p.startNewSubPath(x, y);
        p.lineTo(x + bounds.getWidth() * 0.2f, bounds.getY() + bounds.getHeight() * 0.3f);
        p.lineTo(x + bounds.getWidth() * 0.5f, bounds.getBottom() - bounds.getHeight() * 0.25f);
        p.lineTo(x + bounds.getWidth() * 0.8f, bounds.getY() + bounds.getHeight() * 0.6f);
        p.lineTo(bounds.getRight(), bounds.getBottom() - bounds.getHeight() * 0.25f);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawFxIcon(juce::Graphics& g, juce::Rectangle<float> bounds,
                           juce::Colour c, float stroke = 1.5f)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float r = bounds.getWidth() * 0.12f;
        for (int i = 0; i < 4; ++i)
        {
            float angle = float(i) * juce::MathConstants<float>::halfPi;
            float dx = std::cos(angle) * bounds.getWidth() * 0.22f;
            float dy = std::sin(angle) * bounds.getHeight() * 0.22f;
            g.setColour(i == 0 ? c.brighter(0.3f) : c);
            g.fillEllipse(cx + dx - r, cy + dy - r, r * 2.0f, r * 2.0f);
        }
        g.setColour(c.brighter(0.5f));
        g.fillEllipse(cx - r * 0.6f, cy - r * 0.6f, r * 1.2f, r * 1.2f);
    }

    static void drawFilterIcon(juce::Graphics& g, juce::Rectangle<float> bounds,
                               juce::Colour c, float stroke = 1.5f)
    {
        juce::Path p;
        float x = bounds.getX();
        float yBase = bounds.getBottom() - bounds.getHeight() * 0.2f;
        p.startNewSubPath(x, yBase);
        p.lineTo(x + bounds.getWidth() * 0.25f, yBase);
        p.cubicTo(x + bounds.getWidth() * 0.35f, yBase,
                  x + bounds.getWidth() * 0.4f, bounds.getY() + bounds.getHeight() * 0.15f,
                  x + bounds.getWidth() * 0.5f, bounds.getY() + bounds.getHeight() * 0.15f);
        p.cubicTo(x + bounds.getWidth() * 0.6f, bounds.getY() + bounds.getHeight() * 0.15f,
                  x + bounds.getWidth() * 0.65f, yBase,
                  x + bounds.getWidth() * 0.75f, yBase);
        p.lineTo(bounds.getRight(), yBase);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawLaneIcon(juce::Graphics& g, int lane, juce::Rectangle<float> bounds,
                             juce::Colour c, float stroke = 1.5f)
    {
        switch (lane)
        {
            case 0: drawSliceIcon(g, bounds, c, stroke); break;
            case 1: drawLoopIcon(g, bounds, c, stroke); break;
            case 2: drawEnvelopeIcon(g, bounds, c, stroke); break;
            case 3: drawFxIcon(g, bounds, c, stroke); break;
            case 4: drawFilterIcon(g, bounds, c, stroke); break;
            case 5: drawFxIcon(g, bounds, c, stroke); break;
            default: break;
        }
    }

    // ── Preset icons by lane + presetIndex ────────────────────────────
    static void drawPresetIcon(juce::Graphics& g, int lane, int presetIndex,
                               juce::Rectangle<float> bounds, juce::Colour c)
    {
        if (presetIndex >= 1 && presetIndex <= 4)
        {
            drawUserSlotIcon(g, presetIndex, bounds, c);
            return;
        }

        int fx = presetIndex - 5;
        switch (lane)
        {
            case 0: drawSlicePresetIcon(g, fx, bounds, c); break;
            case 1: drawLoopPresetIcon(g, fx, bounds, c); break;
            case 2: drawEnvelopePresetIcon(g, fx, bounds, c); break;
            case 3: drawFxPresetIcon(g, fx, bounds, c); break;
            case 4: drawFilterPresetIcon(g, fx, bounds, c); break;
            case 5: drawFxPresetIcon(g, fx, bounds, c); break;
            default: break;
        }
    }

    static void drawUserSlotIcon(juce::Graphics& g, int slotIndex,
                                 juce::Rectangle<float> bounds, juce::Colour c)
    {
        juce::String text = "U" + juce::String(slotIndex);
        g.setFont(juce::Font(juce::FontOptions().withHeight(bounds.getHeight() * 0.5f).withStyle("Bold")));
        g.setColour(c);
        g.drawText(text, bounds, juce::Justification::centred, false);
    }

    // ── SLICE presets ────────────────────────────────────────────────
    static void drawSlicePresetIcon(juce::Graphics& g, int fx,
                                    juce::Rectangle<float> bounds, juce::Colour c)
    {
        int slices = fx + 1;
        if (slices > 16) slices = 16;
        int cols = (slices <= 4) ? slices : (slices <= 8) ? 4 : 5;
        int rows = (slices + cols - 1) / cols;
        float padX = bounds.getWidth() * 0.08f;
        float padY = bounds.getHeight() * 0.08f;
        float cellW = (bounds.getWidth() - padX * (cols + 1)) / cols;
        float cellH = (bounds.getHeight() - padY * (rows + 1)) / rows;
        float r = juce::jmin(cellW, cellH) * 0.15f;
        g.setColour(c);
        int drawn = 0;
        for (int row = 0; row < rows && drawn < slices; ++row)
        {
            for (int col = 0; col < cols && drawn < slices; ++col)
            {
                float cx = bounds.getX() + padX + col * (cellW + padX) + cellW * 0.5f;
                float cy = bounds.getY() + padY + row * (cellH + padY) + cellH * 0.5f;
                g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
                ++drawn;
            }
        }
    }

    // ── LOOP presets ─────────────────────────────────────────────────
    static void drawLoopPresetIcon(juce::Graphics& g, int fx,
                                   juce::Rectangle<float> bounds, juce::Colour c)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float w = bounds.getWidth() * 0.55f;
        float h = bounds.getHeight() * 0.35f;

        juce::Path p;
        bool reverse = (fx >= 8);
        int style = fx % 8;
        int arrows = (style < 2) ? 1 : (style < 5) ? 2 : 3;
        float spacing = h * 0.45f;
        float startY = cy - (arrows - 1) * spacing * 0.5f;

        for (int i = 0; i < arrows; ++i)
        {
            float y = startY + i * spacing;
            float x1 = reverse ? cx + w * 0.5f : cx - w * 0.5f;
            float x2 = reverse ? cx - w * 0.5f : cx + w * 0.5f;
            p.startNewSubPath(x1, y);
            p.lineTo(x2, y);
            float headX = reverse ? x2 + 5.0f : x2 - 5.0f;
            p.startNewSubPath(headX, y - 3.5f);
            p.lineTo(x2, y);
            p.lineTo(headX, y + 3.5f);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    // ── ENVELOPE presets ─────────────────────────────────────────────
    static void drawEnvelopePresetIcon(juce::Graphics& g, int fx,
                                       juce::Rectangle<float> bounds, juce::Colour c)
    {
        juce::Path p;
        float x = bounds.getX() + bounds.getWidth() * 0.1f;
        float w = bounds.getWidth() * 0.8f;
        float yBase = bounds.getBottom() - bounds.getHeight() * 0.2f;
        float yTop = bounds.getY() + bounds.getHeight() * 0.2f;

        p.startNewSubPath(x, yBase);
        switch (fx % 8)
        {
            case 0:
                p.lineTo(x + w * 0.3f, yTop);
                p.lineTo(x + w, yBase);
                break;
            case 1:
                p.lineTo(x + w * 0.3f, yTop);
                p.lineTo(x + w, yBase);
                break;
            case 2:
                p.lineTo(x + w * 0.2f, yTop);
                p.lineTo(x + w * 0.7f, yTop);
                p.lineTo(x + w, yBase);
                break;
            case 3:
                p.cubicTo(x + w * 0.3f, yTop, x + w * 0.6f, yTop, x + w, yBase);
                break;
            case 4:
                p.lineTo(x + w * 0.15f, yTop);
                p.lineTo(x + w * 0.25f, yBase);
                p.lineTo(x + w, yBase);
                break;
            case 5:
                p.cubicTo(x + w * 0.2f, yBase, x + w * 0.8f, yTop, x + w, yTop);
                break;
            case 6:
                p.lineTo(x + w * 0.5f, yTop);
                p.lineTo(x + w * 0.5f, yBase);
                p.lineTo(x + w, yBase);
                break;
            case 7:
            default:
                p.lineTo(x + w * 0.2f, yTop);
                p.lineTo(x + w * 0.2f, yBase);
                p.lineTo(x + w * 0.5f, yBase);
                p.lineTo(x + w * 0.5f, yTop);
                p.lineTo(x + w * 0.75f, yTop);
                p.lineTo(x + w * 0.75f, yBase);
                p.lineTo(x + w, yBase);
                break;
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    // ── FX presets ───────────────────────────────────────────────────
    static void drawFxPresetIcon(juce::Graphics& g, int fx,
                                 juce::Rectangle<float> bounds, juce::Colour c)
    {
        switch (fx % 16)
        {
            case 0:  drawIconDelay(g, bounds, c); break;
            case 1:  drawIconReverb(g, bounds, c); break;
            case 2:  drawIconChorus(g, bounds, c); break;
            case 3:  drawIconBitcrush(g, bounds, c); break;
            case 4:  drawIconPitch(g, bounds, c); break;
            case 5:  drawIconFlanger(g, bounds, c); break;
            case 6:  drawIconPhaser(g, bounds, c); break;
            case 7:  drawIconTremolo(g, bounds, c); break;
            case 8:  drawIconDistortion(g, bounds, c); break;
            case 9:  drawIconGrain(g, bounds, c); break;
            case 10: drawIconVinyl(g, bounds, c); break;
            case 11: drawIconStretch(g, bounds, c); break;
            case 12: drawIconRingMod(g, bounds, c); break;
            case 13: drawIconTonalizer(g, bounds, c); break;
            case 14: drawIconChaos(g, bounds, c); break;
            case 15: drawIconPhaser2(g, bounds, c); break;
            default: break;
        }
    }

    // ── FILTER presets ───────────────────────────────────────────────
    static void drawFilterPresetIcon(juce::Graphics& g, int fx,
                                     juce::Rectangle<float> bounds, juce::Colour c)
    {
        juce::Path p;
        float x = bounds.getX() + bounds.getWidth() * 0.08f;
        float w = bounds.getWidth() * 0.84f;
        float yBase = bounds.getBottom() - bounds.getHeight() * 0.15f;
        float yTop = bounds.getY() + bounds.getHeight() * 0.15f;

        p.startNewSubPath(x, yBase);
        switch (fx % 16)
        {
            case 0:
            case 1:
                p.lineTo(x + w * 0.25f, yBase);
                p.cubicTo(x + w * 0.35f, yBase, x + w * 0.4f, yTop, x + w * 0.55f, yTop);
                p.lineTo(x + w, yTop);
                break;
            case 2:
            case 3:
                p.lineTo(x, yTop);
                p.lineTo(x + w * 0.45f, yTop);
                p.cubicTo(x + w * 0.6f, yTop, x + w * 0.65f, yBase, x + w * 0.75f, yBase);
                p.lineTo(x + w, yBase);
                break;
            case 4:
                p.lineTo(x + w * 0.15f, yBase);
                p.cubicTo(x + w * 0.3f, yBase, x + w * 0.4f, yTop, x + w * 0.5f, yTop);
                p.cubicTo(x + w * 0.6f, yTop, x + w * 0.7f, yBase, x + w * 0.85f, yBase);
                p.lineTo(x + w, yBase);
                break;
            case 5:
                p.lineTo(x, yTop);
                p.lineTo(x + w * 0.3f, yTop);
                p.cubicTo(x + w * 0.4f, yTop, x + w * 0.45f, yBase, x + w * 0.55f, yBase);
                p.cubicTo(x + w * 0.65f, yBase, x + w * 0.7f, yTop, x + w, yTop);
                break;
            case 6:
                p.cubicTo(x + w * 0.25f, yBase, x + w * 0.35f, yTop, x + w * 0.5f, yTop);
                p.cubicTo(x + w * 0.65f, yTop, x + w * 0.75f, yBase, x + w, yBase);
                break;
            case 7:
                p.cubicTo(x + w * 0.2f, yBase, x + w * 0.35f, yTop, x + w * 0.5f, yTop);
                p.cubicTo(x + w * 0.65f, yTop, x + w * 0.8f, yBase, x + w, yBase);
                break;
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            {
                juce::String v = juce::String::charToString(juce::String("AEIOU")[juce::jmin(4, fx - 8)]);
                g.setFont(juce::Font(juce::FontOptions().withHeight(bounds.getHeight() * 0.55f).withStyle("Bold")));
                g.setColour(c);
                g.drawText(v, bounds, juce::Justification::centred, false);
                return;
            }
            case 13:
                drawIconMorph(g, bounds, c); return;
            case 14:
                drawIconTalk(g, bounds, c); return;
            case 15:
            default:
                p.lineTo(x + w * 0.2f, yBase);
                p.cubicTo(x + w * 0.4f, yBase, x + w * 0.5f, yTop, x + w * 0.7f, yTop);
                p.lineTo(x + w, yTop);
                break;
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    // ── FX sub-icons ─────────────────────────────────────────────────
    static void drawIconDelay(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float y = b.getCentreY();
        juce::Path p;
        p.startNewSubPath(b.getX() + b.getWidth() * 0.15f, y);
        p.lineTo(b.getRight() - b.getWidth() * 0.15f, y);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.0f));
        for (int i = 1; i <= 3; ++i)
        {
            float x = b.getX() + b.getWidth() * 0.25f * float(i);
            g.drawLine(x, y - 4.0f, x, y + 4.0f, 1.5f);
        }
    }

    static void drawIconReverb(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        g.setColour(c.withAlpha(0.25f));
        for (int i = 3; i >= 1; --i)
        {
            float r = b.getWidth() * 0.12f * i;
            g.drawEllipse(cx - r, cy - r * 0.6f, r * 2.0f, r * 1.2f, 1.5f);
        }
        g.setColour(c);
        g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
    }

    static void drawIconChorus(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        for (int i = -1; i <= 1; ++i)
        {
            float y = cy + i * 5.0f;
            g.setColour(c.withAlpha(0.7f - std::abs(i) * 0.25f));
            g.drawLine(cx - 10.0f, y, cx + 10.0f, y, 2.0f);
        }
    }

    static void drawIconBitcrush(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        juce::Path p;
        float x = b.getX() + 4.0f;
        float w = b.getWidth() - 8.0f;
        float yBase = b.getBottom() - 5.0f;
        float yTop = b.getY() + 5.0f;
        p.startNewSubPath(x, yBase);
        for (int i = 0; i < 8; ++i)
        {
            float nx = x + (i + 1) * (w / 8.0f);
            float ny = ((i % 2) == 0) ? yTop : yBase;
            p.lineTo(nx - w / 16.0f, p.getCurrentPosition().getY());
            p.lineTo(nx - w / 16.0f, ny);
            p.lineTo(nx, ny);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(1.5f));
    }

    static void drawIconPitch(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        juce::Path p;
        p.addTriangle(cx, cy - 10.0f, cx - 7.0f, cy + 6.0f, cx + 7.0f, cy + 6.0f);
        g.setColour(c.withAlpha(0.25f));
        g.fillPath(p);
        p.clear();
        p.addTriangle(cx, cy + 10.0f, cx - 7.0f, cy - 6.0f, cx + 7.0f, cy - 6.0f);
        g.setColour(c);
        g.fillPath(p);
    }

    static void drawIconFlanger(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        juce::Path p;
        for (int i = 0; i < 3; ++i)
        {
            float r = 5.0f + i * 4.0f;
            p.addArc(cx - r, cy - r * 0.5f, r * 2.0f, r, 0.5f, 2.5f, true);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawIconPhaser(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        for (int i = 0; i < 4; ++i)
        {
            float angle = float(i) * juce::MathConstants<float>::halfPi + 0.3f;
            float r = 10.0f;
            float x1 = cx + std::cos(angle) * 5.0f;
            float y1 = cy + std::sin(angle) * 5.0f;
            float x2 = cx + std::cos(angle) * r;
            float y2 = cy + std::sin(angle) * r;
            g.setColour(c.withAlpha(0.6f));
            g.drawLine(x1, y1, x2, y2, 2.5f);
            g.setColour(c);
            g.fillEllipse(x2 - 2.5f, y2 - 2.5f, 5.0f, 5.0f);
        }
    }

    static void drawIconTremolo(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        juce::Path p;
        float x = b.getX() + 4.0f;
        float w = b.getWidth() - 8.0f;
        float cy = b.getCentreY();
        float amp = b.getHeight() * 0.25f;
        p.startNewSubPath(x, cy);
        for (int i = 1; i <= 20; ++i)
        {
            float nx = x + i * (w / 20.0f);
            float ny = cy + std::sin(i * 0.8f) * amp;
            p.lineTo(nx, ny);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawIconDistortion(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        juce::Path p;
        float x = b.getX() + 4.0f;
        float w = b.getWidth() - 8.0f;
        float h = b.getHeight();
        p.startNewSubPath(x, b.getBottom() - 4.0f);
        p.cubicTo(x + w * 0.3f, b.getBottom() - 4.0f,
                  x + w * 0.5f, b.getY() + h * 0.5f,
                  x + w, b.getY() + 4.0f);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawIconGrain(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        juce::Random rng(42);
        g.setColour(c);
        for (int i = 0; i < 12; ++i)
        {
            float rx = cx + (rng.nextFloat() - 0.5f) * b.getWidth() * 0.6f;
            float ry = cy + (rng.nextFloat() - 0.5f) * b.getHeight() * 0.6f;
            float r = 1.5f + rng.nextFloat() * 2.0f;
            g.fillEllipse(rx - r, ry - r, r * 2.0f, r * 2.0f);
        }
    }

    static void drawIconVinyl(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.28f;
        g.setColour(c);
        g.drawEllipse(cx - r, cy - r * 0.6f, r * 2.0f, r * 1.2f, 2.0f);
        g.drawLine(cx, cy - r * 0.6f, cx, cy - r * 0.6f - 6.0f, 2.0f);
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }

    static void drawIconStretch(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        juce::Path p;
        float x = b.getX() + 5.0f;
        float w = b.getWidth() - 10.0f;
        float cy = b.getCentreY();
        p.startNewSubPath(x, cy);
        p.lineTo(x + w * 0.3f, cy);
        p.lineTo(x + w * 0.4f, cy - 7.0f);
        p.lineTo(x + w * 0.6f, cy + 7.0f);
        p.lineTo(x + w * 0.7f, cy);
        p.lineTo(x + w, cy);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawIconRingMod(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        juce::Path p;
        p.addCentredArc(cx, cy, b.getWidth() * 0.22f, b.getHeight() * 0.22f,
                        0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
        g.setColour(c.withAlpha(0.3f));
        g.strokePath(p, juce::PathStrokeType(2.0f));
        juce::Path sine;
        sine.startNewSubPath(cx - 10.0f, cy);
        for (int i = 0; i <= 20; ++i)
        {
            float nx = cx - 10.0f + i;
            float ny = cy + std::sin(i * 0.5f) * 6.0f;
            sine.lineTo(nx, ny);
        }
        g.setColour(c);
        g.strokePath(sine, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    static void drawIconTonalizer(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        for (int i = -2; i <= 2; ++i)
        {
            float y = cy + i * 5.0f;
            g.setColour(c.withAlpha(0.8f - std::abs(i) * 0.2f));
            g.fillEllipse(cx - 3.0f, y - 2.0f, 6.0f, 4.0f);
        }
        g.setColour(c);
        g.drawLine(cx - 10.0f, cy, cx + 10.0f, cy, 1.5f);
    }

    static void drawIconChaos(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        juce::Path p;
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        for (int i = 0; i < 6; ++i)
        {
            float angle = float(i) * juce::MathConstants<float>::pi / 3.0f;
            float r = (i % 2 == 0) ? 10.0f : 5.0f;
            float x = cx + std::cos(angle) * r;
            float y = cy + std::sin(angle) * r;
            if (i == 0) p.startNewSubPath(x, y);
            else p.lineTo(x, y);
        }
        p.closeSubPath();
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawIconPhaser2(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        juce::Path p;
        for (int i = 0; i < 5; ++i)
        {
            float r = 4.0f + i * 3.0f;
            p.addEllipse(cx - r, cy - r * 0.5f, r * 2.0f, r);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawIconMorph(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        juce::Path p;
        float x = b.getX() + 4.0f;
        float w = b.getWidth() - 8.0f;
        float cy = b.getCentreY();
        p.startNewSubPath(x, cy);
        p.cubicTo(x + w * 0.3f, cy - 8.0f, x + w * 0.7f, cy + 8.0f, x + w, cy);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    static void drawIconTalk(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        juce::Path p;
        p.addEllipse(cx - 8.0f, cy - 5.0f, 16.0f, 10.0f);
        g.setColour(c.withAlpha(0.25f));
        g.fillPath(p);
        g.setColour(c);
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }
};

} // namespace zikada
