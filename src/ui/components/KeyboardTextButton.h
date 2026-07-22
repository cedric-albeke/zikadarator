#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace zikada {

inline bool isHostTransportKey(const juce::KeyPress& key)
{
    return key.getKeyCode() == juce::KeyPress::spaceKey;
}

template <typename Control>
class HostTransportSafeControl : public Control
{
public:
    using Control::Control;
    HostTransportSafeControl() = default;

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (isHostTransportKey(key))
            return false;

        return Control::keyPressed(key);
    }
};

using HostSafeToggleButton = HostTransportSafeControl<juce::ToggleButton>;
using HostSafeTextEditor = HostTransportSafeControl<juce::TextEditor>;
using HostSafeComboBox = HostTransportSafeControl<juce::ComboBox>;
using HostSafeSlider = HostTransportSafeControl<juce::Slider>;
using HostSafeListBox = HostTransportSafeControl<juce::ListBox>;

class KeyboardTextButton : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (isHostTransportKey(key))
            return false;

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
