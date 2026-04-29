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
    ZikadaLookAndFeel::drawDeviceDisplay(g, fullBounds);

    auto bounds = fullBounds.toFloat().reduced(7.0f);
    const auto laneGap = juce::jmax(3.0f, bounds.getHeight() * 0.055f);
    auto inputBounds = bounds.removeFromTop((bounds.getHeight() - laneGap) * 0.5f);
    bounds.removeFromTop(laneGap);
    auto outputBounds = bounds;

    g.setColour(Colours::white10.withAlpha(0.35f));
    g.drawLine(inputBounds.getX(), inputBounds.getBottom() + laneGap * 0.5f,
               inputBounds.getRight(), inputBounds.getBottom() + laneGap * 0.5f, 1.0f);

    drawWaveLane(g, inputBounds, inputLane, Colours::laneSlice, 0.20f);
    drawWaveLane(g, outputBounds, outputLane, Colours::waveform, 0.26f);

    for (int i = 1; i < numSlices; ++i)
    {
        auto x = inputBounds.getX() + (static_cast<float>(i) / static_cast<float>(numSlices)) * inputBounds.getWidth();
        int lane = (i - 1) % 6;
        auto laneCol = laneInfos[lane].colour.withAlpha(0.12f);
        g.setColour(laneCol);
        g.drawLine(x, inputBounds.getY() + 1.0f, x, outputBounds.getBottom() - 1.0f, 1.0f);
    }

    const float playheadX = inputBounds.getX() + playheadPos.load() * inputBounds.getWidth();
    const float fullCentreY = fullBounds.toFloat().getCentreY();

    juce::ColourGradient playheadGlow(
        Colours::neonGreen.withAlpha(0.30f), playheadX, fullCentreY,
        Colours::neonGreen.withAlpha(0.0f), playheadX - 18.0f, fullCentreY, true);
    g.setGradientFill(playheadGlow);
    g.fillRect(playheadX - 18.0f, inputBounds.getY(), 18.0f, outputBounds.getBottom() - inputBounds.getY());
    playheadGlow.point1 = juce::Point<float>(playheadX, fullCentreY);
    playheadGlow.point2 = juce::Point<float>(playheadX + 18.0f, fullCentreY);
    g.setGradientFill(playheadGlow);
    g.fillRect(playheadX, inputBounds.getY(), 18.0f, outputBounds.getBottom() - inputBounds.getY());

    g.setColour(Colours::neonGreen);
    g.drawLine(playheadX, inputBounds.getY(), playheadX, outputBounds.getBottom(), 2.2f);

    g.setColour(Colours::neonGreen.withAlpha(0.4f));
    g.drawLine(playheadX - 3.0f, inputBounds.getY(), playheadX - 3.0f, outputBounds.getBottom(), 1.0f);
    g.drawLine(playheadX + 3.0f, inputBounds.getY(), playheadX + 3.0f, outputBounds.getBottom(), 1.0f);

    g.setColour(Colours::neonGreen.brighter(0.3f));
    g.fillEllipse(playheadX - 3.4f, outputBounds.getCentreY() - 3.4f, 6.8f, 6.8f);
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
    playheadPos = juce::jlimit(0.0f, 1.0f, normalizedPosition);
    needsRepaint = true;
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

    const int visibleSamples = lane.samplesAvailable;
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
    g.setColour(Colours::displayBezel.withAlpha(0.46f));
    g.fillRoundedRectangle(bounds, 3.0f);

    const float centreY = bounds.getCentreY();
    const float amplitude = bounds.getHeight() * 0.46f;

    g.setColour(Colours::white10.withAlpha(0.55f));
    g.drawLine(bounds.getX(), centreY, bounds.getRight(), centreY, 1.0f);

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

    g.setColour(colour.withAlpha(0.82f));
    g.strokePath(waveform, juce::PathStrokeType(1.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(colour.withAlpha(0.38f));
    for (int i = 0; i < displayBinCount; i += 6)
    {
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(displayBinCount - 1)) * bounds.getWidth();
        const float y1 = centreY - juce::jlimit(-1.0f, 1.0f, lane.displayMaxes[static_cast<size_t>(i)] * lane.displayGain) * amplitude;
        const float y2 = centreY - juce::jlimit(-1.0f, 1.0f, lane.displayMins[static_cast<size_t>(i)] * lane.displayGain) * amplitude;
        g.drawLine(x, y1, x, y2, 1.0f);
    }
}

}
