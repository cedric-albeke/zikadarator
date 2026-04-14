#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/ZikadaLookAndFeel.h"
#include "ui/panels/HeaderPanel.h"
#include "ui/panels/SequencerPanel.h"
#include "ui/panels/FooterPanel.h"
#include "ui/components/WaveformDisplay.h"

namespace zikada {

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    WaveformDisplay* getWaveformDisplay();

private:
    void timerCallback() override;

    PluginProcessor& processorRef;
    ZikadaLookAndFeel lookAndFeel;
    HeaderPanel headerPanel;
    SequencerPanel sequencerPanel;
    FooterPanel footerPanel;

    int lastPlayingStep{-1};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

}

