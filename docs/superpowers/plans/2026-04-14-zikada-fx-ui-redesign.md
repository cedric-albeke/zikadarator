# Zikada FX UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform Zikada FX into a premium sequencer-first plugin UI with a framed header, premium input strip, stronger lane/step hierarchy, improved macro rail, and a real bottom detail dock.

**Architecture:** Keep the existing JUCE component structure, but restyle and rebalance it around a panel-based layout. Treat the header, waveform strip, sequencer field, macro rail, and bottom dock as distinct premium modules while preserving current interaction foundations.

**Tech Stack:** JUCE 8, C++20, CMake, custom LookAndFeel components

---

## File Structure

- `src/ui/ZikadaLookAndFeel.h/.cpp` — global color, typography, button, panel, and slider styling
- `src/ui/panels/HeaderPanel.h/.cpp` — DAW-first command header and branding
- `src/ui/panels/SequencerPanel.h/.cpp` — top-level sequencer layout and panel framing
- `src/ui/components/StepGrid.h/.cpp` — lane rails, matrix composition, right macro rail integration
- `src/ui/components/StepCell.h/.cpp` — premium step states: idle, hover, active, selected, playing
- `src/ui/components/Knob.h/.cpp` — macro control visuals and readouts
- `src/ui/panels/FooterPanel.h/.cpp` — convert simple footer into the first detail dock shell
- `src/PluginEditor.cpp` — top-level proportions if needed

## Task 1: Header redesign without transport

**Files:**
- Modify: `src/ui/panels/HeaderPanel.h`
- Modify: `src/ui/panels/HeaderPanel.cpp`
- Modify: `src/ui/ZikadaLookAndFeel.cpp`

- [ ] Remove the play button from `HeaderPanel` and rebalance the remaining controls around preset/workflow/status rather than transport.
- [ ] Strengthen the branding block with the cicada logo, product wordmark, and compact subtitle.
- [ ] Convert idle header buttons to readable ghost buttons and keep active/selected states premium but restrained.
- [ ] Verify the header no longer has overlap between actions and system status.

## Task 2: Panel framing pass

**Files:**
- Modify: `src/ui/ZikadaLookAndFeel.h`
- Modify: `src/ui/ZikadaLookAndFeel.cpp`
- Modify: `src/ui/panels/SequencerPanel.cpp`
- Modify: `src/PluginEditor.cpp`

- [ ] Introduce a reusable visual language for premium panels: dark shell, lifted inner surfaces, restrained outlines, and accent separators.
- [ ] Reframe the waveform/input strip so it reads like a device display rather than a flat band.
- [ ] Adjust top-level layout spacing so header, sequencer region, and dock feel like intentional modules.

## Task 3: Sequencer row and step redesign

**Files:**
- Modify: `src/ui/components/StepGrid.cpp`
- Modify: `src/ui/components/StepCell.h`
- Modify: `src/ui/components/StepCell.cpp`

- [ ] Strengthen lane chips/row identity with better framing, label rhythm, and row separation.
- [ ] Upgrade step cells into premium blocks with clearer depth and stronger active/selected/playing distinction.
- [ ] Add visual bar rhythm or grouping cues across the 16-step matrix.
- [ ] Ensure typography and step readability remain strong in idle state.

## Task 4: Macro rail polish

**Files:**
- Modify: `src/ui/components/Knob.h`
- Modify: `src/ui/components/Knob.cpp`
- Modify: `src/ui/components/StepGrid.cpp`

- [ ] Make the macro rail feel integrated into each row instead of detached.
- [ ] Improve knob framing, labels, and numeric value treatment.
- [ ] Make lane macro controls feel premium and consistent across all rows.

## Task 5: Bottom detail dock foundation

**Files:**
- Modify: `src/ui/panels/FooterPanel.h`
- Modify: `src/ui/panels/FooterPanel.cpp`
- Modify: `src/ui/panels/SequencerPanel.cpp`

- [ ] Convert the current footer strip into a real dock shell that visually anchors the bottom of the UI.
- [ ] Preserve current dry/wet, mode, gain, and bypass controls, but regroup them as part of the dock architecture.
- [ ] Add placeholder/detail framing so the dock clearly suggests deeper lane/step editing.

## Task 6: Verification and standalone review

**Files:**
- Verify only

- [ ] Build `ZikadaFX_Standalone` from the worktree.
- [ ] Relaunch the standalone from the worktree build.
- [ ] Verify readability, panel hierarchy, branding presence, and overall premium feel against the approved spec.
