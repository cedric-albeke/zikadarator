#pragma once

#include "../ZikadaLookAndFeel.h"

namespace zikada {

struct PresetIcons
{
    static void drawVariationBadge(juce::Graphics& g, juce::Rectangle<float> bounds,
                                   int variation, juce::Colour c)
    {
        float dotR = bounds.getWidth() * 0.06f;
        float gap = dotR * 2.2f;
        float x = bounds.getRight() - gap * 1.2f;
        float y = bounds.getBottom() - gap;
        g.setColour(c);
        if (variation >= 0) g.fillEllipse(x, y, dotR * 2.0f, dotR * 2.0f);
        if (variation >= 1) g.fillEllipse(x + gap, y, dotR * 2.0f, dotR * 2.0f);
        if (variation >= 2) g.fillEllipse(x, y - gap, dotR * 2.0f, dotR * 2.0f);
        if (variation >= 3) g.fillEllipse(x + gap, y - gap, dotR * 2.0f, dotR * 2.0f);
    }

    static void drawSliceIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, float stroke = 1.5f)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float r = bounds.getWidth() * 0.09f;
        g.setColour(c);
        g.drawEllipse(cx - r * 2.8f - r, cy - r * 1.2f - r, r * 2.0f, r * 2.0f, stroke);
        g.drawEllipse(cx + r * 2.8f - r, cy - r * 1.2f - r, r * 2.0f, r * 2.0f, stroke);
        juce::Path blades;
        float x1 = cx - bounds.getWidth() * 0.32f;
        float y1 = cy + bounds.getHeight() * 0.22f;
        float x2 = cx + bounds.getWidth() * 0.32f;
        float y2 = cy - bounds.getHeight() * 0.22f;
        blades.startNewSubPath(x1, y1);
        blades.lineTo(x2, y2);
        float x3 = cx - bounds.getWidth() * 0.32f;
        float y3 = cy - bounds.getHeight() * 0.22f;
        float x4 = cx + bounds.getWidth() * 0.32f;
        float y4 = cy + bounds.getHeight() * 0.22f;
        blades.startNewSubPath(x3, y3);
        blades.lineTo(x4, y4);
        g.strokePath(blades, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawLoopIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, float stroke = 1.5f)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float r = bounds.getWidth() * 0.26f;
        juce::Path p;
        p.addCentredArc(cx, cy, r, r * 0.65f, 0.0f, 0.4f, 5.6f, true);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        float ax = cx + r * std::cos(5.6f);
        float ay = cy + r * 0.65f * std::sin(5.6f);
        juce::Path arrow;
        arrow.addTriangle(ax - 3.0f, ay - 3.0f, ax + 3.0f, ay, ax - 3.0f, ay + 3.0f);
        g.fillPath(arrow);
    }

    static void drawEnvelopeIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, float stroke = 1.5f)
    {
        juce::Path p;
        float x = bounds.getX() + bounds.getWidth() * 0.1f;
        float w = bounds.getWidth() * 0.8f;
        float yBase = bounds.getBottom() - bounds.getHeight() * 0.25f;
        float yTop = bounds.getY() + bounds.getHeight() * 0.25f;
        p.startNewSubPath(x, yBase);
        p.lineTo(x + w * 0.15f, yTop);
        p.lineTo(x + w * 0.4f, yTop);
        p.lineTo(x + w * 0.55f, yBase);
        p.lineTo(x + w * 0.7f, yBase);
        p.lineTo(x + w * 0.85f, yTop * 0.7f + yBase * 0.3f);
        p.lineTo(x + w, yBase);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawFx1Icon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, float stroke = 1.5f)
    {
        juce::ignoreUnused(stroke);
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float rOut = bounds.getWidth() * 0.28f;
        float rIn = rOut * 0.35f;
        juce::Path star;
        for (int i = 0; i < 8; ++i)
        {
            float angle = float(i) * juce::MathConstants<float>::pi / 4.0f - juce::MathConstants<float>::pi / 8.0f;
            float r = (i % 2 == 0) ? rOut : rIn;
            float x = cx + std::cos(angle) * r;
            float y = cy + std::sin(angle) * r;
            if (i == 0) star.startNewSubPath(x, y);
            else star.lineTo(x, y);
        }
        star.closeSubPath();
        g.setColour(c);
        g.fillPath(star);
    }

    static void drawFx2Icon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, float stroke = 1.5f)
    {
        juce::ignoreUnused(stroke);
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float w = bounds.getWidth() * 0.72f;
        g.setColour(c);
        for (int row = -1; row <= 1; ++row)
        {
            juce::Path wave;
            float y = cy + row * bounds.getHeight() * 0.14f;
            wave.startNewSubPath(cx - w * 0.5f, y);
            for (int i = 0; i <= 10; ++i)
            {
                float nx = cx - w * 0.5f + i * (w / 10.0f);
                float ny = y + std::sin(i * 0.6f + row * 0.3f) * bounds.getHeight() * 0.08f;
                wave.lineTo(nx, ny);
            }
            g.strokePath(wave, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    static void drawFilterIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, float stroke = 1.5f)
    {
        juce::Path p;
        float x = bounds.getX() + bounds.getWidth() * 0.1f;
        float w = bounds.getWidth() * 0.8f;
        float yBase = bounds.getBottom() - bounds.getHeight() * 0.2f;
        float yTop = bounds.getY() + bounds.getHeight() * 0.15f;
        p.startNewSubPath(x, yBase);
        p.lineTo(x + w * 0.25f, yBase);
        p.cubicTo(x + w * 0.35f, yBase, x + w * 0.4f, yTop, x + w * 0.5f, yTop);
        p.cubicTo(x + w * 0.6f, yTop, x + w * 0.65f, yBase, x + w * 0.75f, yBase);
        p.lineTo(x + w, yBase);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawLaneIcon(juce::Graphics& g, int lane, juce::Rectangle<float> bounds,
                             juce::Colour c, float stroke = 1.5f)
    {
        switch (lane)
        {
            case 0: drawSliceIcon(g, bounds, c, stroke); break;
            case 1: drawLoopIcon(g, bounds, c, stroke); break;
            case 2: drawEnvelopeIcon(g, bounds, c, stroke); break;
            case 3: drawFx1Icon(g, bounds, c, stroke); break;
            case 4: drawFilterIcon(g, bounds, c, stroke); break;
            case 5: drawFx2Icon(g, bounds, c, stroke); break;
            default: break;
        }
    }

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
            case 3: drawFx1PresetIcon(g, fx, bounds, c); break;
            case 4: drawFilterPresetIcon(g, fx, bounds, c); break;
            case 5: drawFx2PresetIcon(g, fx, bounds, c); break;
            default: break;
        }
    }

    static void drawUserSlotIcon(juce::Graphics& g, int slotIndex,
                                 juce::Rectangle<float> bounds, juce::Colour c)
    {
        juce::String text = "U" + juce::String(slotIndex);
        g.setFont(juce::Font(juce::FontOptions().withHeight(bounds.getHeight() * 0.45f).withStyle("Bold")));
        g.setColour(c);
        g.drawText(text, bounds, juce::Justification::centred, false);
    }

    static void drawSlicePresetIcon(juce::Graphics& g, int fx,
                                    juce::Rectangle<float> bounds, juce::Colour c)
    {
        static const char* labels[] = {
            "1", "2", "3", "4", "5", "6", "8", "10",
            "12", "14", "16", "FWD", "REV", "SC", "RP", "ST"
        };
        juce::String text = labels[juce::jlimit(0, 15, fx)];
        float fontH = bounds.getHeight() * (text.length() > 2 ? 0.32f : 0.45f);
        g.setFont(juce::Font(juce::FontOptions().withHeight(fontH).withStyle("Bold")));
        g.setColour(c);
        g.drawText(text, bounds, juce::Justification::centred, false);
    }

    static void drawLoopPresetIcon(juce::Graphics& g, int fx,
                                   juce::Rectangle<float> bounds, juce::Colour c)
    {
        int base = fx / 2;
        int var = fx % 2;
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float r = bounds.getWidth() * 0.22f;
        juce::Path ring;
        ring.addEllipse(cx - r, cy - r * 0.7f, r * 2.0f, r * 1.4f);
        g.setColour(c.withAlpha(0.4f));
        g.strokePath(ring, juce::PathStrokeType(2.0f));
        if (base < 4)
        {
            bool rev = (base >= 2);
            int beats = (base % 2 == 0) ? 1 : 2;
            float x1 = rev ? cx + r * 0.6f : cx - r * 0.6f;
            float x2 = rev ? cx - r * 0.6f : cx + r * 0.6f;
            float y = cy + (beats - 1) * 3.0f;
            juce::Path arrow;
            arrow.startNewSubPath(x1, y);
            arrow.lineTo(x2, y);
            float hx = rev ? x2 + 4.0f : x2 - 4.0f;
            arrow.startNewSubPath(hx, y - 3.0f);
            arrow.lineTo(x2, y);
            arrow.lineTo(hx, y + 3.0f);
            g.setColour(c);
            g.strokePath(arrow, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (base < 6)
        {
            juce::Path zig;
            zig.startNewSubPath(cx - r * 0.5f, cy);
            for (int i = 1; i <= 6; ++i)
            {
                float nx = cx - r * 0.5f + i * (r / 3.0f);
                float ny = cy + ((i % 2) * 2.0f - 1.0f) * 3.0f;
                zig.lineTo(nx, ny);
            }
            g.setColour(c);
            g.strokePath(zig, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            g.setColour(c);
            g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
        }
        drawVariationBadge(g, bounds, var, c);
    }

    static void drawEnvelopePresetIcon(juce::Graphics& g, int fx,
                                       juce::Rectangle<float> bounds, juce::Colour c)
    {
        int shape = fx % 8;
        juce::Path p;
        float x = bounds.getX() + bounds.getWidth() * 0.1f;
        float w = bounds.getWidth() * 0.8f;
        float yBase = bounds.getBottom() - bounds.getHeight() * 0.2f;
        float yTop = bounds.getY() + bounds.getHeight() * 0.2f;
        p.startNewSubPath(x, yBase);
        switch (shape)
        {
            case 0: p.lineTo(x + w * 0.3f, yTop); p.lineTo(x + w, yBase); break;
            case 1: p.lineTo(x + w * 0.3f, yTop); p.lineTo(x + w, yBase); break;
            case 2: p.lineTo(x + w * 0.2f, yTop); p.lineTo(x + w * 0.7f, yTop); p.lineTo(x + w, yBase); break;
            case 3: p.cubicTo(x + w * 0.3f, yTop, x + w * 0.6f, yTop, x + w, yBase); break;
            case 4: p.lineTo(x + w * 0.15f, yTop); p.lineTo(x + w * 0.25f, yBase); p.lineTo(x + w, yBase); break;
            case 5: p.cubicTo(x + w * 0.2f, yBase, x + w * 0.8f, yTop, x + w, yTop); break;
            case 6: p.lineTo(x + w * 0.5f, yTop); p.lineTo(x + w * 0.5f, yBase); p.lineTo(x + w, yBase); break;
            case 7:
            default:
                p.lineTo(x + w * 0.2f, yTop); p.lineTo(x + w * 0.2f, yBase);
                p.lineTo(x + w * 0.5f, yBase); p.lineTo(x + w * 0.5f, yTop);
                p.lineTo(x + w * 0.75f, yTop); p.lineTo(x + w * 0.75f, yBase); p.lineTo(x + w, yBase);
                break;
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawFx1PresetIcon(juce::Graphics& g, int fx,
                                  juce::Rectangle<float> bounds, juce::Colour c)
    {
        int base = fx / 2;
        int var = fx % 2;
        switch (base)
        {
            case 0: drawIconDelay(g, bounds, c, var); break;
            case 1: drawIconReverb(g, bounds, c, var); break;
            case 2: drawIconChorus(g, bounds, c, var); break;
            case 3: drawIconFlanger(g, bounds, c, var); break;
            case 4: drawIconPhaser(g, bounds, c, var); break;
            case 5: drawIconTremolo(g, bounds, c, var); break;
            case 6: drawIconDistortion(g, bounds, c, var); break;
            case 7: drawIconGrain(g, bounds, c, var); break;
            default: break;
        }
        drawVariationBadge(g, bounds, var, c);
    }

    static void drawFx2PresetIcon(juce::Graphics& g, int fx,
                                  juce::Rectangle<float> bounds, juce::Colour c)
    {
        int base = fx / 2;
        int var = fx % 2;
        switch (base)
        {
            case 0: drawIconBitcrush(g, bounds, c, var); break;
            case 1: drawIconPitch(g, bounds, c, var); break;
            case 2: drawIconVinyl(g, bounds, c, var); break;
            case 3: drawIconStretch(g, bounds, c, var); break;
            case 4: drawIconRingMod(g, bounds, c, var); break;
            case 5: drawIconTonalizer(g, bounds, c, var); break;
            case 6: drawIconChaos(g, bounds, c, var); break;
            case 7: drawIconSpace(g, bounds, c, var); break;
            default: break;
        }
        drawVariationBadge(g, bounds, var, c);
    }

    static void drawFilterPresetIcon(juce::Graphics& g, int fx,
                                     juce::Rectangle<float> bounds, juce::Colour c)
    {
        int var = fx % 2;
        juce::Path p;
        float x = bounds.getX() + bounds.getWidth() * 0.08f;
        float w = bounds.getWidth() * 0.84f;
        float yBase = bounds.getBottom() - bounds.getHeight() * 0.15f;
        float yTop = bounds.getY() + bounds.getHeight() * 0.15f;
        p.startNewSubPath(x, yBase);
        switch (fx)
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
            case 13: drawIconMorph(g, bounds, c); return;
            case 14: drawIconTalk(g, bounds, c); return;
            case 15:
            default:
                p.lineTo(x + w * 0.2f, yBase);
                p.cubicTo(x + w * 0.4f, yBase, x + w * 0.5f, yTop, x + w * 0.7f, yTop);
                p.lineTo(x + w, yTop);
                break;
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (fx < 8) drawVariationBadge(g, bounds, var, c);
    }

    static void drawIconDelay(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        juce::ignoreUnused(var);
        float cy = b.getCentreY();
        float x0 = b.getX() + b.getWidth() * 0.15f;
        float w = b.getWidth() * 0.7f;
        g.setColour(c);
        for (int i = 0; i < 3; ++i)
        {
            float alpha = 1.0f - i * 0.25f;
            float off = i * w * 0.18f;
            g.setColour(c.withAlpha(alpha));
            g.drawLine(x0 + off, cy - 5.0f, x0 + off, cy + 5.0f, 2.5f);
        }
    }

    static void drawIconReverb(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        int rings = (var == 0) ? 3 : 4;
        g.setColour(c.withAlpha(0.25f));
        for (int i = rings; i >= 1; --i)
        {
            float r = b.getWidth() * 0.1f * i;
            g.drawEllipse(cx - r, cy - r * 0.6f, r * 2.0f, r * 1.2f, 1.5f);
        }
        g.setColour(c);
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }

    static void drawIconChorus(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        int lines = (var == 0) ? 3 : 5;
        float spacing = b.getHeight() * 0.12f;
        float startY = cy - (lines - 1) * spacing * 0.5f;
        for (int i = 0; i < lines; ++i)
        {
            float y = startY + i * spacing;
            float alpha = 1.0f - std::abs(i - (lines - 1) / 2.0f) * 0.25f;
            g.setColour(c.withAlpha(alpha));
            g.drawLine(cx - b.getWidth() * 0.3f, y, cx + b.getWidth() * 0.3f, y, 2.0f);
        }
    }

    static void drawIconFlanger(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        int arcs = (var == 0) ? 2 : 4;
        juce::Path p;
        for (int i = 0; i < arcs; ++i)
        {
            float r = 4.0f + i * 4.0f;
            p.addArc(cx - r, cy - r * 0.5f, r * 2.0f, r, 0.5f, 2.5f, true);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawIconPhaser(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        int spokes = (var == 0) ? 4 : 6;
        float rIn = 5.0f;
        float rOut = 11.0f;
        for (int i = 0; i < spokes; ++i)
        {
            float angle = float(i) * juce::MathConstants<float>::twoPi / spokes + 0.2f;
            float x1 = cx + std::cos(angle) * rIn;
            float y1 = cy + std::sin(angle) * rIn;
            float x2 = cx + std::cos(angle) * rOut;
            float y2 = cy + std::sin(angle) * rOut;
            g.setColour(c.withAlpha(0.5f));
            g.drawLine(x1, y1, x2, y2, 2.5f);
            g.setColour(c);
            g.fillEllipse(x2 - 2.0f, y2 - 2.0f, 4.0f, 4.0f);
        }
    }

    static void drawIconTremolo(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        juce::Path p;
        float x = b.getX() + 4.0f;
        float w = b.getWidth() - 8.0f;
        float cy = b.getCentreY();
        float amp = (var == 0) ? b.getHeight() * 0.22f : b.getHeight() * 0.32f;
        float freq = (var == 0) ? 0.6f : 1.2f;
        p.startNewSubPath(x, cy);
        for (int i = 1; i <= 24; ++i)
        {
            float nx = x + i * (w / 24.0f);
            float ny = cy + std::sin(i * freq) * amp;
            p.lineTo(nx, ny);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawIconDistortion(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        juce::Path p;
        float x = b.getX() + 4.0f;
        float w = b.getWidth() - 8.0f;
        float h = b.getHeight();
        float clip = (var == 0) ? 0.35f : 0.15f;
        p.startNewSubPath(x, b.getBottom() - 4.0f);
        float mx = x + w * 0.5f;
        float my = b.getY() + h * clip;
        p.cubicTo(x + w * 0.25f, b.getBottom() - 4.0f, mx - w * 0.1f, my, mx, my);
        p.cubicTo(mx + w * 0.1f, my, x + w * 0.75f, b.getBottom() - 4.0f, x + w, b.getBottom() - 4.0f);
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawIconGrain(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        int count = (var == 0) ? 8 : 16;
        float spread = (var == 0) ? 0.35f : 0.55f;
        juce::Random rng(42);
        g.setColour(c);
        for (int i = 0; i < count; ++i)
        {
            float rx = cx + (rng.nextFloat() - 0.5f) * b.getWidth() * spread;
            float ry = cy + (rng.nextFloat() - 0.5f) * b.getHeight() * spread;
            float r = 1.0f + rng.nextFloat() * 1.5f;
            g.fillEllipse(rx - r, ry - r, r * 2.0f, r * 2.0f);
        }
    }

    static void drawIconBitcrush(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        juce::Path p;
        float x = b.getX() + 4.0f;
        float w = b.getWidth() - 8.0f;
        float yBase = b.getBottom() - 5.0f;
        float yTop = b.getY() + 5.0f;
        int steps = (var == 0) ? 6 : 12;
        p.startNewSubPath(x, yBase);
        for (int i = 0; i < steps; ++i)
        {
            float nx = x + (i + 1) * (w / steps);
            float ny = ((i % 2) == 0) ? yTop : yBase;
            p.lineTo(nx - w / (steps * 2.0f), p.getCurrentPosition().getY());
            p.lineTo(nx - w / (steps * 2.0f), ny);
            p.lineTo(nx, ny);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(1.8f));
    }

    static void drawIconPitch(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        juce::Path p;
        if (var == 0)
        {
            p.addTriangle(cx, cy - 10.0f, cx - 7.0f, cy + 5.0f, cx + 7.0f, cy + 5.0f);
        }
        else
        {
            p.addTriangle(cx, cy + 10.0f, cx - 7.0f, cy - 5.0f, cx + 7.0f, cy - 5.0f);
        }
        g.setColour(c);
        g.fillPath(p);
    }

    static void drawIconVinyl(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.28f;
        g.setColour(c);
        g.drawEllipse(cx - r, cy - r * 0.5f, r * 2.0f, r, 2.0f);
        if (var == 0)
        {
            g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
            g.drawLine(cx + r * 0.3f, cy - r * 0.6f, cx + r * 0.8f, cy - r * 1.1f, 2.0f);
        }
        else
        {
            juce::Path scratch;
            scratch.startNewSubPath(cx - r * 0.5f, cy + r * 0.3f);
            scratch.cubicTo(cx, cy + r * 0.5f, cx + r * 0.3f, cy - r * 0.3f, cx + r * 0.5f, cy + r * 0.3f);
            g.strokePath(scratch, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    static void drawIconStretch(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        juce::Path p;
        float x = b.getX() + 4.0f;
        float w = b.getWidth() - 8.0f;
        float cy = b.getCentreY();
        float amp = (var == 0) ? 4.0f : 8.0f;
        float freq = (var == 0) ? 1.2f : 0.6f;
        p.startNewSubPath(x, cy);
        for (int i = 1; i <= 16; ++i)
        {
            float nx = x + i * (w / 16.0f);
            float ny = cy + std::sin(i * freq) * amp;
            p.lineTo(nx, ny);
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawIconRingMod(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.18f;
        juce::Path ring;
        ring.addCentredArc(cx, cy, r, r, 0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
        g.setColour(c.withAlpha(0.35f));
        g.strokePath(ring, juce::PathStrokeType(2.5f));
        juce::Path sine;
        float freq = (var == 0) ? 0.4f : 0.9f;
        float amp = (var == 0) ? 5.0f : 8.0f;
        sine.startNewSubPath(cx - 10.0f, cy);
        for (int i = 0; i <= 20; ++i)
        {
            float nx = cx - 10.0f + i;
            float ny = cy + std::sin(i * freq) * amp;
            sine.lineTo(nx, ny);
        }
        g.setColour(c);
        g.strokePath(sine, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawIconTonalizer(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        int bars = (var == 0) ? 4 : 7;
        float totalW = b.getWidth() * 0.7f;
        float barW = totalW / (bars * 1.5f);
        float gap = barW * 0.5f;
        float x0 = cx - (bars * barW + (bars - 1) * gap) * 0.5f;
        for (int i = 0; i < bars; ++i)
        {
            float h = b.getHeight() * (0.3f + 0.5f * ((i + var * 2) % 3) / 2.0f);
            float bx = x0 + i * (barW + gap);
            g.setColour(c.withAlpha(0.7f + 0.3f * ((i + var) % 2)));
            g.fillRect(bx, cy - h * 0.5f, barW, h);
        }
    }

    static void drawIconChaos(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        juce::Path p;
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        int points = (var == 0) ? 5 : 8;
        float r = b.getWidth() * 0.28f;
        for (int i = 0; i < points; ++i)
        {
            float angle = float(i) * juce::MathConstants<float>::twoPi / points + 0.1f;
            float rr = (i % 2 == 0) ? r : r * 0.5f;
            float x = cx + std::cos(angle) * rr;
            float y = cy + std::sin(angle) * rr * 0.7f;
            if (i == 0) p.startNewSubPath(x, y);
            else p.lineTo(x, y);
        }
        p.closeSubPath();
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void drawIconSpace(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, int var = 0)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        int rings = (var == 0) ? 4 : 3;
        for (int i = rings; i >= 1; --i)
        {
            float r = b.getWidth() * 0.08f * i;
            g.setColour(c.withAlpha(0.15f + 0.15f * i));
            g.fillEllipse(cx - r, cy - r * 0.5f, r * 2.0f, r);
        }
        juce::Path sparkle;
        sparkle.addStar(juce::Point<float>(cx - 4.0f, cy - 2.0f), 4, 2.5f, 5.0f, 0.0f);
        g.setColour(c.brighter(0.4f));
        g.fillPath(sparkle);
        if (var == 1)
        {
            juce::Path sp2;
            sp2.addStar(juce::Point<float>(cx + 5.0f, cy + 3.0f), 4, 1.5f, 3.0f, 0.5f);
            g.fillPath(sp2);
        }
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
        g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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

}
