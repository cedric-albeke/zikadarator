# Audio Routing Stabilization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make ZIKADARATOR's lane routing behave like a serial multi-FX chain where lane mix blends that lane only, not the whole plugin output.

**Architecture:** Add a tiny reusable mixing helper, then change `PluginProcessor::processSegment` so each active lane snapshots its input, processes the lane, and dry/wet blends the lane output back into the chain. Keep the existing lane order and engines intact while removing the destructive `left *= mix` behavior. Use offline tests and source-smoke checks because subjective Ableton auditioning cannot be automated from this shell.

**Tech Stack:** JUCE 8, C++20, CMake/MSBuild, `ZikadaEngineTests`, `pluginval`.

---

### Task 1: Pin Lane Mix Semantics

**Files:**
- Create: `src/engine/MixUtils.h`
- Modify: `tests/engine/LaneTransitionTests.cpp`
- Modify: `scripts/source-smoke-tests.mjs`

- [x] **Step 1: Write failing test**
  Add an engine test proving `0%` lane mix returns lane input, `50%` blends input and processed output, and `100%` returns processed output.

- [x] **Step 2: Run test**
  `cmake --build build --config Release --target ZikadaEngineTests --parallel; build\Release\ZikadaEngineTests.exe`
  Expected before implementation: compile failure because `engine/MixUtils.h` is missing.

- [x] **Step 3: Implement `MixUtils.h`**
  Provide `blendLaneMixSample(dry, wet, mix)` and `applyLaneMix(...)`.

- [x] **Step 4: Run test**
  Expected after implementation: new mix utility test passes.

### Task 2: Fix Processor Lane Routing

**Files:**
- Modify: `src/PluginProcessor.h`
- Modify: `src/PluginProcessor.cpp`
- Modify: `scripts/source-smoke-tests.mjs`

- [x] **Step 1: Add source-smoke guard**
  Fail if `processSegment` still contains the destructive `left[i] *= mix` / `right[i] *= mix` lane-mix pattern.

- [x] **Step 2: Add lane input scratch buffers**
  Add `laneInputLeftBuffer` and `laneInputRightBuffer`, size them in `ensureScratchBuffers`.

- [x] **Step 3: Blend per lane**
  Before each active lane processor, copy the current chain into lane input scratch. After the lane processor, call `applyLaneMix` to blend lane input and lane output.

- [x] **Step 4: Preserve capture semantics**
  Keep slice and loop history capture from the current chain, but do not use lane mix as gain.

### Task 3: Verify And Install

**Files:**
- Modify: `docs/ENGINE_REBUILD.md`
- Modify: `docs/LLM_WIKI.md`

- [x] **Step 1: Run verification**
  Run source-smoke, engine tests, CTest, Release VST3 build, and pluginval strictness 5.

- [ ] **Step 2: Install**
  Replace the system VST3 with the new Release build and clear `%APPDATA%\ZIKADARATOR\UI-Debug.log`.

  Deferred in this pass because Ableton Live 12 was running with a modified set. Do not force-close Live or overwrite the plugin while the host may have the VST3 loaded.

- [x] **Step 3: Log baseline**
  Run `scripts\ableton-log-scan.ps1` and record the outcome in the final report.

### Additional Follow-Up Completed

- [x] Added a `LoopEngine` regression proving a triggered loop freezes captured audio instead of chasing later rolling input.
- [x] Changed `LoopEngine` to snapshot triggered loops into owned buffers and round loop durations to sample counts.
- [x] Reduced grid animation repaint cost by repainting tied cells instead of the whole grid timer path.
- [x] Added display-only waveform normalization for clearer input/output waveform lanes.
