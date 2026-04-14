# Zikada FX

A 2026-worthy VST FX plugin inspired by Sugarbytes Looperator, built for the Zikada brand identity.

## Concept

**"Sequence the signal."**

Zikada FX is a 16-step multi-FX sequencer that transforms incoming audio in real-time. Chop, stutter, filter, and reshape loops through 6 independent, reorderable effect lanes — each with 20+ preset variants per step.

## Features

- **16-step FX sequencer** with 6 reorderable lanes
- **Per-step sub-presets** — each step selects *which variant* of an effect to apply
- **3-layer depth model**: Step grid → User slots (U1-U4) → Per-parameter modulation
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
│   └── SequencerEngine.cpp/.h  # 16-step sequencer core
├── state/                      # Parameter / state management
│   ├── PluginState.cpp/.h
│   └── ParameterIDs.h
└── ui/                         # User interface
    ├── ZikadaLookAndFeel.cpp/.h
    ├── components/
    │   ├── VcrLabel.cpp/.h
    │   ├── StepCell.cpp/.h
    │   └── StepGrid.cpp/.h
    └── panels/
        ├── HeaderPanel.cpp/.h
        ├── SequencerPanel.cpp/.h
        └── FooterPanel.cpp/.h
```

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
