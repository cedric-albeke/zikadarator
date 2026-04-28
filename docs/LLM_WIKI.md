# ZIKADARATOR LLM Wiki

This is a compact handoff file for future AI coding sessions. Prefer it over reconstructing project state from chat history.

## Repository

- Root: `C:\Development\zikadarator`
- Primary remote: `origin` -> `https://github.com/cedric-albeke/zikadarator.git`
- Main branch at time of writing: `master`
- Stack: JUCE 8.0.6, CMake, C++20, VST3/AU/Standalone
- Current active rebuild plan: `docs/superpowers/plans/2026-04-28-engine-rebuild.md`

## Build And Test Commands

Use Visual Studio Build Tools CMake directly if plain `cmake` is not on `PATH`.

```powershell
node scripts\source-smoke-tests.mjs
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target ZikadaEngineTests
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Debug --output-on-failure
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target ZikadaFX_VST3
.tools\pluginval\pluginval.exe --validate-in-process --strictness-level 5 --validate "C:\Development\zikadarator\build\ZikadaFX_artefacts\Release\VST3\ZIKADARATOR.vst3"
```

## Current Engine Shape

- `PluginProcessor::processBlock` is the top-level audio entry point.
- `StepScheduler` splits each block into sample-accurate segments using host PPQ or free clock.
- `PluginProcessor::processSegment` runs lane logic for one segment.
- `RealtimeRingBuffer` is the audio-history primitive for slice/loop history.
- `WaveformTap` is the only audio-thread-to-UI waveform handoff.
- `DelayEngine` uses JUCE `dsp::DelayLine<float, Linear>` with smoothed delay, mix, and feedback.
- `FilterEngine` has real 12/24 dB differences, a real band-reject path, and a comb path.
- `PluginEditor` is resizable but constrained to a fixed 3:2 aspect ratio.

## Realtime Constraints

Keep these invariants unless there is a deliberate architecture change:

- No editor/component access from `processBlock`.
- No file logging from `processBlock`.
- No heap allocation in normal audio-block processing.
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

- Pitch, stretch, grain, vinyl, and chaos labels overpromise compared to the current `PitchEngine`.
- `PitchEngine` is a simple experimental ring-buffer pitch-color processor, not production time-stretch or granular DSP.
- Dedicated lane processor classes are still deferred; orchestration still lives mostly in `PluginProcessor.cpp`.
- Ableton Live 12 manual acceptance needs to be repeated after each installed VST3 build.
- Factory presets need an alpha pass so labels and audible behavior match implemented DSP.

## Best Next Task

Continue with the rebuild plan Task 8:

1. Rename misleading preset labels in `src/ui/panels/SidebarPanel.cpp`.
2. Add source-smoke checks that block `Stretch`, `Grain`, `Vinyl`, and `Chaos` labels until real DSP exists.
3. Document `PitchEngine` as experimental.
4. Build Release VST3 and run pluginval.

After Task 8, do Task 9:

1. Create `scripts/ableton-log-scan.ps1`.
2. Update `docs/RELEASE_TESTING.md` and `docs/QUICKSTART.md`.
3. Run it after an Ableton Live 12 manual loop test.

## Ableton Log Scan

Windows log scanner:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ableton-log-scan.ps1
```

It locates the newest Ableton Live 12 log under `%APPDATA%\Ableton`, scans `%APPDATA%\ZIKADARATOR\UI-Debug.log`, and lists recent Ableton usage logs. Treat crash/fatal/exception/restore-failure lines as blockers before sharing tester builds.
