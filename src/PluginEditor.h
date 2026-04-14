#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/ZikadaLookAndFeel.h"
#include "ui/panels/HeaderPanel.h"
#include "ui/panels/SequencerPanel.h"
#include "ui/panels/FooterPanel.h"
#include "ui/components/WaveformDisplay.h"

namespace zikada {

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    WaveformDisplay* getWaveformDisplay();

private:
    PluginProcessor& processorRef;
    ZikadaLookAndFeel lookAndFeel;
    HeaderPanel headerPanel;
    SequencerPanel sequencerPanel;
    FooterPanel footerPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

}

