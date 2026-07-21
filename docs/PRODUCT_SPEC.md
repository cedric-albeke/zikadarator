# Zikada FX — Product Specification

## Elevator Pitch
A 16-step multi-FX sequencer for modern electronic music production. Chop, stutter, filter, and transform loops in real-time with a cyberpunk-native UI that feels like an instrument.

## Tagline
**"Sequence the signal."**

---

## Core Concept

Zikada FX sits on any audio track and processes incoming audio through a 16-step sequencer. Each step can trigger one of 20+ effect presets across 6 independent lanes. The lanes can be reordered to create radically different signal flows.

Unlike simple on/off step sequencers, each step selects *which variant* of an effect to apply — creating melodic, rhythmic, and textural patterns that evolve over time.

---

## The 6 Lanes

### 1. INPUT — Buffer & Slice Visualization
- **Function**: Real-time audio capture and auto-slicing
- **Visual**: Live waveform display with 16 slice markers
- **Controls**: Slice sensitivity, transient detection mode
- **Output**: 16 discrete audio slices shared across all lanes

### 2. SLICE — Buffer Shuffle
- **Function**: Rearrange which of the 16 input slices plays at each step
- **Presets**: Forward, Reverse, Scatter, Repeat, Stutter patterns, Random shuffle
- **Unique behavior**: Each step can reference ANY of the 16 buffer slices
- **Visual warning**: Exclamation mark (!) on steps referencing future/unfilled slices

### 3. LOOP — Stutter & Repeat
- **Function**: Micro-looping and stutter effects per step
- **Engine model**: Dedicated micro-loop engine with captured audio history, retrigger, reverse, and variable playback rate
- **Implemented presets**: Forward 1/16, 1/8, 1/4, 1/2; Reverse 1/16, 1/8, 1/4, 1/2; Speed x2/x4; Slow x1/2/x1/4; Reverse x2/x4; Tail 4x/8x
- **User controls**: LEN, RATE, REV, FADE, MIX, VOL, PAN
- **Visual contract**: Each LOOP preset icon must depict its audible behavior with waveform windows, direction arrows, chopped speed markers, or long-tail blocks.
- **Creative use**: Build tension with rhythmic stutters before drops

### 4. ENVELOPE — Per-Step Dynamics
- **Function**: Volume shaping per step
- **Curves**: 20 preset envelope shapes:
  - Linear attack/decay
  - Exponential curves
  - Hard gates
  - Tremolo shapes
  - Wobble curves
  - Sine/swelling envelopes
- **Controls**: Start point, end point, curve amount

### 5. FX1 — Primary Multi-Effect
- **Function**: First dedicated FX lane
- **Effect types** (selectable per step):
  1. **Delay** — Stereo delay with feedback, filter, pitch
  2. **Reverb** — Plate/hall with size, damp, width
  3. **Distortion** — Drive with multi-band EQ, mono/stereo modes
  4. **Grain** — Granular synthesis with size, density, pitch
  5. **Tonalizer** — Tuned delay tails with scale quantization
  6. **Phaser** — Stereo phaser with LFO sync
  7. **Vinyl** — Speed control, stop/reverse, scratch simulation
  8. **ChaosSynth** — Synthetic textures with FM/AM modulation
  9. **Stretch** — Time-stretch with formant preservation
  10. **RingMod** — Ring modulation with LFO

### 6. FILTER — Frequency Shaping
- **Function**: Per-step filter + formant effects
- **Filter types**:
  - LP 12dB / 24dB
  - HP 12dB / 24dB
  - BP / Band-Reject
  - Comb filter
  - Vowel formants (A, E, I, O, U + morphs)
- **Stereo modes**: Linked, independent L/R, split L/R

### 7. FX2 — Secondary Multi-Effect
- **Function**: Second dedicated FX lane
- **Effect types**: Same pool as FX1
- **Creative use**: Placing FX2 before FILTER creates very different results than FILTER → FX2

---

## Per-Step Depth: The 3 Layers

### Layer 1 — Step Assignment
Click any step cell to select it; the right sidebar shows the lane's FX preset grid:
- **16 factory presets** per lane, each with a unique procedural icon
- **4 user slots** (U1–U4)
- Presets are arranged in a 5-column icon grid inside the 300 px sidebar
- Selected presets are reflected as short labels inside active step cells
- Tie mode (⛓️) links steps visually with a continuous lane-colour bar
- Right-click deletes a step

### Layer 2 — User Slot Configuration
Click the `⚙` icon on any lane to open the User Editor:
- 4 tabs: U1, U2, U3, U4
- Each slot exposes 5 parameters specific to the selected effect type
- Parameters vary by effect (e.g., Delay has Time L, Time R, Feedback, Filter, Mix)
- Copy/Paste/Reset/Random per slot

### Layer 3 — Per-Parameter Modulation
Each of the 5 parameters can be modulated by:

#### A. Static
Fixed value. Automatable via DAW.

#### B. Motion
20 preset motion curves:
- Ramp up/down
- Sine/triangle/square
- Exponential swell/decay
- **Controls**: Curve shape, start amount, end amount
- **Behavior**: Modulates the parameter across the step duration

#### C. Envelope Follower
Reacts to input audio amplitude in real time:
- **Base value**: Starting parameter value
- **Direction**: Positive/negative response
- **Attack**: How fast it reacts to transients
- **Release**: How fast it decays after the transient

#### D. Random
Generates random values per trigger:
- **Min/Max**: Value range
- **#Rnd**: Number of random values per step (1–16 subdivisions)
- **Mode**: Step (jumps) or Glide (interpolates)

---

## Signal Flow & Routing

### Default Order
```
INPUT → SLICE → LOOP → ENVELOPE → FX1 → FILTER → FX2 → MIX → OUTPUT
```

### Reorderable Lanes
Users can drag lane headers vertically to change the processing order.

**Example creative routings:**
- `FILTER → SLICE` = Filter the audio BEFORE slicing (smoother results)
- `FX1 → SLICE` = Reverb-then-slice (cavernous choppy textures)
- `LOOP → ENVELOPE` = Loop inside the envelope (tighter stutters)

### Per-Lane Mix
Each lane has an independent dry/wet control on the far right of the sequencer row.

### Per-Lane Mute / Solo
Each lane header provides **M** (mute) and **S** (solo) toggle buttons. Solo logic gates the audio engine: only soloed lanes pass signal, and muted lanes are always silent. Mute/solo states are exposed as automated DAW parameters.

### Global Mix
- **Dry/Wet slider**: Master blend
- **Blend modes**:
  1. Linear — standard crossfade
  2. Ducking — dry ducks when wet is present
  3. Sidechain — wet triggered by dry amplitude
  4. Multiply — ring-mod style blend
  5. Screen — lightening blend mode
  6. Difference — absolute difference blend

---

## Transport & Clock

### Clock Modes
1. **HOST** — Syncs to DAW transport, tempo, and playhead
2. **FREE** — Internal clock with BPM control

### Step Resolution
- **1/16 note** = 1-bar loop
- **1/8 note** = 2-bar loop
- **1/4 note** = 4-bar loop
- **1/2 note** = 8-bar loop

### Trigger Behavior
- **Polyphonic** = New triggers start new patterns (layered)
- **Monophonic** = New triggers restart the pattern (Force Clock Retrigger)

---

## Preset System

### Factory Presets
- 300+ organized presets across categories:
  - Ambient
  - Build-Ups
  - Complex
  - Destruction
  - DJ Tools
  - Filter Sweeps
  - Glitch
  - Harmonic
  - Looping
  - Rearrange
  - Reverse
  - Rhythmic
  - Stutter
  - Subtle
  - Vinyl
  - User

### User Presets
- Save full sequencer state + all U1-U4 configurations
- Saved to user documents folder
- Tag-based organization (custom categories)
- Favorites + recent-use metadata drive both the browser order and the header dropdown

### Remote Lists (Performance Mode)
- Map presets to MIDI notes for live triggering
- Create set lists for live performance
- Key offset for different MIDI ranges

---

## MIDI Integration

### MIDI Learn
- Right-click any control → assign MIDI CC
- CC Recall Lock: preserve CC assignments across preset changes

### Program Change
- Switch presets via MIDI Program Change messages
- Optional: ignore tempo/mix changes on program change

### Note Trigger
- Assign presets to MIDI notes in Remote List mode
- Play the plugin like a performance instrument

---

## Randomization System

### Global Random (🎲)
Six modes that intelligently fill the entire grid:
1. **Smart** — Balanced, musical results
2. **Space** — Reverb/delay heavy
3. **Loopy** — Loop and slice focused
4. **Glitchy** — Chaotic, high-energy
5. **Subtle** — Minimal, tasteful touches
6. **Full Chaos** — Everything everywhere

### Track Random
- Per-lane randomize button
- Each lane has track-specific random modes
- Example (Filter lane): LP only, HP only, Talk, Motion, Smart, Chaos

### Step Random
- Individual steps set to Random mode
- Re-randomizes every time the step plays
- Creates ever-evolving patterns

---

## UI/UX Features

### Navigation
- **Single-row header** with tabs (Sequencer / Presets / Settings), preset strip, and undo/redo icons
- **Header preset dropdown** opens favorites, recents, full preset list, and direct browser access
- **Scroll wheel** over step cells cycles presets without opening popup
- **Drag horizontally** from an active cell to copy its preset through every crossed step
- **Drag horizontally** from an inactive cell to erase every crossed step
- **Shift+Drag** from an active cell creates Tie steps
- **Right-click** deletes steps or trims an existing Tie chain

### Visual Feedback
- Real-time INPUT waveform with slice markers (98 px tall strip)
- Animated playhead across the step grid
- Lane colors indicate active effects at a glance
- **Lane category icons** on every lane chip (SLICE, LOOP, ENVELOPE, FX1, FILTER, FX2)
- **Tie steps** rendered as a continuous colored bar spanning tied cells
- **Mute/Solo buttons** on each lane header: M (amber warning) and S (neon green)
- **Right sidebar FX preset grid** — 300 px wide, 5-column icon grid with 16 factory presets + 4 user slots per lane
- **Procedural preset icons** — every selectable FX preset has a unique vector icon (no raster assets)
- Subtle neon glow on hovered/selected elements
- In-header preset strip reflects the active preset, favorite state, and dirty state

### Settings Surface
- **Standalone builds** expose the real JUCE Audio/MIDI device panel inside the Settings tab
- Controls include output/input device selection, active input channels, sample rate, audio buffer size, MIDI inputs, and feedback-loop mute
- Product-specific workflow settings can be layered into the same tab beneath the device panel over time

### Editor Scaling
- 75% — Compact mode for small screens
- 100% — Default
- 150% — Large monitors
- 200% — Presentation/demo mode

---

## 2026 Enhancements (Differentiators from Looperator)

1. **Modern modulation**: Per-step LFOs with custom wave drawing (not just 20 presets)
2. **Macro controls**: 4 assignable macros that control multiple parameters across lanes
3. **Probability per step**: Each step has a probability % (0–100%) for generative patterns
4. **Ratchet/repeats**: Steps can repeat 1–8 times within their duration (trig conditions)
5. **MPE support**: Per-note expression for polyphonic triggering
6. **GPU-accelerated UI**: 60fps vector UI with shader-based glows and blur
7. **Cloud preset sharing**: Direct integration with Zikada platform account
8. **Dark/light adaptive**: Respects system theme (dark = default, light = inverted)

---

## Minimum Viable Product (MVP)

For initial release, focus on:
1. 16-step sequencer grid (6 lanes)
2. INPUT + SLICE + LOOP + ENVELOPE + FILTER + FX1 (FX2 deferred)
3. 10 core effect types in FX1
4. Static + Motion modulation (Env Follower + Random deferred)
5. Host sync only (Free mode deferred)
6. 50 factory presets
7. Dry/Wet mix with Linear blend mode
8. Full custom UI matching Zikada brand

---

## Target Specs

- **Latency**: ≤ 512 samples (depends on slice buffer)
- **CPU**: ≤ 8% on single core @ 48kHz (modern CPU)
- **Formats**: VST3, AU, Standalone
- **macOS**: 11+ (Intel + Apple Silicon)
- **Windows**: 10+ (x64)
- **Plugin size**: < 50MB
