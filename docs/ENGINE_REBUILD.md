# ZIKADARATOR Engine Rebuild

This document tracks the practical state of the Windows-native audio-engine rebuild. The detailed task plan is in `docs/superpowers/plans/2026-04-28-engine-rebuild.md`; this file is the shorter human-readable status note.

## Goal

Move ZIKADARATOR from a UI-heavy prototype toward a realtime-safe, host-synced FX sequencer that can survive Ableton Live usage and strict plugin validation.

## Implemented

- Added deterministic engine tests through `ZikadaEngineTests`.
- Added `RealtimeRingBuffer` for overwrite-style audio history with integer and linear-interpolated reads.
- Added `StepScheduler` for host/free-clock block splitting at step boundaries.
- Added `WaveformTap` so waveform rendering no longer requires direct editor access from the audio thread.
- Refactored `PluginProcessor::processBlock` into scheduler-driven `processSegment` calls.
- Reused owned dry/wet/slice scratch buffers instead of allocating temporary vectors inside the main audio block path.
- Ported slice and loop history to realtime ring buffers.
- Replaced integer delay with JUCE fractional delay and smoothed delay, mix, and feedback controls.
- Corrected filter contracts:
  - Low/high 12 dB use one TPT state-variable section.
  - Low/high 24 dB use cascaded TPT sections.
  - Band reject is dry minus bandpass, not negative bandpass.
  - Comb has an implemented fractional-delay path.
- Fixed the plugin editor to a 3:2 aspect ratio.
- Added `scripts/ableton-log-scan.ps1` for Windows Ableton/ZIKADARATOR log review.
- Expanded the alpha factory bank to eight presets that use only implemented DSP paths.

## Current Verification

Last verified locally on Windows:

```powershell
node scripts\source-smoke-tests.mjs
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target ZikadaEngineTests
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Debug --output-on-failure
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target ZikadaFX_VST3
.tools\pluginval\pluginval.exe --validate-in-process --strictness-level 5 --validate "C:\Development\zikadarator\build\ZikadaFX_artefacts\Release\VST3\ZIKADARATOR.vst3"
```

Expected results:

- Source smoke: pass
- Engine tests: pass
- CTest: pass
- Release VST3 build: pass
- pluginval strictness 5: pass

## Known Limitations

- Pitch/time/stretch/grain/vinyl style families are intentionally not advertised in the alpha UI until real DSP exists.
- Lane processors are still orchestrated in `PluginProcessor.cpp`; a cleaner lane-processor split remains planned.
- Ableton Live 12 manual audio acceptance still needs a documented pass after the next installed build is tested in-session.
- Advanced pitch/time/grain preset families remain deferred until a real library is integrated.

## Next Tasks

1. Run the Ableton Live 12 manual acceptance test and archive scanner output.
2. Run the factory bank through Ableton and remove or revise any preset that clicks or underperforms.
3. Continue splitting lane processors out of `PluginProcessor.cpp` once audible behavior is stable.
