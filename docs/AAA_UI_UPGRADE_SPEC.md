# ZIKADARATOR AAA UI/UX Upgrade Specification

## Design Philosophy

Target: Enterprise-grade, AAA plugin aesthetic. Deep, immersive dark surfaces with subtle 3D depth, precise arc-ring controls, and bold color discipline. Every pixel earns its place. The UI should feel like Serum 2 meets Effectrix 2 — premium, precise, and purposeful.

## Color System (Updated)

### Background Hierarchy
| Token | Value | Usage |
|---|---|---|
| `shellBg` | `#000708` | Outermost window background |
| `bgPrimary` | `#000C0D` | Base panel surfaces |
| `bgAccent` | `#011C1A` | Lifted panel surfaces (every 2nd lane) |
| `bgSurface` | `#0D1F1E` | Control backgrounds, button surfaces |
| `bgHover` | `#071212CC` | Hover overlays |
| `panelRaised` | `#022220` | Raised module surfaces |
| `displayBezel` | `#00100F` | Inset display bezels |

### Accent Colors (Per Lane)
| Lane | Color | Hex |
|---|---|---|
| SLICE | Cyan | `#00DEFF` |
| LOOP | Purple | `#C040C0` |
| ENVELOPE | Violet | `#8A40FF` |
| FX1 | Neon Green | `#00FF85` |
| FILTER | Lime | `#BFFF00` |
| FX2 | Teal | `#00FFB3` |

### Functional Colors
| Token | Value | Usage |
|---|---|---|
| `neonGreen` | `#00FF85` | Primary accent, playhead, active indicators |
| `secondaryGreen` | `#00DE74` | Secondary accent |
| `white` | `#FFFFFF` | Primary text, bright elements |
| `white85` | `#FFFFFFD9` | Secondary text |
| `white50` | `#FFFFFF80` | Muted text, disabled |
| `white10` | `#FFFFFF1A` | Subtle borders, dividers |
| `warning` | `#FFB800` | Warning states, mute |
| `error` | `#FF4444` | Error states |
| `success` | `#00FF85` | Success states |

## Panel System (3D Depth)

### Premium Panel Framing
All panels now use a 3-layer depth system:

1. **Outer rim** — 1px line at `#white10` with subtle shadow
2. **Body surface** — Fill at `bgPrimary` or `panelRaised`
3. **Inner bezel** — 1px inset line at `#000000` with subtle highlight

```
┌─────────────────────────┐  ← Outer rim (white10, 1px)
│ ┌─────────────────────┐ │  ← Body (bgPrimary)
│ │  ○───────○          │ │  ← Content area
│ │  Content here       │ │
│ └─────────────────────┘ │  ← Inner bezel (black inset, 1px)
└─────────────────────────┘
```

### Corner Radius
- Outer panels: `8.0f`
- Inner modules: `5.0f`
- Buttons/knobs: `4.0f` or `6.0f` (rounded)
- Step cells: `4.0f`

### Shadows
- Panel drop shadow: `2px` offset, `4px` blur, `rgba(0,0,0,0.3)`
- Inset shadow on displays: `1px` offset, `2px` blur, `rgba(0,0,0,0.5)` inside

## Knob Design (Arc-Ring System)

### Visual Structure
```
    ╭───────╮
   ╱    │    ╲     ← Outer ring (bgSurface, 2px)
  │   ╭───╮   │
  │  │ ● │   │    ← Center cap (panelRaised, 3D bevel)
  │   ╰───╯   │
   ╲  ═════  ╱     ← Colored arc (lane accent, 3px)
    ╰───────╯
```

### Specifications
- **Outer ring**: 2px stroke at `bgSurface` with 1px inner highlight at `white10`
- **Arc indicator**: 3px stroke at lane color, arc length = normalized value × 270° (starts at 7 o'clock)
- **Center cap**: 60% of knob diameter, `panelRaised` fill with subtle radial gradient for 3D effect
- **Center dot**: 4px circle at lane color (when active) or `white50` (when inactive)
- **No ticks**: The arc IS the indicator. Ticks clutter the clean aesthetic.
- **Label**: Below knob, 11px Space Mono, `white85`
- **Value**: Below label, 10px Space Mono, lane color (when active) or `white50` (when inactive)

### Interaction States
- **Hover**: Arc glows brighter (alpha +0.15), center cap lifts slightly
- **Drag**: Arc thickens to 4px, center dot enlarges to 6px
- **Disabled**: Entire knob desaturates to `white50`, arc hidden

## Step Cell Design

### Structure
```
┌──────────┐
│  ╭──╮    │   ← Preset icon (when active)
│  │  │    │
│  ╰──╯    │
│  ░░░░    │   ← Mini waveform preview (when active)
│     12   │   ← Step number (bottom-right, 9px)
└──────────┘
```

### Specifications
- **Size**: 48×48px (at default 1200×800 scale)
- **Border**: 1px rounded rectangle at `bgSurface`
- **Active state**: Fill at `bgAccent` with 2px border at lane color (alpha 0.6)
- **Playing state**: Outer glow ring at `neonGreen` (alpha 0.4), 3px blur
- **Selected state**: 2px border at `white` (alpha 0.8), inner shadow
- **Preset icon**: 16×16px, lane color, centered in upper half
- **Step number**: 9px Space Mono, `white50`, bottom-right corner
- **Hover**: Subtle brightness lift (+0.05) on fill

### Chain Indicators
- **Chained cells**: Connected by a 2px line at lane color (alpha 0.4) between cell centers
- **Chainable indicator**: Small `+` badge at bottom-left, lane color, 10px

## Waveform Display

### Specifications
- **Bezel**: `displayBezel` with 2px inset shadow, 5px corner radius
- **Grid**: Vertical lines at 1/16 boundaries, `white10` at 0.5px; horizontal center line at `white10` at 0.8px
- **Waveform**: 2px stroke at lane color (alpha 0.7), filled underneath at lane color (alpha 0.15)
- **Playhead**: 2px vertical line at `neonGreen` with 6px glow gradient (alpha 0.3 → 0.0)
- **Step markers**: Small dots at step boundaries, `white50` at 2px
- **Normalization**: Auto-gain with 150ms attack time (smooth transitions)
- **Input/output lanes**: Stacked with 3px gap, input in `laneSlice`, output in `waveform` (#C040C0)

## Header Design

### Structure (Compact, Icon-Driven)
```
┌─────────────────────────────────────────────────────────┐
│ [Z] ZIKADARATOR  v1  │  [SEQ] [PRE] [SET]  │  ◄ ◄ PRESET ► ►  │  ↩  ↪  │
└─────────────────────────────────────────────────────────┘
```

### Specifications
- **Height**: 60px (reduced from 80px for more content area)
- **Logo**: 36×36px icon + "ZIKADARATOR" in 18px VCR font + "v1" badge in 10px Space Mono
- **Page tabs**: Icon + label, 14px Space Mono, `white50` inactive / `white` active with underline indicator at `neonGreen` (2px)
- **Preset display**: Compact row with preset name (14px VCR) + category (11px Space Mono) + dirty dot
- **Undo/Redo**: 20×20px icons, `white50` with hover at `white`
- **Background**: `shellBg` with bottom border at `white10` (1px)

## Footer Design

### Structure (Grouped Parameter Rows)
```
┌─────────────────────────────────────────────────────────┐
│  DRY/WET ─○──────  │  STEP DETAIL  │  LEN  │  RATE  │  ...
│  [====]              │  [knob] [knob] │ [knob]│ [knob] │
│  100%               │  7%      20%   │  9%   │  48%   │
└─────────────────────────────────────────────────────────┘
```

### Specifications
- **Height**: 150px (reduced from 170px)
- **Group headers**: 10px Space Mono, `white50`, uppercase, above each knob group
- **Knob rows**: 2 rows of 6 knobs each, 12px gaps
- **Global controls**: Left column — DRY/WET slider (vertical, 120px tall) + OUTPUT knob + MIX MODE selector + BYPASS button
- **Step detail**: Middle column — 6 knobs for the selected step's parameters
- **Right column**: MIX MODE selector (button group), OUTPUT knob, STEP RES selector
- **Background**: `bgPrimary` with top border at `white10` (1px)

## Sidebar Design

### Structure
```
┌────────────────────────┐
│  LOOP                  │  ← Lane name, 18px VCR, lane color
│  ┌──────────────────┐  │
│  │ Preset grid      │  │  ← 4×5 grid of preset icons
│  │ [1][2][3][4]     │  │
│  │ [5][6][7][8]     │  │
│  └──────────────────┘  │
│  ┌──────────────────┐  │
│  │ Preset details   │  │  ← Info panel with name, category, params
│  │ Name: DELAY PULSE│  │
│  │ Cat:  DELAY      │  │
│  └──────────────────┘  │
└────────────────────────┘
```

### Specifications
- **Width**: 280px (reduced from 300px)
- **Lane header**: 18px VCR font, lane color, with icon
- **Preset grid**: 4 columns × 5 rows, 44×44px cells, 4px gaps
- **Preset cell**: Rounded square, `bgSurface` fill, lane color border when active, preset icon centered
- **Details panel**: `displayBezel` background, 12px Space Mono text, lane color for parameter names
- **Background**: `bgPrimary` with left border at `white10` (1px)

## Sequencer Panel Design

### Structure
```
┌─────────────────────────────────────────────────────────┐
│  ┌──────────────────────────────────────────────────┐  │
│  │  SIGNAL  │  Waveform Display (large, detailed)     │  │
│  │  INPUT   │                                        │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  SLICE  │  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16  │ M S  │  │
│  │  ROW 1  │  □  □  □  □  □  □  □  □  □  □  □  □  □  □  □  □  │ ○  │  │
│  │  LOOP   │  □  □  □  □  □  □  □  □  □  □  □  □  □  □  □  □  │ ○  │  │
│  │  ROW 2  │  □  □  □  □  □  □  □  □  □  □  □  □  □  □  □  □  │ ○  │  │
│  │  ...    │  ...                                    │ ○  │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### Specifications
- **Waveform area**: 98px tall (unchanged), but with improved bezel and grid
- **Step grid area**: Remaining space, 6 lanes × 16 steps
- **Lane chip**: 100px wide, `displayBezel` background, lane color accent strip on left (4px)
- **Step cell**: 48×48px, 2px gaps between cells
- **Mix knob strip**: 48px wide, right side, arc-ring knob + M/S buttons stacked
- **Playhead rail**: 2px vertical line at current step, `neonGreen` with glow, spans all lanes
- **Ruler**: 14px tall, beat markers at 1, 5, 9, 13 with `neonGreen` accent

## Typography System

| Role | Font | Size | Weight | Color |
|---|---|---|---|---|
| App title | VCR | 18px | Normal | `white` |
| Section headers | VCR | 14px | Normal | `white85` |
| Lane names | VCR | 15px | Normal | Lane color |
| Labels | Space Mono | 11px | Normal | `white50` |
| Values | Space Mono | 10px | Normal | Lane color (active) or `white50` |
| Step numbers | Space Mono | 9px | Normal | `white50` |
| Preset names | VCR | 14px | Normal | `white` |
| Preset meta | Space Mono | 11px | Normal | `white50` |
| Button text | Space Mono | 12px | Normal | `white85` |

## Animation & Micro-interactions

| Interaction | Effect | Duration | Easing |
|---|---|---|---|
| Step hover | Brightness +0.05 | 100ms | ease-out |
| Step click | Scale 0.95 → 1.0 | 150ms | ease-out |
| Step active | Border fade in | 200ms | ease-in-out |
| Knob hover | Arc alpha +0.15 | 100ms | ease-out |
| Knob drag | Arc thickness 3→4px | instant | — |
| Playhead advance | Rail position lerp | 50ms | linear |
| Panel transition | Opacity 0→1 | 200ms | ease-out |
| Preset change | Flash lane color | 300ms | ease-out |
| Chain draw | Line draw animation | 150ms | ease-out |
| Focus ring | Pulse glow | 800ms | ease-in-out (loop) |

## Spacing & Metrics

| Element | Value |
|---|---|
| Window size | 1200×800 (default), 900×600 (min) |
| Shell inset | 8px |
| Module gap | 6px |
| Panel padding | 14px |
| Inner padding | 10px |
| Control gap | 12px |
| Knob gap | 12px |
| Step gap | 2px |
| Lane gap | 0px (tight) |
| Header height | 60px |
| Footer height | 150px |
| Sidebar width | 280px |
| Waveform height | 98px |
| Ruler height | 14px |
| Lane chip width | 100px |
| Mix strip width | 48px |
| Step cell size | 48×48px |
| Knob diameter | 40px |
| Button height | 20px |
| Icon size | 16–20px |
| Corner radius (outer) | 8px |
| Corner radius (inner) | 5px |
| Corner radius (button) | 4px |
| Border width | 1px |

## Implementation Roadmap

### Slice 1: Panel Depth & Framing
- Update `ZikadaLookAndFeel::drawPremiumPanel` with 3-layer depth system
- Update `ZikadaLookAndFeel::drawDeviceDisplay` with improved bezel and inset shadow
- Update all panel backgrounds to use the new depth system
- Smoke test: verify panel depth functions exist

### Slice 2: Knob Redesign (Arc-Ring)
- Redesign `Knob` component with arc-ring indicator system
- Add 3D center cap with radial gradient
- Remove tick marks, replace with arc indicator
- Update footer knob layout for tighter spacing
- Smoke test: verify arc indicator, center cap, value label

### Slice 3: Step Cell Redesign
- Redesign `StepCell` with chunkier active states
- Add preset icon display in active cells
- Add outer glow for playing state
- Add chain connection lines
- Update `StepGrid` layout for tighter spacing
- Smoke test: verify step cell states, chain visuals

### Slice 4: Waveform Display Upgrade
- Redesign `WaveformDisplay` with improved bezel and grid
- Add step boundary markers
- Improve waveform rendering with fill and stroke
- Add auto-gain normalization with smooth transitions
- Smoke test: verify waveform display, grid, playhead

### Slice 5: Header Redesign
- Compact header to 60px
- Icon-driven page tabs with underline indicator
- Improved preset display with dirty indicator
- Smoke test: verify header layout, tab switching

### Slice 6: Footer & Sidebar Redesign
- Tighten footer to 150px with grouped parameter rows
- Redesign sidebar with preset grid and details panel
- Smoke test: verify footer layout, sidebar preset grid

### Slice 7: Animation & Micro-interactions
- Add hover animations to step cells and knobs
- Add step click scale animation
- Add focus ring pulse animation
- Add chain draw animation
- Smoke test: verify animation functions exist

### Slice 8: Final Polish & Integration
- Verify all spacing metrics match spec
- Verify color application consistency
- Verify typography hierarchy
- Final smoke test pass
- Build and validate
