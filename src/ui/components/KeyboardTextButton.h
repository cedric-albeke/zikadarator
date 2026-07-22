#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace zikada {

class KeyboardTextButton : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key.getKeyCode() == juce::KeyPress::spaceKey && isEnabled())
        {
            internalClickCallback(juce::ModifierKeys::currentModifiers);
            return true;
        }

        return juce::TextButton::keyPressed(key);
    }

    void focusGained(juce::Component::FocusChangeType cause) override
    {
        juce::TextButton::focusGained(cause);
        if (onKeyboardFocus)
            onKeyboardFocus();
    }

    std::function<void()> onKeyboardFocus;
};

} // namespace zikada
