# ZIKADARATOR LLM Wiki

This is a compact handoff file for future AI coding sessions. Prefer it over reconstructing project state from chat history.

## Repository

- Root: `C:\Development\zikadarator`
- Primary remote: `origin` -> `https://github.com/cedric-albeke/zikadarator.git`
- Main branch at time of writing: `master`
- Stack: JUCE 8.0.6, CMake, C++20, VST3/AU/Standalone
- Current active rebuild plans:
  - `docs/superpowers/plans/2026-04-28-engine-rebuild.md`
  - `docs/superpowers/plans/2026-04-29-audio-routing-stabilization.md`

## Build And Test Commands

Use Visual Studio Build Tools CMake directly if plain `cmake` is not on `PATH`.

```powershell
node scripts\source-smoke-tests.mjs
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target ZikadaEngineTests
.\build\Release\ZikadaEngineTests.exe
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Release --output-on-failure
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target ZikadaFX_VST3
.tools\pluginval\pluginval.exe --validate-in-process --strictness-level 5 --validate "C:\Development\zikadarator\build\ZikadaFX_artefacts\Release\VST3\ZIKADARATOR.vst3"
```

## Current Engine Shape

- `PluginProcessor::processBlock` is the top-level audio entry point.
- `StepScheduler` splits each block into sample-accurate segments using host PPQ or free clock.
- `PluginProcessor::processSegment` runs lane logic for one segment.
- Lane mix is not output gain. Each active lane snapshots its own input, processes the lane, then uses `blendLaneMixSample()` / `applyLaneMix()` to dry/wet blend that lane result back into the chain.
- `RealtimeRingBuffer` is the audio-history primitive for slice/loop history.
- `WaveformTap` is the only audio-thread-to-UI waveform handoff. The processor owns one input tap and one processed-output tap.
- `DelayEngine` uses JUCE `dsp::DelayLine<float, Linear>` with smoothed delay, mix, and feedback.
- `FilterEngine` has real 12/24 dB differences, a real band-reject path, and a comb path.
- `PluginEditor` is resizable but constrained to a fixed 3:2 aspect ratio.
- `stepResolution` choices are `1/16`, `1/8`, `1/4`, `1/2`; the APVTS default remains `1/8` at index 1.
- LOOP lane presets 5-20 are implemented as forward/reverse note windows, speed variants, slow variants, reverse speed variants, Tail 4 Beat, and Tail Max. Prepared history is 12 seconds: four user-controlled beats fit at 20 BPM, while Tail Max documents its cap. Do not reintroduce placeholder "Loop Alt" labels.
- LOOP lane U1-U4 slots are semantic: `filterCutoff` stores length in beats, `filterResonance` stores playback rate, `delayTime` stores reverse amount, `delayFeedback` stores fade/smoothing, and `delayMix` stores wet mix.
- `LoopEngine` has frozen trigger snapshots, loop-wrap smoothing, trigger-edge smoothing, and rounded loop-duration sample counts. Keep the snapshot/discontinuity regression tests in `LaneTransitionTests.cpp` when changing playback.
- `WaveformDisplay` has two stacked rolling min/max waveform lanes, a visible sample window synced to the current 16-step musical loop span, and display-only normalization through `displayGain`.
- `StepGrid::timerCallback()` must not repaint the whole grid. It only repaints tied/chain-consumed cells for chain animation.
- StepGrid keyboard parity is part of the V1 contract: Page Up/Down cycles presets and Shift+Left/Right resizes ties. Keep these commands in accessibility help, preserve one undo boundary per edit, resolve consumed steps to their chain root, and never extend a tie across an active step.

## Realtime Constraints

Keep these invariants unless there is a deliberate architecture change:

- No editor/component access from `processBlock`.
- No file logging from `processBlock`.
- No heap allocation in normal audio-block processing.
- Lane mix must not contain `left[i] *= mix` / `right[i] *= mix` style attenuation of the shared wet chain.
- Use fixed-size stack segment buffers or pre-owned scratch buffers in the processor.
- `juce::AbstractFifo` is acceptable for UI telemetry, not for audio history playback semantics.

The source guard in `scripts/source-smoke-tests.mjs` checks the most important invariants.

## Current Verification Coverage

- `tests/engine/RealtimeRingBufferTests.cpp`
- `tests/engine/StepSchedulerTests.cpp`
- `tests/engine/LaneTransitionTests.cpp`
- `scripts/source-smoke-tests.mjs`
- pluginval strictness 5 against the Release VST3

## Known Product/Code Gaps

- Runtime preset labels and icons are aligned with the implemented DSP. Stretch, grain, vinyl/scratch, ChaosSynth, tonal quantization, and true phaser remain product-roadmap terms only and must not be added to the runtime UI without dedicated engines.
- `PitchEngine` is a simple experimental ring-buffer pitch-color processor, not production time-stretch or granular DSP.
- Dedicated lane processor classes are still deferred; orchestration still lives mostly in `PluginProcessor.cpp`.
- Ableton Live 12 manual acceptance needs to be repeated after each installed VST3 build. If Live is open with an unsaved set, build and pluginval the artifact but do not force-replace the system VST3.
- Alpha factory presets are intentionally limited to implemented DSP paths.
- The signal display now has stacked rolling input/output waveform lanes over the current 16-step musical loop span with display-only normalization, but it still needs richer per-effect visual annotation during playback.

## Alpha Factory Bank

Current embedded factory presets:

- `INIT`
- `NEON GATE`
- `SPACE BLOOM`
- `DELAY PULSE`
- `FILTER CUTS`
- `CRUSH GRID`
- `LOOP CHOP`
- `NOTCH MOTION`

These should stay conservative until the next Ableton acceptance pass confirms they are click-safe and musically useful.

## Best Next Task

Task 8 and the factory-bank slice of Task 10 are done. Best next task:

1. Close Ableton Live, then install the latest Release VST3 into the system VST3 folder.
2. Run the Ableton Live 12 acceptance loop with `ZIKADARATOR_DEBUG_LOG=1` and a clean `UI-Debug.log` when diagnostic log capture is needed.
3. Use `scripts/ableton-log-scan.ps1` to review logs.
4. Audition the LOOP lane presets specifically and adjust icons/audio mappings where the audio-visual link is weak.
5. Fix any audible clicks, restore issues, or log storms found in Live.

## Ableton Log Scan

Windows log scanner:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ableton-log-scan.ps1
```

For a strict diagnostic-log pass after setting `ZIKADARATOR_DEBUG_LOG=1`:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ableton-log-scan.ps1 -RequireZikadaLog
```

It locates the newest Ableton Live 12 log under `%APPDATA%\Ableton`, scans `%APPDATA%\ZIKADARATOR\UI-Debug.log` when diagnostic logging is explicitly enabled with `ZIKADARATOR_DEBUG_LOG=1`, and lists recent Ableton usage logs. Treat crash/fatal/exception/restore-failure lines as blockers before sharing tester builds.
