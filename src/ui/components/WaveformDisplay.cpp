#include "ui/components/WaveformDisplay.h"

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

    auto bounds = fullBounds.toFloat().reduced(6.0f);
    auto centreY = bounds.getCentreY();
    auto height = bounds.getHeight() * 0.72f;

    juce::Path waveform;
    waveform.startNewSubPath(bounds.getX(), centreY);

    for (int i = 0; i < numDisplayBins; ++i)
    {
        auto x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numDisplayBins - 1)) * bounds.getWidth();
        auto y = centreY - displayPeaks[i] * height * 0.5f;
        waveform.lineTo(x, y);
    }

    for (int i = numDisplayBins - 1; i >= 0; --i)
    {
        auto x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numDisplayBins - 1)) * bounds.getWidth();
        auto y = centreY + displayPeaks[i] * height * 0.5f;
        waveform.lineTo(x, y);
    }

    waveform.closeSubPath();
    g.setColour(Colours::waveform.withAlpha(0.25f));
    g.fillPath(waveform);

    g.setColour(Colours::waveform);
    g.strokePath(waveform, juce::PathStrokeType(1.5f));

    for (int i = 1; i < numSlices; ++i)
    {
        auto x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numSlices)) * bounds.getWidth();
        int lane = (i - 1) % 6;
        auto laneCol = laneInfos[lane].colour.withAlpha(0.12f);
        g.setColour(laneCol);
        g.drawLine(x, bounds.getY() + 2.0f, x, bounds.getBottom() - 2.0f, 1.0f);
    }

    auto playheadX = bounds.getX() + playheadPos.load() * bounds.getWidth();

    juce::ColourGradient playheadGlow(
        Colours::neonGreen.withAlpha(0.35f), playheadX, centreY,
        Colours::neonGreen.withAlpha(0.0f), playheadX - 18.0f, centreY, true);
    g.setGradientFill(playheadGlow);
    g.fillRect(playheadX - 18.0f, bounds.getY(), 18.0f, bounds.getHeight());
    playheadGlow.point1 = juce::Point<float>(playheadX, centreY);
    playheadGlow.point2 = juce::Point<float>(playheadX + 18.0f, centreY);
    g.setGradientFill(playheadGlow);
    g.fillRect(playheadX, bounds.getY(), 18.0f, bounds.getHeight());

    g.setColour(Colours::neonGreen);
    g.drawLine(playheadX, bounds.getY(), playheadX, bounds.getBottom(), 2.5f);

    g.setColour(Colours::neonGreen.withAlpha(0.4f));
    g.drawLine(playheadX - 3.0f, bounds.getY(), playheadX - 3.0f, bounds.getBottom(), 1.0f);
    g.drawLine(playheadX + 3.0f, bounds.getY(), playheadX + 3.0f, bounds.getBottom(), 1.0f);

    float headY = centreY;
    g.setColour(Colours::neonGreen.brighter(0.3f));
    g.fillEllipse(playheadX - 3.5f, headY - 3.5f, 7.0f, 7.0f);
}

void WaveformDisplay::resized()
{
}

void WaveformDisplay::pushSamples(const float* samples, int numSamples)
{
    auto writeHandle = fifo.write(numSamples);
    
    if (writeHandle.blockSize1 > 0)
    {
        for (int i = 0; i < writeHandle.blockSize1; ++i)
            audioData[writeHandle.startIndex1 + i] = samples[i];
    }
    
    if (writeHandle.blockSize2 > 0)
    {
        for (int i = 0; i < writeHandle.blockSize2; ++i)
            audioData[writeHandle.startIndex2 + i] = samples[i + writeHandle.blockSize1];
    }
    
    needsRepaint = true;
}

void WaveformDisplay::setPlayheadPosition(float normalizedPosition)
{
    playheadPos = juce::jlimit(0.0f, 1.0f, normalizedPosition);
}

void WaveformDisplay::timerCallback()
{
    if (!needsRepaint)
        return;
    
    auto readHandle = fifo.read(fifo.getNumReady());
    if (readHandle.blockSize1 + readHandle.blockSize2 == 0)
        return;
    
    auto totalSamples = readHandle.blockSize1 + readHandle.blockSize2;
    auto samplesPerBin = juce::jmax(1, totalSamples / numDisplayBins);
    
    for (int bin = 0; bin < numDisplayBins; ++bin)
    {
        float peak = 0.0f;
        int sampleStart = bin * samplesPerBin;
        int sampleEnd = juce::jmin(totalSamples, (bin + 1) * samplesPerBin);
        
        for (int s = sampleStart; s < sampleEnd; ++s)
        {
            float sample = (s < readHandle.blockSize1)
                ? audioData[readHandle.startIndex1 + s]
                : audioData[readHandle.startIndex2 + (s - readHandle.blockSize1)];
            peak = juce::jmax(peak, std::abs(sample));
        }
        
        displayPeaks[bin] = displayPeaks[bin] * 0.7f + peak * 0.3f;
    }
    
    needsRepaint = false;
    repaint();
}

}
