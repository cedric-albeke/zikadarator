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

- Pitch/time/stretch/grain/vinyl style behavior is still experimental or mislabeled in places. For alpha reliability, either rename these presets to what the current DSP actually does or integrate a proper pitch/time library such as Signalsmith Stretch in a later branch.
- Lane processors are still orchestrated in `PluginProcessor.cpp`; a cleaner lane-processor split remains planned.
- Ableton Live 12 manual audio acceptance still needs a documented pass after the next installed build is tested in-session.
- Preset/state contract still needs an alpha freeze so the factory set only promises implemented DSP.

## Next Tasks

1. Align pitch/time/grain/vinyl/chaos UI labels with implemented DSP.
2. Add Ableton log scanner and manual Live 12 acceptance checklist.
3. Freeze alpha preset/state scope around implemented processors.
4. Update factory presets to favor stable slice, loop, envelope, delay, filter, reverb, and bitcrush behavior.
