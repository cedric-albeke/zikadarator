#pragma once

#include "../ZikadaLookAndFeel.h"
#include "../components/VcrLabel.h"

namespace zikada {

class HeaderPanel : public juce::Component
{
public:
    HeaderPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    VcrLabel titleLabel;
    juce::TextButton playButton{"PLAY"};
    juce::TextButton presetButton{"PRESET"};
    juce::TextButton undoButton{"UNDO"};
    juce::TextButton redoButton{"REDO"};
    juce::TextButton randomButton{"RANDOM"};
};

}
