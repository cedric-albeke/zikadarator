# Zikada Design System

Extracted from zikada.io and zikada-rec resources. Use these exact values for all UI implementation.

---

## Color Palette

### Primary Colors
| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **Neon Green** | `#00FF85` | rgb(0, 255, 133) | Primary accent, active states, CTA buttons, play buttons, prices, meters |
| **Secondary Green** | `#00DE74` | rgb(0, 222, 116) | Dimmed green, secondary accents |
| **Third Green** | `#00AF5B` | rgb(0, 175, 91) | Dark green, hover states |
| **Dark Green** | `#00B75B` | rgb(0, 183, 91) | Borders, dividers |

### Background Colors
| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **BG Primary** | `#000C0D` | rgb(0, 12, 13) | Main plugin background |
| **BG Accent** | `#011C1A` | rgb(1, 28, 26) | Elevated surfaces, panels |
| **BG Surface** | `#0D1F1E` | rgb(13, 31, 30) | Cards, input fields |
| **BG Hover** | `#071212` | rgba(7, 18, 18, 0.8) | Hover states |
| **Frosted** | `#00000080` | rgba(0, 0, 0, 0.5) | Glassmorphism backgrounds |

### Neutral Colors
| Name | Hex | Usage |
|------|-----|-------|
| **White** | `#FFFFFF` | Primary text, headings |
| **White 85%** | `#FFFFFFD9` | Secondary text |
| **White 50%** | `#FFFFFF80` | Muted text, disabled states |
| **White 10%** | `#FFFFFF1A` | Subtle borders |
| **Dark Grey** | `#333333` | Dark borders |

### Lane Accent Colors (FX Sequencer)
| Lane | Hex | Name |
|------|-----|------|
| INPUT | `#FFFFFF` | White |
| SLICE | `#00DEFF` | Cyan |
| LOOP | `#C040C0` | Magenta |
| ENVELOPE | `#8A40FF` | Violet |
| FX1 | `#00FF85` | Neon Green |
| FILTER | `#BFFF00` | Yellow-Green |
| FX2 | `#00FFB3` | Teal |

### Semantic Colors
- **Success/Active**: `#00FF85` (neon green pulse)
- **Warning**: `#FFB800` (amber)
- **Error**: `#FF4444` (red)
- **Audio Waveform**: `#C040C0` (magenta) or `#00FF85` (green)

---

## Typography

### Font Stack
```cpp
// Primary UI
"Inter", -apple-system, BlinkMacSystemFont, sans-serif

// Monospace / Data
"Space Mono", "SF Mono", Consolas, monospace

// Display / Logo
"Anta", sans-serif

// VCR / Labels
"VCR OSD Mono", "Courier New", monospace
```

### Type Scale
| Element | Size | Weight | Letter Spacing | Line Height |
|---------|------|--------|----------------|-------------|
| **H1 Hero** | 96px | 700 | -2.4px | 1.0 |
| **H2 Section** | 36px | 700 | normal | 1.2 |
| **H3 Card** | 20px | 700 | normal | 1.3 |
| **H4** | 14px | 500 | normal | 1.4 |
| **Body** | 16px | 400 | normal | 1.5 |
| **Body Small** | 14px | 400 | normal | 1.5 |
| **Caption** | 12px | 400 | 0.5px | 1.4 |
| **VCR Label** | 12px | 400 | 2px (uppercase) | 1.0 |

### VCR Label Style
```cpp
// For section headers, parameter names, navigation
font-family: "VCR OSD Mono";
font-size: 12px;
font-weight: 400;
letter-spacing: 2px;
text-transform: uppercase;
color: #00FF85; // or #FFFFFF for inactive
```

### Monospace Data Style
```cpp
// For values, BPM, step numbers, percentages
font-family: "Space Mono";
font-size: 14px;
font-weight: 700;
color: #FFFFFF;
```

---

## Spacing & Layout

### Grid
- **Base unit**: 4px
- **Step grid gap**: 4px between cells
- **Panel padding**: 16px, 24px
- **Lane height**: 48px
- **Step cell size**: 32px × 40px

### Border Radius
| Element | Radius |
|---------|--------|
| Small buttons | 3px |
| Input fields | 4px |
| Cards | 6px |
| Panels | 8px |
| Large cards | 12px |

### Shadows & Glows
```cpp
// Neon glow (active elements)
box-shadow: 0 0 10px rgba(0, 255, 133, 0.5),
            0 0 20px rgba(0, 255, 133, 0.3);

// Subtle panel shadow
box-shadow: 0 4px 24px rgba(0, 0, 0, 0.4);

// Inner glow (pressed buttons)
box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.5);
```

---

## Components

### Step Cell
```cpp
// Inactive
background: #0D1F1E;
border: 1px solid rgba(255, 255, 255, 0.1);

// Active (with effect)
background: {laneColor}; // e.g., #00FF85 for FX1
border: 1px solid {laneColor};
box-shadow: 0 0 8px {laneColor}40; // 25% opacity glow

// Hover
border-color: {laneColor};

// Selected (editing)
outline: 2px solid #FFFFFF;
outline-offset: 2px;
```

### Knob
- Size: 48px diameter
- Track: `#333333` (background arc)
- Fill: `{laneColor}` (value arc)
- Indicator: `#FFFFFF` line
- Center dot: `#00FF85` (when active)

### Button (Primary)
```cpp
background: #00FF85;
color: #000C0D;
border-radius: 3px;
padding: 14px 28px;
font-family: "Space Mono";
font-weight: 700;
font-size: 14px;
text-transform: uppercase;
letter-spacing: 2px;

// Hover
background: #00AF5B;

// Active
background: #002F17;
color: #FFFFFF;
```

### Button (Ghost)
```cpp
background: transparent;
color: #00FF85;
border: 1px solid #00FF85;

// Hover
background: rgba(0, 255, 133, 0.1);
```

### Panel / Card
```cpp
background: #0D1F1E;
border: 1px solid rgba(0, 255, 133, 0.2);
border-radius: 8px;
padding: 16px;
```

### VCR Section Header
```cpp
// Label above section
color: #00FF85;
font-family: "VCR OSD Mono";
font-size: 12px;
letter-spacing: 2px;
text-transform: uppercase;
margin-bottom: 8px;
```

### Status Indicator (System Online)
```cpp
// Animated green dot
width: 8px;
height: 8px;
background: #00FF85;
border-radius: 50%;
box-shadow: 0 0 8px #00FF85;
animation: pulse 2s ease-in-out infinite;
```

---

## Interaction Patterns

### Hover States
- **NO translateX/translateY transforms** — only color, shadow, scale
- **Max scale**: 1.02 (very subtle)
- **Allowed transitions**: opacity, box-shadow, border-color, background-color

### Active/Pressed States
- Inset shadow
- Slightly darker background
- Color inversion for primary buttons

### Scroll Wheel
- Primary interaction for cycling through options
- Works on: step cells (cycle presets), knobs (fine adjust), faders

### Drag to Paint
- Drag horizontally across step cells to copy values
- Visual feedback: trail effect, cursor change

### Keyboard Shortcuts
- **Shift + Click**: Create tie step
- **Right-click**: Delete step / context menu
- **Space**: Play/Pause transport
- **Undo/Redo**: Cmd/Ctrl+Z, Cmd/Ctrl+Shift+Z

---

## Animation

### Timing Functions
```cpp
// Standard ease
juce::Easing::easeInOut

// Bounce (for playful elements)
juce::Easing::easeOutBack

// Quick response
juce::Easing::easeOut
```

### Durations
| Animation | Duration |
|-----------|----------|
| Hover transitions | 150ms |
| Panel open/close | 200ms |
| Value changes | 100ms |
| Playhead step | 50ms (sync to audio) |
| Glow pulse | 2000ms (infinite) |

---

## Brand Assets

### Cicada Logo
- **File**: `assets/images/zikada-cicada.png`
- **Usage**: About dialog, splash screen, preset browser header
- **Style**: Glowing neon green cicada on dark teal coin

### Fonts (Embedded)
All fonts located at: `/home/zady/Development/zikada-rec/resources/3886-dev.webflow/fonts/`

| Font | File | License |
|------|------|---------|
| Space Mono Bold | SpaceMono-Bold.ttf | OFL |
| Space Mono Regular | SpaceMono-Regular.ttf | OFL |
| Anta Regular | anta-regular.ttf | OFL |
| VCR OSD Mono | VCR.ttf | Free for commercial use |

---

## Audio Visualization

### Waveform Display
- Background: `#0D1F1E`
- Waveform fill: `#C040C0` (magenta) or `#00FF85` (green)
- Grid lines: `rgba(255, 255, 255, 0.05)`
- Slice markers: `#FFFFFF` vertical lines
- Playhead: `#00FF85` with glow

### Level Meters
- Background: `#000C0D`
- Low: `#00FF85` (green)
- Mid: `#BFFF00` (yellow-green)
- High: `#FFB800` (amber)
- Peak: `#FF4444` (red)
- Peak hold: `#FFFFFF`

---

## Language & Copy

### Transmission/Signal Metaphors
Use for UI labels where appropriate:
- "SIGNAL" instead of "Audio"
- "TRANSMISSION" instead of "Output"
- "CARRIER" instead of "Master"
- "DECODE" instead of "Process"
- "HIVE" for community/collective

### Section Labels (VCR Style)
- `SIGNAL.LOG`
- `FX.MATRIX`
- `CARRIER.WAVE`
- `SYSTEM.STATUS`
- `MODULATION.NETWORK`

---

## Responsive Sizing

The plugin UI should scale from 75% to 200%:
- **Default**: 100% (1200×800px editor size)
- **Compact**: 75% (900×600px)
- **Large**: 150% (1800×1200px)
- **Presentation**: 200% (2400×1600px)

Use `juce::AffineTransform` for scaling the entire editor canvas.
