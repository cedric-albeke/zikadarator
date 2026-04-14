# ZIKADARATOR

A 2026-worthy VST FX plugin inspired by Sugarbytes Looperator, built for the Zikada brand identity.

## Concept

**"Sequence the signal."**

ZIKADARATOR is a 16-step multi-FX sequencer that transforms incoming audio in real-time. Chop, stutter, filter, and reshape loops through 6 independent lanes, each with per-step preset assignment, footer-based user-slot editing, and an integrated preset/settings workspace.

## Features

- **16-step FX sequencer** with 6 reorderable lanes
- **Right-sidebar preset grid** for per-step assignment
- **Footer detail dock** for U1-U4 slot editing and modulation
- **Preset browser + Settings tabs** embedded directly in the main editor
- **Header preset dropdown** with favorites, recents, and direct loading
- **Standalone device settings in-tab** — audio device, sample rate, buffer size, and MIDI inputs now live inside the Settings page
- **Zikada-native UI**: Neon green (`#00FF85`) on deep teal-black, VCR OSD Mono typography, vector-based custom components
- **VST3 / AU / Standalone** formats

## Tech Stack

- **Framework**: JUCE 8.x
- **Build System**: CMake 3.22+
- **Language**: C++20
- **UI**: Custom JUCE components (vector-based, zero stock LookAndFeel)

## Project Structure

```
src/
├── PluginProcessor.cpp/.h      # Main audio processor
├── PluginEditor.cpp/.h         # Main editor component
├── engine/                     # Audio engine
│   ├── SequencerEngine.cpp/.h  # 16-step sequencer core
│   ├── SliceEngine.cpp/.h
│   ├── FilterEngine.cpp/.h
│   ├── DelayEngine.cpp/.h
│   ├── ReverbEngine.cpp/.h
│   ├── BitcrushEngine.cpp/.h
│   ├── PitchEngine.cpp/.h
│   ├── ModulationEngine.cpp/.h
│   └── GainPanEngine.cpp/.h
├── state/                      # Parameter / state management
│   ├── PluginState.cpp/.h
│   ├── ParameterIDs.h
│   ├── SequencerState.h
│   └── PresetManager.cpp/.h
└── ui/                         # User interface
    ├── ZikadaLookAndFeel.cpp/.h
    ├── components/
    │   ├── VcrLabel.cpp/.h
    │   ├── StepCell.cpp/.h
    │   ├── StepGrid.cpp/.h
    │   ├── Knob.cpp/.h
    │   └── WaveformDisplay.cpp/.h
    └── panels/
        ├── HeaderPanel.cpp/.h
        ├── SequencerPanel.cpp/.h
        ├── FooterPanel.cpp/.h
        ├── SidebarPanel.cpp/.h
        └── WorkspacePanel.cpp/.h
```

## Current Editor Layout

- **Header**: Serum-inspired tab rail, in-header preset strip, undo/redo
- **Header preset strip**: popup dropdown for favorites, recent presets, full preset list, and browser access
- **Sequencer page**: Signal monitor, 6-lane step grid, right preset sidebar, footer detail dock
- **Presets page**: In-app browser for factory and user presets
- **Settings page**: Embedded JUCE standalone Audio/MIDI device selector

## Building

```bash
cmake -B build
cmake --build build --target ZikadaFX_Standalone
```

## Design System

See [`docs/DESIGN_SYSTEM.md`](docs/DESIGN_SYSTEM.md) for exact brand tokens: colors, typography, spacing, and component specifications.

## Architecture

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the full technical architecture.

## Product Spec

See [`docs/PRODUCT_SPEC.md`](docs/PRODUCT_SPEC.md) for feature requirements and roadmap.

## License

Copyright (c) Zikada. All rights reserved.
