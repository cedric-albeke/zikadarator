cmake_minimum_required(VERSION 3.22)
project(ZikadaFX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Binary data target for fonts and images
juce_add_binary_data(ZikadaFXAssets SOURCES
    assets/fonts/SpaceMono-Regular.ttf
    assets/fonts/SpaceMono-Bold.ttf
    assets/fonts/Anta-Regular.ttf
    assets/fonts/VCR-OSD-MONO.ttf
    assets/images/zikada-cicada.png
    assets/images/zikada-cicada-128.png
)

set_target_properties(ZikadaFXAssets PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
)

# Plugin target
juce_add_plugin(ZikadaFX
    COMPANY_NAME "Zikada"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Zika
    PLUGIN_CODE Zkfx
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Zikada FX"
    VERSION "0.1.0"
    BUNDLE_ID "io.zikada.fx"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Zikada FX needs microphone access for audio input"
)

target_sources(ZikadaFX
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/ui/ZikadaLookAndFeel.cpp
        src/ui/components/VcrLabel.cpp
        src/ui/components/StepCell.cpp
        src/ui/components/StepGrid.cpp
        src/ui/components/Knob.cpp
        src/ui/components/WaveformDisplay.cpp
        src/ui/panels/HeaderPanel.cpp
        src/ui/panels/SequencerPanel.cpp
        src/ui/panels/FooterPanel.cpp
        src/state/PluginState.cpp
        src/engine/SequencerEngine.cpp
        src/engine/CircularAudioBuffer.cpp
        src/engine/SliceEngine.cpp
        src/engine/FilterEngine.cpp
        src/engine/DelayEngine.cpp
        src/engine/ReverbEngine.cpp
        src/engine/BitcrushEngine.cpp
        src/engine/PitchEngine.cpp
)

target_include_directories(ZikadaFX
    PRIVATE
        src
        src/ui
        src/ui/components
        src/ui/panels
        src/state
        src/engine
)

target_link_libraries(ZikadaFX
    PRIVATE
        ZikadaFXAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Enable all warnings
target_compile_options(ZikadaFX PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Linux: Add GTK3, WebKit includes and link libraries
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(WEBKIT2 REQUIRED webkit2gtk-4.1)
    target_include_directories(ZikadaFX PRIVATE ${GTK3_INCLUDE_DIRS} ${WEBKIT2_INCLUDE_DIRS})
    target_link_libraries(ZikadaFX PRIVATE curl)
endif()
