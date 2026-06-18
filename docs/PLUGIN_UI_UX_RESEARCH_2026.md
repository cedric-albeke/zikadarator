# ZIKADARATOR V1 Plugin UI/UX Research - 2026 Pass

Date: 2026-06-18

## Reference Products

- Cableguys ShaperBox 3: drawable modulation, audio-triggered movement, clear effect modules, and immediate motion feedback. Source: https://www.cableguys.com/shaperbox
- Cableguys TimeShaper: focused time effects, visible time offset feedback, resizable/HiDPI UI, hover help, oscilloscope, dry/wet per band, and smooth bypass. Source: https://www.cableguys.com/timeshaper
- Sugar Bytes Effectrix 2: paint-the-matrix workflow, 32-step sequencing, quick random/preset exploration, per-step modulation, and direct performance patterns. Source: https://sugar-bytes.de/effectrix2
- Sugar Bytes Effectrix: playful block painting for looping, scratching, reverse, stretching, and modulation tracks. Source: https://sugar-bytes.de/effectrix
- Devious Machines Infiltrator 2: stacked/sequenced effect modules, large effect taxonomy, random/Euclidean/swing helpers, and per-module modulation. Source: https://deviousmachines.com/product/infiltrator/
- MusicRadar Infiltrator coverage: reinforces the modern expectation that step FX plugins expose sequencer, modulation, macros, presets, and randomization as first-class performance controls. Source: https://www.musicradar.com/news/could-devious-machines-infiltrator-multi-effect-sequencer-plugin-be-the-shaper-of-things-to-come

## 2026 Pattern Summary

1. The sequencer is the product. In modern step-FX plugins, the grid or curve editor is the largest, clearest object. Secondary docks should serve it, not compete with it.
2. Visual feedback is expected, but it should explain the sound. Waveforms, playheads, active-module meters, and modulation curves matter more than decorative glow.
3. Preset discovery is fast and playful. Users expect quick randomization, favorites/recents, and readable preset names before they expect huge icon grids.
4. Effect identity must be honest. Do not advertise scratch, vinyl stop, or spectral FX until the DSP exists and is mapped to presets.
5. Dense DAW plugins need strong hierarchy. Reserve the loudest neon and bevel treatment for active lanes, selected steps, transport/playhead states, and currently edited controls.
6. Resizing and HiDPI polish are table stakes. Controls must remain readable at compact DAW sizes and should not require hover-only knowledge.
7. Performance matters. Repaint only changed regions, cache static panel/grid layers where possible, and keep animation purposeful.

## Current ZIKADARATOR Culprits

1. Panel depth is over-applied. `drawPremiumPanel` and `drawDeviceDisplay` make too many passive areas feel equally important.
2. The sequencer is crowded by the 280px sidebar and 150px footer. The grid should become visually dominant.
3. Header density is high: logo, tabs, preset browser, undo/redo, and now CRT all need stricter grouping.
4. Step cells stack too many state signals: fill, glow, icon, border, hover, playing, selected, chain dots.
5. Sidebar preset recognition is weak because the grid relies heavily on icons and hover text.
6. Footer knobs need more spacing and stronger grouping by what the current lane actually uses.
7. The UI still reads like a table in places. Beat grouping and lane grouping should feel more musical.
8. Animation should be region-scoped. The CRT display is safe because it repaints only its own bounds.

## Applied In This Slice

- Added a small CRT-style `FX MON` display in the header, placed between the wordmark and sequencer tabs as marked in the screenshot.
- The monitor shows selected step/preset state and pulses from the real playhead lane-activity mask.
- The display uses lane colors, scanlines, and a phosphor trace for Zikada character without claiming unsupported DSP.
- Added source smoke guards so the display API, painter, and editor wiring remain present.
- Reworked the sidebar preset browser from a flat five-column icon wall into a four-column factory grid with visible short labels and a separated U1-U4 user-slot row.
- Added keyboard focus, arrow navigation, visible focus ring, and Space/Return activation for the sidebar preset browser.
- Expanded the sidebar selected-preset readout into a roomier device-display panel with separate title/body labels and a lane-color accent.
- Centralized StepGrid layout metrics and added subtle four-step beat-group backplates so the sequencer reads as musical phrases, not only a table.
- Quieted passive sequencer framing and step-cell glow so selected, playing, hovered, and active cells keep the strongest hierarchy.

## Recommended Next Slices

1. Sequencer hierarchy follow-up: continue evaluating selected/playing/tied state priority in real DAW sizes after the passive glow reduction.
2. Sidebar follow-up: add direct lane-category grouping if the preset set grows beyond 16 factory options.
3. Footer rework: split selected-step controls into lane-aware groups with more knob spacing and fewer always-visible globals.
4. Engine FX slice: implement one real groove effect before adding labels. The lowest-risk first slice is real Slice-lane reverse/repeat/stutter/retrigger playback, because the UI already exposes those concepts and the existing `SliceEngine` can be extended without promising vinyl/tape-stop yet.
5. Performance slice: cache static grid/panel layers and repaint only old/new playhead columns plus active CRT bounds.

## FX Roadmap Integrity Notes

- Existing real groove pieces: Slice forward/reverse/repeat/stutter playback modes, Loop forward/reverse windows, loop speed changes, long tails, Envelope attack/decay/swell/fade/tremolo/wobble/punch/snap/glide/hold shapes, FX2 short wobble/modulated delay, FX1 tremolo, and global ducking/sidechain mix modes.
- Misaligned or incomplete promises: vinyl/tape-stop/scratch should not be advertised until dedicated DSP exists.
- Recommended DSP implementation path: deepen Slice scatter/retrigger click-safety, then rate-glide/tape-stop in the Loop/time path, then vinyl scratch once there is a deliberate resampling/scrub model.
