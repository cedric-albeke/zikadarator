#pragma once

#include "../ZikadaLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zikada {

class HeaderPanel : public juce::Component
{
public:
    HeaderPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Image logoImage;
    juce::TextButton presetButton{"PRESET"};
    juce::TextButton undoButton{"UNDO"};
    juce::TextButton redoButton{"REDO"};
    juce::TextButton randomButton{"RANDOM"};
};

}
