# ZIKADARATOR V1 Stability Roadmap

This is the current priority map for getting ZIKADARATOR to a stable V1.

## Fixed in the Current Gate

- Step grid APVTS/state sync for click activation, drag-copy painting, and Shift-drag chaining.
- Factory preset active-step state now mirrors APVTS step-active parameters.
- Factory presets now reset lane mute and solo parameters.
- Processor global controls are wired through the engine-rebuild audio path: dry/wet, output gain, bypass, mix mode, clock source, tempo, and step resolution.
- `processBlock` is split through `StepScheduler` segment rendering, and source-smoke tests guard the audio thread against editor access, logging, and heap-owned buffer creation.
- Windows validation gate builds tests, runs CTest, builds VST3, and runs pluginval level 5.
- macOS validation gate builds tests, runs CTest, builds VST3/AU/Standalone, runs VST3 and AU pluginval, and runs `auval`.
- Windows CI is pinned to `windows-2022` for Visual Studio 2022 and explicitly installs/probes Inno Setup.
- macOS CI pins a macOS 11.0 deployment target and uploads validation logs.
- Waveform capture is now processor-owned: audio-thread waveform taps publish input and processed output for the editor timer.
- `SequencerState` now publishes plain-value snapshots through fixed buffers, removing `atomic<shared_ptr>` refcount traffic from the audio path.
- The branch is rebased onto the newer `codex/windows-engine-rebuild` line, preserving the rebuilt engine core and newer FX preset/icon work.
- Mono processing now renders with an independent right-side scratch channel and folds stereo wet/dry output back to mono, covered by a processor regression test.

## V1 Blockers

1. Make the sequencer usable at every supported editor size.
   The 900x600 minimum is visually cramped. Either raise the minimum, add a compact breakpoint, or scale the 1200x800 logical layout.

## Next AAA UI/UX Polish

1. Replace the header preset "dropdown opens browser" behavior with an anchored quick menu: favorites, recents, full list, and "Open browser...".
2. Add mouse wheel preset cycling on hovered/selected step cells.
3. Add keyboard focus, arrow navigation, and visible focus rings for custom-painted controls.
4. Persist sidebar preset details on selection instead of requiring hover.
5. Add unit-aware knob display and mappings: Hz as log scale, resonance as decimal, mix as percent, pan as L/C/R.
6. Add a column-wide playhead rail in the step grid and remove double framing around the waveform.

## Release Hardening

1. Run the macOS validation workflow on a real macOS runner; Windows can only syntax-check the script.
2. Add Steinberg VST3 validator paths to Windows/macOS validation where available.
3. Add host-state roundtrip and additional host-layout processor regression tests.
4. Add signing/notarization paths for public V1 artifacts; keep unsigned packages labeled as tester builds.
