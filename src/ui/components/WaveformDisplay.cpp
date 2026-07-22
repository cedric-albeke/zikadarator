#include "ui/components/WaveformDisplay.h"

#include <cmath>

namespace zikada {

WaveformDisplay::WaveformDisplay()
{
    startTimerHz(30);
}

WaveformDisplay::~WaveformDisplay()
{
    stopTimer();
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bgPrimary);

    auto fullBounds = getLocalBounds();
    auto bounds = fullBounds.toFloat().reduced(7.0f);
    const auto laneGap = juce::jmax(3.0f, bounds.getHeight() * 0.055f);
    auto inputBounds = bounds.removeFromTop((bounds.getHeight() - laneGap) * 0.5f);
    bounds.removeFromTop(laneGap);
    auto outputBounds = bounds;

    // Divider between input and output lanes
    g.setColour(Colours::white10.withAlpha(0.35f));
    g.drawLine(inputBounds.getX(), inputBounds.getBottom() + laneGap * 0.5f,
               inputBounds.getRight(), inputBounds.getBottom() + laneGap * 0.5f, 1.0f);

    // Draw grid: step boundaries with lane colors, beat markers stronger
    for (int i = 1; i < numSlices; ++i)
    {
        auto x = inputBounds.getX() + (static_cast<float>(i) / static_cast<float>(numSlices)) * inputBounds.getWidth();
        int lane = (i - 1) % 6;
        auto laneCol = laneInfos[lane].colour;
        
        // Beat markers (every 4 steps) are stronger
        bool isBeat = (i % 4 == 0);
        float alpha = isBeat ? 0.22f : 0.10f;
        float lineWidth = isBeat ? 1.2f : 0.8f;
        
        g.setColour(laneCol.withAlpha(alpha));
        g.drawLine(x, inputBounds.getY() + 1.0f, x, outputBounds.getBottom() - 1.0f, lineWidth);
    }

    // Draw step numbers at bottom of output lane
    const auto* laf = dynamic_cast<const ZikadaLookAndFeel*>(&getLookAndFeel());
    for (int i = 0; i < numSlices; ++i)
    {
        if (i % 4 != 0) continue; // Only show beat numbers
        auto x = inputBounds.getX() + (static_cast<float>(i) / static_cast<float>(numSlices)) * inputBounds.getWidth();
        g.setColour(Colours::white.withAlpha(0.35f));
        g.setFont(laf != nullptr ? laf->getSpaceMonoFont(9.0f)
                                 : juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.drawText(juce::String(i + 1),
                   juce::Rectangle<int>(static_cast<int>(x) - 6, 
                                        static_cast<int>(outputBounds.getBottom()) - 12, 12, 10),
                   juce::Justification::centred, false);
    }

    drawWaveLane(g, inputBounds, inputLane, Colours::laneSlice, 0.22f);
    drawWaveLane(g, outputBounds, outputLane, Colours::waveform, 0.28f);

    // Playhead rendering
    const float playheadX = inputBounds.getX() + playheadPos.load() * inputBounds.getWidth();
    const float fullCentreY = fullBounds.toFloat().getCentreY();

    // Playhead glow (wider, more diffuse)
    juce::ColourGradient playheadGlow(
        Colours::neonGreen.withAlpha(0.35f), playheadX, fullCentreY,
        Colours::neonGreen.withAlpha(0.0f), playheadX - 22.0f, fullCentreY, true);
    g.setGradientFill(playheadGlow);
    g.fillRect(playheadX - 22.0f, inputBounds.getY(), 22.0f, outputBounds.getBottom() - inputBounds.getY());
    playheadGlow.point1 = juce::Point<float>(playheadX, fullCentreY);
    playheadGlow.point2 = juce::Point<float>(playheadX + 22.0f, fullCentreY);
    g.setGradientFill(playheadGlow);
    g.fillRect(playheadX, inputBounds.getY(), 22.0f, outputBounds.getBottom() - inputBounds.getY());

    // Playhead line
    g.setColour(Colours::neonGreen);
    g.drawLine(playheadX, inputBounds.getY(), playheadX, outputBounds.getBottom(), 2.5f);

    // Playhead shadow lines
    g.setColour(Colours::neonGreen.withAlpha(0.45f));
    g.drawLine(playheadX - 3.0f, inputBounds.getY(), playheadX - 3.0f, outputBounds.getBottom(), 1.0f);
    g.drawLine(playheadX + 3.0f, inputBounds.getY(), playheadX + 3.0f, outputBounds.getBottom(), 1.0f);

    // Playhead dot at center
    g.setColour(Colours::neonGreen.brighter(0.3f));
    g.fillEllipse(playheadX - 3.5f, outputBounds.getCentreY() - 3.5f, 7.0f, 7.0f);
}

void WaveformDisplay::resized()
{
}

void WaveformDisplay::pushSamples(const float* samples, int numSamples)
{
    pushInputSamples(samples, numSamples);
}

void WaveformDisplay::pushInputSamples(const float* samples, int numSamples)
{
    pushLaneSamples(inputLane, samples, numSamples);
}

void WaveformDisplay::pushOutputSamples(const float* samples, int numSamples)
{
    pushLaneSamples(outputLane, samples, numSamples);
}

void WaveformDisplay::pushLaneSamples(WaveLane& lane, const float* samples, int numSamples)
{
    if (samples == nullptr || numSamples <= 0)
        return;

    if (numSamples > historySampleCount)
    {
        samples += numSamples - historySampleCount;
        numSamples = historySampleCount;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        lane.historyData[static_cast<size_t>(lane.writePosition)] = samples[i];
        lane.writePosition = (lane.writePosition + 1) % historySampleCount;
        lane.samplesAvailable = juce::jmin(historySampleCount, lane.samplesAvailable + 1);
    }

    rebuildDisplayBins(lane);
    needsRepaint = true;
}

void WaveformDisplay::setPlayheadPosition(float normalizedPosition)
{
    const float nextPosition = juce::jlimit(0.0f, 1.0f, normalizedPosition);
    if (playheadPos.load() == nextPosition)
        return;

    playheadPos = nextPosition;
    needsRepaint = true;
}

void WaveformDisplay::setVisibleSampleCount(int sampleCount)
{
    const int nextVisibleSampleCount = juce::jlimit(displayBinCount, historySampleCount, sampleCount);
    if (visibleSampleCount == nextVisibleSampleCount)
        return;

    visibleSampleCount = nextVisibleSampleCount;
    rebuildDisplayBins(inputLane);
    rebuildDisplayBins(outputLane);
    needsRepaint = true;
}

void WaveformDisplay::clearHistory()
{
    resetLane(inputLane);
    resetLane(outputLane);
    playheadPos = 0.0f;
    needsRepaint = true;
}

bool WaveformDisplay::hasRetainedSamples() const noexcept
{
    return inputLane.samplesAvailable > 0 || outputLane.samplesAvailable > 0;
}

void WaveformDisplay::resetLane(WaveLane& lane)
{
    lane.displayMins.fill(0.0f);
    lane.displayMaxes.fill(0.0f);
    lane.writePosition = 0;
    lane.samplesAvailable = 0;
    lane.displayGain = 1.0f;
}

void WaveformDisplay::timerCallback()
{
    if (!needsRepaint.exchange(false))
        return;

    repaint();
}

void WaveformDisplay::rebuildDisplayBins(WaveLane& lane)
{
    if (lane.samplesAvailable <= 0)
    {
        lane.displayMins.fill(0.0f);
        lane.displayMaxes.fill(0.0f);
        return;
    }

    const int visibleSamples = juce::jmin(lane.samplesAvailable, visibleSampleCount);
    const int oldestIndex = (lane.writePosition - visibleSamples + historySampleCount) % historySampleCount;
    float peak = 0.0f;

    for (int bin = 0; bin < displayBinCount; ++bin)
    {
        float minValue = 0.0f;
        float maxValue = 0.0f;
        const int sampleStart = (bin * visibleSamples) / displayBinCount;
        const int sampleEnd = juce::jmax(sampleStart + 1, ((bin + 1) * visibleSamples) / displayBinCount);

        for (int s = sampleStart; s < sampleEnd; ++s)
        {
            const int index = (oldestIndex + s) % historySampleCount;
            const float sample = lane.historyData[static_cast<size_t>(index)];
            minValue = juce::jmin(minValue, sample);
            maxValue = juce::jmax(maxValue, sample);
        }

        lane.displayMins[static_cast<size_t>(bin)] = juce::jlimit(-1.0f, 1.0f, minValue);
        lane.displayMaxes[static_cast<size_t>(bin)] = juce::jlimit(-1.0f, 1.0f, maxValue);
        peak = juce::jmax(peak,
                          std::abs(lane.displayMins[static_cast<size_t>(bin)]),
                          std::abs(lane.displayMaxes[static_cast<size_t>(bin)]));
    }

    const float targetGain = peak > 0.001f
        ? juce::jlimit(1.0f, 10.0f, 0.82f / peak)
        : 1.0f;
    lane.displayGain = lane.displayGain * 0.84f + targetGain * 0.16f;
}

void WaveformDisplay::drawWaveLane(juce::Graphics& g,
                                   juce::Rectangle<float> bounds,
                                   const WaveLane& lane,
                                   juce::Colour colour,
                                   float fillAlpha)
{
    g.setColour(Colours::displayBezel.withAlpha(0.50f));
    g.fillRoundedRectangle(bounds, 3.0f);

    const float centreY = bounds.getCentreY();
    const float amplitude = bounds.getHeight() * 0.46f;

    // Horizontal grid lines (center + quarter marks)
    g.setColour(Colours::white10.withAlpha(0.35f));
    g.drawLine(bounds.getX(), centreY, bounds.getRight(), centreY, 1.0f);
    g.setColour(Colours::white10.withAlpha(0.15f));
    g.drawLine(bounds.getX(), centreY - amplitude * 0.5f, bounds.getRight(), centreY - amplitude * 0.5f, 0.5f);
    g.drawLine(bounds.getX(), centreY + amplitude * 0.5f, bounds.getRight(), centreY + amplitude * 0.5f, 0.5f);

    juce::Path waveform;
    waveform.startNewSubPath(bounds.getX(), centreY);

    for (int i = 0; i < displayBinCount; ++i)
    {
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(displayBinCount - 1)) * bounds.getWidth();
        const float y = centreY - juce::jlimit(-1.0f, 1.0f, lane.displayMaxes[static_cast<size_t>(i)] * lane.displayGain) * amplitude;
        waveform.lineTo(x, y);
    }

    for (int i = displayBinCount - 1; i >= 0; --i)
    {
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(displayBinCount - 1)) * bounds.getWidth();
        const float y = centreY - juce::jlimit(-1.0f, 1.0f, lane.displayMins[static_cast<size_t>(i)] * lane.displayGain) * amplitude;
        waveform.lineTo(x, y);
    }

    waveform.closeSubPath();
    g.setColour(colour.withAlpha(fillAlpha));
    g.fillPath(waveform);

    g.setColour(colour.withAlpha(0.88f));
    g.strokePath(waveform, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Vertical tick marks at every 6th bin
    g.setColour(colour.withAlpha(0.42f));
    for (int i = 0; i < displayBinCount; i += 6)
    {
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(displayBinCount - 1)) * bounds.getWidth();
        const float y1 = centreY - juce::jlimit(-1.0f, 1.0f, lane.displayMaxes[static_cast<size_t>(i)] * lane.displayGain) * amplitude;
        const float y2 = centreY - juce::jlimit(-1.0f, 1.0f, lane.displayMins[static_cast<size_t>(i)] * lane.displayGain) * amplitude;
        g.drawLine(x, y1, x, y2, 1.0f);
    }
}

}
