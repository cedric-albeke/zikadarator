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

## Current Rebuild Status

As of 2026-04-29, the Windows-native engine rebuild has completed the first realtime-safety/routing pass:

- `processBlock` is split into sample-accurate scheduler segments through `StepScheduler`.
- `SliceEngine` and `LoopEngine` use `RealtimeRingBuffer` history instead of FIFO-readiness semantics.
- `LoopEngine` freezes triggered loops into owned snapshot buffers, so playback repeats the captured audio window instead of following later rolling input.
- Per-lane mix snapshots the signal entering each lane, processes that lane, then blends lane output against lane input. A lane mix below 100% must not reduce previous lanes or the full track output.
- Waveform UI data crosses from audio thread to message thread through `WaveformTap`; `processBlock` no longer touches the active editor. The processor publishes separate input and processed-output taps for the stacked signal display.
- `DelayEngine` uses JUCE fractional delay with smoothed delay, mix, and feedback targets.
- `FilterEngine` now has distinct 12 dB and cascaded 24 dB modes, real band-reject behavior, and an implemented comb path.
- The editor is constrained to a fixed 3:2 aspect ratio.
- The step-grid chain animation avoids full-grid timer repaint; UI timers should repaint only the components whose visual state changed.
- Regression coverage lives in `ZikadaEngineTests` plus `scripts/source-smoke-tests.mjs`.

Known limitation: advanced pitch/time/grain/vinyl labels still need to be aligned with implemented DSP or replaced by a real time-stretch/pitch library.

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
│   │   ├── SliceEngine.h       # Audio slicing / buffer shuffle
│   │   ├── LoopEngine.h        # Dedicated micro-loop / stutter engine
│   │   ├── FilterEngine.h      # Filter + vowel / formant style shaping
│   │   ├── DelayEngine.h
│   │   ├── ReverbEngine.h
│   │   ├── BitcrushEngine.h
│   │   ├── PitchEngine.h
│   │   ├── ModulationEngine.h
│   │   └── GainPanEngine.h
│   ├── ui/                     # User interface
│   │   ├── ZikadaLookAndFeel.h # Brand color/fonts (not LnF inheritance)
│   │   ├── components/         # Reusable UI widgets
│   │   │   ├── StepGrid.h      # 16x6 step sequencer grid
│   │   │   ├── StepCell.h      # Individual step button
│   │   │   ├── Knob.h          # Custom rotary knob
│   │   │   ├── WaveformDisplay.h
│   │   │   ├── VcrLabel.h      # Monospace uppercase label
│   │   │   └── StepGrid.h
│   │   ├── panels/             # Main layout panels
│   │   │   ├── HeaderPanel.h
│   │   │   ├── SequencerPanel.h
│   │   │   ├── FooterPanel.h
│   │   │   ├── SidebarPanel.h
│   │   │   └── WorkspacePanel.h
│   │   └── fonts/              # Embedded binary fonts
│   └── state/                  # Parameter / state management
│       ├── PluginState.h
│       ├── ParameterIDs.h
│       ├── SequencerState.h
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
- **Step resolution**: 1/16, 1/8, 1/4, 1/2 note (configurable 1, 2, 4, or 8 bar total loops across 16 steps)
- **Reorderable lanes**: Users can drag lane headers to change signal flow
- **Per-lane dry/wet**: Mix control for each FX lane, scoped to that lane's input/output pair
- **Per-lane mute/solo**: M (`warning` amber) and S (`neonGreen`) buttons on each lane header; solo logic gates the audio engine signal flow
- **Tie steps**: Visually rendered as a continuous rounded bar across tied cells in the step grid

### Signal Flow (Default)
```
INPUT → SLICE → LOOP → ENVELOPE → FX1 → FILTER → FX2 → MIX → OUTPUT
```

### Processing Model
- `PluginProcessor::processBlock` snapshots parameters once per block, builds fixed-stack scheduler segments, and calls `processSegment` for each step-owned sample range.
- `StepScheduler` converts host/free-clock PPQ into sample offsets, step index, phase start, and phase delta.
- `SliceEngine` and `LoopEngine` capture input/history through `RealtimeRingBuffer`, allowing deterministic overwrite history and interpolated reads.
- `LoopEngine::trigger()` creates a frozen loop snapshot from the history buffer. Playback reads the snapshot with interpolation and wrap smoothing.
- `Envelope` processing applies per-step amplitude curves using scheduler phase.
- FX1 and FX2 host delay, reverb, bitcrush, pitch-color, and tone-filter paths.
- `FilterEngine` is used both as the dedicated FILTER lane and as an internal tone shaper for FX presets.
- Final global mixing applies dry/wet, mix mode, and output gain after lane processing.
- Input waveform samples are pushed into `waveformTap`; processed output samples are pushed into `processedWaveformTap`. `PluginEditor::timerCallback` pops both streams and feeds `WaveformDisplay` as two stacked waveform lanes while the Sequencer is visible.
- Hidden editors and non-Sequencer pages discard queued tap samples on the UI consumer side, and leaving the Sequencer clears retained display history. Returning therefore starts from current audio instead of replaying a stale FIFO backlog.
- Tap reconfiguration rejects new producer/consumer operations and waits for active operations before resizing storage, preventing host reprepare from racing an open editor. A generation counter invalidates retained display history after sample-rate or tap resets.
- `WaveformDisplay` draws rolling min/max waveform bins for input and output over the current 16-step musical loop span, with display-only normalization. It is a UI diagnostic path only and must not feed back into DSP.

### Realtime Rules

- No `getActiveEditor()` calls from `processBlock`.
- No `juce::Logger::writeToLog` calls from `processBlock`.
- No `std::vector<float>` allocation or growth inside `processBlock`; processor scratch buffers are sized in `prepareToPlay`, and oversized host blocks are processed in bounded chunks against that fixed capacity.
- Realtime parameter access uses constructor-cached atomic pointers; `processBlock` does not construct parameter IDs or search APVTS.
- `juce::AbstractFifo` is acceptable for UI telemetry, but not for audio history semantics.

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
┌──────────────────────────────────────────────────────────────────────────┐
│ HEADER (single 80px row): Logo | Tabs | Preset Strip | Undo/Redo        │
├──────────────────────────────────────────────────────────────────────────┤
│ PAGE A — SEQUENCER                                                      │
│   Signal display (98px waveform strip)                                  │
│   6-lane step grid with lane-icon chips                                 │
│   Right FX preset sidebar (300px) with category header + icon grid      │
│   Footer detail dock                                                    │
├──────────────────────────────────────────────────────────────────────────┤
│ PAGE B — PRESETS                                                        │
│   Search/filter browser | preset details | library metadata             │
├──────────────────────────────────────────────────────────────────────────┤
│ PAGE C — SETTINGS                                                       │
│   Embedded standalone Audio/MIDI device selector                        │
└──────────────────────────────────────────────────────────────────────────┘
```

### HeaderPanel Architecture
The header is implemented as a single 80px row (`HeaderPanel`) with a custom paint split:

- **`paint()`** — draws background, logo, wordmark (`ZIKADA RATOR V1`), and subline (`SEQUENCE THE SIGNAL`). The logo uses the pre-scaled `zikadacicada128_png` (256×256) asset to avoid pixelation, with a subtle neon-green brightness overlay.
- **`paintOverChildren()`** — draws tab cells (borders, dot indicators, text), the preset strip container, nav chevrons, and undo/redo SVG icons *on top* of the button hit-zones. This prevents JUCE's default `TextButton` look-and-feel from interfering with custom styling.

**Layout flow (left → right):**
1. **Logo block** — cicada logo + wordmark + subline
2. **Tabs** — `SEQUENCER`, `PRESETS`, `SETTINGS` (radio group, custom drawn)
3. **Flexible spacer**
4. **Preset strip** — prev/next chevrons, save icon, preset name, dropdown triangle
5. **Undo / Redo** — SVG action icons

**SVG loading pattern:**
Inline SVG strings are parsed via `juce::parseXML()` → `juce::Drawable::createFromSVG()`. Stroke/fill attributes are placed directly on each `<path>` element because JUCE's SVG renderer does not reliably inherit presentation attributes from parent `<svg>` elements.

### Design Principles
- **Vector-based**: All UI drawn via `juce::Graphics` (no raster assets needed)
- **Bounded animations**: Editor, waveform, and CRT timers use a stable 30 Hz cadence with region-limited repaints
- **Immediate feedback**: Every interaction has a visual response within 1 frame
- **Keyboard + mouse**: Scroll wheel cycles presets, normal drag copies/erases steps, Shift+Drag draws ties, and right-click deletes or trims
- **Resizable**: Editor scales from 900x600 to 2400x1600 while preserving a fixed 3:2 aspect ratio
- **Standalone integration**: Audio device, sample rate, buffer size, and MIDI routing are embedded inside the Settings tab via JUCE standalone host APIs
- **Preset access**: Header dropdown and browser both read from the same metadata-backed preset ordering

### SidebarPanel Architecture
The right sidebar (`SidebarPanel`) is a reactive FX preset selector with three key visual layers:

- **Header area (98 px)** — matches the waveform strip height. Shows a large procedural lane category icon, the lane name in bold, and a `SELECT PRESET` sublabel. The header background uses the lane colour as a faint left-edge accent and rounded chip styling.
- **Preset icon grid** — 5-column square button grid. Each lane now exposes **16 factory presets + 4 user slots (U1–U4) = 20 total buttons**. Buttons have no text; preset identity is communicated entirely through procedural icons drawn in `paintOverChildren()`.
- **Icon system** — `PresetIcons.h` provides a pure `juce::Path` / `juce::Graphics` drawing library (no raster assets). It defines:
  - **Lane category icons** (`drawSliceIcon`, `drawLoopIcon`, `drawEnvelopeIcon`, `drawFxIcon`, `drawFilterIcon`) used in both the sidebar header and the StepGrid lane chips.
  - **Preset icons** per lane type — parameterized shapes such as slice-count dots, loop waveform windows, reverse arrows, speed chops, envelope waveforms, FX effect symbols, and filter response curves.

**Layout flow (left → right in sequencer page):**
1. **Step grid** — 6 lanes × 16 steps, each lane chip shows the category icon + lane name + `ROW N`
2. **Sidebar** — 300 px wide, right-aligned, reacts to the selected step
3. **Waveform strip** — 98 px tall, sits above the step grid

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
- Per-lane Mute (6 params)
- Per-lane Solo (6 params)
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

### Standalone Host State
- Embedded `AudioDeviceSelectorComponent` uses JUCE standalone host state
- Audio device setup is still owned by `juce::StandalonePluginHolder`
- Settings tab now surfaces the same device options that used to live only behind the standalone host Options button

### Preset Metadata State
- `PresetManager` persists favorites and recent-use ordering alongside user presets
- Header dropdown and browser both read from the same metadata-backed preset ordering

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
