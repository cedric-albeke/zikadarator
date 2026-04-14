# Zikada FX UI Design Spec

Date: 2026-04-14
Status: Approved for planning

## Goal

Elevate Zikada FX from a functional prototype into a premium sequencer-first plugin UI with the overall quality of modern high-end FX instruments. The design should keep Zikada branding and color identity, but adopt stronger panel architecture, clearer hierarchy, better readability, and more tactile control presentation.

## Decisions Already Made

- Layout direction: **A — sequencer-first with bottom detail dock**
- Top visual priority: **overall premium feel**
- Primary usage context: **in-DAW**, so the UI does **not** need a dedicated play/pause transport button
- Design target: **premium modular flat-3D hybrid**, not skeuomorphic hardware and not minimal utility UI

## Design Principles

1. **Sequencer remains the hero**
   The step matrix is the primary visual and functional element. Other controls support it rather than compete with it.

2. **Premium through hierarchy, not decoration**
   The UI should feel expensive because spacing, paneling, emphasis, and typography are deliberate—not because of excessive chrome.

3. **Brand accents stay controlled**
   Zikada neon and lane colors should be used as emphasis for active, selected, and important states. Large surfaces should remain dark and restrained.

4. **DAW-first workflow**
   The UI should assume host transport, host tempo, and host playback. Global controls should focus on plugin state, mode, presets, and performance flow.

5. **Context reveals detail**
   The top-level surface stays clean, while the bottom dock exposes richer effect and step detail for the current selection.

## Layout Architecture

### 1. Header Command Bar

Purpose: global identity and lightweight command controls.

Structure:
- Left: Zikada logo + product wordmark + compact subtitle/tagline
- Center/right: preset and workflow controls relevant during DAW usage
- Far right: system/plugin status block

Rules:
- Remove dedicated play/pause control
- Utility buttons should use ghost or low-emphasis styling by default
- The current preset/workflow state should be easier to scan than generic utility actions
- Header must read as a framed premium module, not a plain strip

### 2. Input Strip

Purpose: visualize the incoming signal and orient the user before the sequencer rows.

Structure:
- Premium waveform/status band directly below the header
- Stronger framing than the current thin line treatment
- Clear start/playhead indicators and slice structure

Rules:
- The waveform strip should feel like a device display, not a debug graph
- Slice guides and current position should be readable at a glance

### 3. Main Sequencer Field

Purpose: primary interaction area for lane programming.

Structure:
- Left rail: lane identity chips
- Center: 16-step matrix as the dominant visual element
- Right rail: lane macro controls and values

Rules:
- Rows should feel like modules, not just labeled table rows
- Step cells should look like intentional blocks/capsules with state depth
- Active blocks should carry lane character without flooding the whole matrix with color
- The currently playing step must become visually obvious

### 4. Right Macro Rail

Purpose: per-lane quick control without opening deeper editing.

Structure:
- One macro cluster per lane
- Knob + numeric value + optional compact label or mode text

Rules:
- Controls should feel integrated into the row architecture
- Value readability matters more than decorative complexity
- Knobs should feel tactile and expensive, but still clean

### 5. Bottom Detail Dock

Purpose: contextual deep editing area for selected lane/step/effect.

Structure:
- Dock spans the full bottom width
- Content changes based on selection
- First implementation can be a framed placeholder/detail region, but it must establish the final visual architecture now

Rules:
- The dock is what moves the UI from prototype to instrument
- It should feel like the “editor layer” for the sequencer, similar in importance to lower panels in premium reference plugins
- It must visually belong to the rest of the UI, not look like an unrelated footer

## Visual Language

### Paneling

- Use nested dark surfaces with subtle elevation differences
- Outer shell stays darkest
- Major modules use slightly lifted surfaces
- Borders should be soft but intentional: thin outlines, low-contrast framing, and restrained neon accents
- Rounded corners are acceptable, but should feel engineered rather than playful

### Color Strategy

- Base canvas: deep teal-black / dark graphite-green surfaces
- Brand neon green: reserved for emphasis, active highlights, important meters, and critical controls
- Lane colors: row identity, active blocks, selected accents, control highlights
- White/off-white: key text and premium contrast moments

Do not:
- Fill large areas with neon green
- Use the same bright fill and text color on the same control state
- Let all controls compete for emphasis equally

### Typography

Hierarchy:
- Product/section identity: Anta or strongest display face
- Lane/module labels: VCR/mono style for character and instrument identity
- Values/data/supporting labels: Space Mono or similar mono treatment

Rules:
- Every label must be readable at rest, not only on hover or press
- Numeric values should look deliberate and technical
- Typography should carry part of the premium feel, not just function as a label layer

## Control Language

### Buttons

- Default: dark ghost buttons with readable text and subtle accent border
- Active/selected: brighter fill or stronger lane/brand accent with inverted text as needed
- Primary controls should be larger or more visually weighted than tertiary ones
- No state should ever depend on hover/press for basic legibility

### Step Cells

Required visual states:
- idle
- hover
- active
- selected/focused
- currently playing

Behavioral design intent:
- Idle state stays dark and quiet
- Active state becomes a saturated lane block
- Playing state gets a stronger outline/glow/readhead effect than active alone
- Selection state should enable the bottom dock relationship clearly

### Knobs and Values

- Knobs need better tactile framing, not just a ring
- Each knob needs a readable value readout
- Lane macro controls should look like premium lane instruments, not detached utility widgets

## Content Priorities For First UI Redesign Pass

The first implementation pass should focus on structure and quality, not feature breadth.

### Must include

1. Header redesign without play/pause
2. Stronger premium branding block
3. Framed input strip
4. Framed sequencer surface with clearer row architecture
5. Better step state styling
6. Improved macro rail presentation
7. Bottom detail dock as a real architectural element
8. Correct control readability everywhere

### May defer

- Full effect editor internals in the dock
- Advanced animation polish beyond basic current-step emphasis
- Deep preset browser flows
- Complex editor subviews per lane

## Immediate Implementation Priorities

1. **Header architecture**
   Remove the transport-style play button, rebalance button hierarchy, strengthen branding, and improve status/preset composition.

2. **Panel framing pass**
   Introduce premium containers for header, waveform strip, sequencer field, macro rail, and bottom dock.

3. **Sequencer row and step pass**
   Redesign row chips, step blocks, spacing, and playhead emphasis.

4. **Control polish pass**
   Improve knobs, labels, values, button readability, and row-level control clarity.

5. **Detail dock foundation**
   Establish the lower editor region visually and structurally, even if its first contents are limited.

## Success Criteria

The redesign succeeds when:

- the UI reads as a premium plugin, not a skinned prototype
- the sequencer is still clearly the main interaction surface
- branding feels intentional and product-like
- controls are readable in their idle state
- the layout resembles the quality level of the provided references in hierarchy and panel architecture
- the bottom dock clearly suggests a deeper editing layer for selected content

## Non-Goals

- Mimicking the exact colors or branding of the reference plugins
- Recreating a hardware-look skeuomorphic UI
- Packing every possible control into the first redesign pass
- Adding host transport behavior that belongs to the DAW

## Implementation Notes

- The design should reuse the existing Zikada brand colors, fonts, and logo asset, but with stronger composition and better usage discipline.
- Existing interactive foundations (step toggles, lane colors, knobs, waveform, footer controls) should be restyled and restructured rather than discarded unless a component fundamentally blocks the new architecture.
- The redesign should prefer modular component boundaries so header, dock, row styling, and macro controls can evolve independently.
