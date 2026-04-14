# Zikada FX VST — Project Architecture

## Overview
A 2026-worthy VST FX plugin inspired by Sugarbytes Looperator, built for the Zikada brand identity. A 16-step multi-FX sequencer with an enterprise-grade custom vector UI.

## Target Formats
- VST3 (primary)
- AudioUnit (macOS)
- AAX (Pro Tools)
- Standalone app

## Tech Stack
- **Framework**: JUCE 8.x (fetched via CMake FetchContent)
- **Build System**: CMake 3.22+
- **Language**: C++20
- **UI**: Custom JUCE components (vector-based, no stock LookAndFeel)

---

## Directory Structure

```
/home/zady/Development/zikada-fx-vst/
├── CMakeLists.txt              # Root CMake configuration
├── cmake/
│   └── JUCE.cmake              # JUCE FetchContent wrapper
├── docs/
│   ├── ARCHITECTURE.md         # This file
│   ├── DESIGN_SYSTEM.md        # Zikada brand tokens
│   └── PRODUCT_SPEC.md         # Feature specification
├── resources/
│   └── Looperator.pdf          # Reference manual
├── src/
│   ├── PluginProcessor.cpp/.h  # Main audio processor
│   ├── PluginEditor.cpp/.h     # Main editor component
│   ├── engine/                 # Audio engine
│   │   ├── SequencerEngine.h   # 16-step sequencer core
│   │   ├── StepData.h          # Per-step state structures
│   │   ├── SliceEngine.h       # Audio slicing / buffer shuffle
│   │   ├── LoopEngine.h        # Stutter / loop patterns
│   │   ├── EnvelopeEngine.h    # Per-step volume envelopes
│   │   ├── FilterEngine.h      # Filter + vowel effects
│   │   ├── FxRack.h            # FX1 + FX2 multi-effect lanes
│   │   └── effects/            # Individual effect implementations
│   │       ├── DelayEffect.h
│   │       ├── ReverbEffect.h
│   │       ├── DistortionEffect.h
│   │       ├── GrainEffect.h
│   │       ├── PhaserEffect.h
│   │       ├── VinylEffect.h
│   │       └── ...
│   ├── ui/                     # User interface
│   │   ├── ZikadaLookAndFeel.h # Brand color/fonts (not LnF inheritance)
│   │   ├── components/         # Reusable UI widgets
│   │   │   ├── StepGrid.h      # 16x6 step sequencer grid
│   │   │   ├── StepCell.h      # Individual step button
│   │   │   ├── LaneHeader.h    # Lane label + controls
│   │   │   ├── EffectPopup.h   # Step effect selector popup
│   │   │   ├── UserEditor.h    # U1-U4 parameter editor
│   │   │   ├── Knob.h          # Custom rotary knob
│   │   │   ├── Fader.h         # Custom vertical fader
│   │   │   ├── WaveformDisplay.h
│   │   │   ├── VcrLabel.h      # Monospace uppercase label
│   │   │   └── GlowButton.h    # Neon glow button
│   │   ├── panels/             # Main layout panels
│   │   │   ├── HeaderPanel.h
│   │   │   ├── SequencerPanel.h
│   │   │   ├── FooterPanel.h
│   │   │   └── SettingsPanel.h
│   │   └── fonts/              # Embedded binary fonts
│   └── state/                  # Parameter / state management
│       ├── PluginState.h
│       ├── ParameterIDs.h
│       └── PresetManager.h
└── assets/                     # Images, fonts, binary data
    ├── fonts/
    │   ├── SpaceMono-Bold.ttf
    │   ├── SpaceMono-Regular.ttf
    │   ├── Anta-Regular.ttf
    │   └── VCR-OSD-MONO.ttf
    └── images/
        └── zikada-cicada.png
```

---

## Audio Engine Architecture

### Sequencer Core
- **Fixed 16 steps**, tempo-synced to host or internal clock
- **Step resolution**: 1/8, 1/4, 1/2 note (configurable, 2/4/8 bar total loops)
- **Reorderable lanes**: Users can drag lane headers to change signal flow
- **Per-lane dry/wet**: Mix control for each FX lane

### Signal Flow (Default)
```
INPUT → SLICE → LOOP → ENVELOPE → FX1 → FILTER → FX2 → MIX → OUTPUT
```

### Processing Model
- `SliceEngine` operates on a shared circular audio buffer (2+ bars)
- Each subsequent engine receives the output of the previous
- `EnvelopeEngine` applies per-step amplitude curves
- `FxRack` hosts 2 independent effect lanes (FX1, FX2)
- Final `MixEngine` blends dry/wet with multiple blend modes

### Modulation System (Layer 3)
Each user parameter can be driven by:
1. **Static** — fixed value
2. **Motion** — 20 preset curves with start/end points
3. **Env Follower** — input amplitude tracking
4. **Random** — min/max range, step/glide, subdivisions

---

## UI Architecture

### Layout (Top → Bottom)
```
┌────────────────────────────────────────────────────────────┐
│ HEADER: Transport | Preset | Undo | Random | Settings      │
├────────────────────────────────────────────────────────────┤
│ INPUT LANE: Live waveform (16-slice visualization)         │
├────────────────────────────────────────────────────────────┤
│ LANE 1: [Label] [⚙] [R] [■■■■■■■■■■■■■■■■] [Wet]          │
│ LANE 2: [Label] [⚙] [R] [■■■■■■■■■■■■■■■■] [Wet]          │
│ LANE 3: [Label] [⚙] [R] [■■■■■■■■■■■■■■■■] [Wet]          │
│ LANE 4: [Label] [⚙] [R] [■■■■■■■■■■■■■■■■] [Wet]          │
│ LANE 5: [Label] [⚙] [R] [■■■■■■■■■■■■■■■■] [Wet]          │
│ LANE 6: [Label] [⚙] [R] [■■■■■■■■■■■■■■■■] [Wet]          │
├────────────────────────────────────────────────────────────┤
│ FOOTER: Dry/Wet Mix | Blend Mode | Bypass | Output Level   │
└────────────────────────────────────────────────────────────┘
```

### Design Principles
- **Vector-based**: All UI drawn via `juce::Graphics` (no raster assets needed)
- **60fps animations**: Smooth playhead, waveform updates, parameter transitions
- **Immediate feedback**: Every interaction has a visual response within 1 frame
- **Keyboard + mouse**: Scroll wheel cycles presets, drag-to-paint, shift+tie, right-click delete
- **Resizable**: Editor scales from 75% to 200%

### Color Mapping to Lanes (Zikada Palette)
| Lane | Color | Hex |
|------|-------|-----|
| INPUT | White/neutral | `#FFFFFF` |
| SLICE | Cyan | `#00DEFF` |
| LOOP | Magenta | `#C040C0` |
| ENVELOPE | Violet | `#8A40FF` |
| FX1 | Neon Green | `#00FF85` |
| FILTER | Yellow-Green | `#BFFF00` |
| FX2 | Teal | `#00FFB3` |

---

## Parameter State

### Automated Parameters (exposed to DAW)
- Global Dry/Wet
- Global Mix Mode
- Output Gain
- Per-lane Dry/Wet (6 params)
- Per-lane Enable (6 params)
- Clock Source (Host/Free)
- Tempo (Free mode)
- Step Resolution

### Internal State (saved in presets, not automated)
- Full 16×6 step grid assignments
- All U1-U4 user configurations
- All modulation settings
- Lane order
- MIDI Learn mappings
- Randomization preferences

---

## Development Phases

1. **Phase 0 — Foundation**: CMake + JUCE + basic processor/editor skeleton
2. **Phase 1 — UI Framework**: Custom components, fonts, design system implementation
3. **Phase 2 — Sequencer Core**: Step grid, state management, basic playback
4. **Phase 3 — Audio Engine**: Slicer, looper, envelope, filter, FX rack
5. **Phase 4 — Effects**: Individual effect modules (delay, reverb, distortion, etc.)
6. **Phase 5 — Modulation**: Motion curves, env follower, randomization
7. **Phase 6 — Polish**: Preset browser, MIDI remote, undo/redo, resize, AAX

---

## References
- `resources/Looperator.pdf` — Sugar Bytes Looperator user manual (63 pages)
- `zikada-rec/resources/zikada-3886-website/` — VJ control panel UI reference
- `zikada-rec/apps/web/src/app/globals.css` — Complete design token system
- `zikada.io` — Live brand identity reference
