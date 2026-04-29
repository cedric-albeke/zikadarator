# ZIKADARATOR Engine Rebuild

This document tracks the practical state of the Windows-native audio-engine rebuild. The detailed task plan is in `docs/superpowers/plans/2026-04-28-engine-rebuild.md`; this file is the shorter human-readable status note.

## Goal

Move ZIKADARATOR from a UI-heavy prototype toward a realtime-safe, host-synced FX sequencer that can survive Ableton Live usage and strict plugin validation.

## Implemented

- Added deterministic engine tests through `ZikadaEngineTests`.
- Added `RealtimeRingBuffer` for overwrite-style audio history with integer and linear-interpolated reads.
- Added `StepScheduler` for host/free-clock block splitting at step boundaries.
- Added `WaveformTap` so waveform rendering no longer requires direct editor access from the audio thread.
- Added separate input and processed-output waveform taps so the signal display can compare dry input against the affected output.
- Refactored `PluginProcessor::processBlock` into scheduler-driven `processSegment` calls.
- Added 1/16 step resolution while keeping 1/8 as the default for existing behavior.
- Reused owned dry/wet/slice scratch buffers instead of allocating temporary vectors inside the main audio block path.
- Ported slice and loop history to realtime ring buffers.
- Rebuilt the LOOP lane preset map around implemented micro-loop behaviors: forward/reverse note windows, speed variants, slow variants, reverse speed variants, and long tails.
- LOOP lane U1-U4 slots now use semantic loop controls (`LEN`, `RATE`, `REV`, `FADE`, `MIX`, `VOL`, `PAN`) instead of filter/delay labels.
- `LoopEngine` crossfades loop wrap and trigger edges to reduce clicks from arbitrary loop boundaries.
- `LoopEngine` now freezes a triggered loop into an internal snapshot buffer. Loop playback no longer chases later rolling input, so stutter/repeat presets stay audibly tied to the pattern shown by their icon.
- Lane mix now has a defined per-lane dry/wet contract: each active lane snapshots the signal entering that lane, processes the lane, then blends the lane output against that lane input. Turning down LOOP/FX/FILTER mix no longer attenuates the whole track or earlier lanes in the chain.
- Replaced integer delay with JUCE fractional delay and smoothed delay, mix, and feedback controls.
- Corrected filter contracts:
  - Low/high 12 dB use one TPT state-variable section.
  - Low/high 24 dB use cascaded TPT sections.
  - Band reject is dry minus bandpass, not negative bandpass.
  - Comb has an implemented fractional-delay path.
- Fixed the plugin editor to a 3:2 aspect ratio.
- Improved UI hot paths by avoiding full-grid repaint from the chain-animation timer and caching the Windows/Wine renderer check.
- The stacked signal display now keeps rolling input/output waveform histories, displays one BPM/step-resolution-derived 16-step window, and applies display-only normalization so quiet material still reads as waveform shape instead of a flat meter line.
- Added `scripts/ableton-log-scan.ps1` for Windows Ableton/ZIKADARATOR log review.
- Expanded the alpha factory bank to eight presets that use only implemented DSP paths.

## Implementation References

- JUCE `AudioProcessorGraph` cascade guidance keeps active processors connected in series from input to output; ZIKADARATOR currently keeps that serial chain explicit in `PluginProcessor::processSegment` until the lane classes are split out.
- JUCE `dsp::DryWetMixer` defines dry/wet as pushing dry samples and mixing fully wet samples back into them. The lane mix helper mirrors that contract at lane scope.
- ChowDSP BYOD and DISTRHO Cardinal were reviewed as open-source references for production plugin architecture: BYOD is a customizable signal-chain effects plugin, while Cardinal packages a modular engine as a self-contained plugin. Both reinforce the same direction for ZIKADARATOR: explicit routing contracts and stable processing modules rather than UI-only preset labels.

Links:

- https://juce.com/tutorials/tutorial_audio_processor_graph/
- https://docs.juce.com/master/classjuce_1_1dsp_1_1DryWetMixer.html
- https://github.com/Chowdhury-DSP/BYOD
- https://github.com/DISTRHO/Cardinal

## Current Verification

Last verified locally on Windows on 2026-04-29:

```powershell
node scripts\source-smoke-tests.mjs
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target ZikadaEngineTests
.\build\Release\ZikadaEngineTests.exe
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Release --output-on-failure
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
- Ableton Live 12 manual audio acceptance still needs a documented pass after the next installed build is tested in-session. Do not replace the system VST3 while Live is open with an unsaved project.
- The signal display now shows two stacked rolling waveform lanes over one musical 16-step window with display-only normalization. The next step is stronger per-effect visual annotation so loop/reverse/stutter actions read more explicitly during playback.
- Advanced pitch/time/grain preset families remain deferred until a real library is integrated.

## Next Tasks

1. Close Ableton, install the latest Release VST3, and run the Ableton Live 12 manual acceptance test.
2. Audition the frozen-loop LOOP lane presets against simple drum and melodic loops; revise any mapping that does not sound like its icon.
3. Continue improving perceived responsiveness by profiling sidebar interactions, footer knob updates, and waveform repaint cost.
4. Continue splitting lane processors out of `PluginProcessor.cpp` once audible behavior is stable.
