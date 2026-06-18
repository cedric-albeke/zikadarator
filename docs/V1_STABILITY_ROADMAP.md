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
- The editor now lays out the authored 1200x800 Zikada interface on a logical canvas and scales it to supported host sizes, preventing the compact view from crushing fixed panels.
- The header preset control now opens an anchored quick menu with favorites, recents, the full preset list, and a direct browser command instead of forcing a page switch.
- Sidebar preset details now persist from the selected step/preset and temporarily preview hover targets, so the detail area stays useful after the pointer leaves the grid.
- Step cells now support mouse-wheel preset cycling on the hovered or selected cell, with undo snapshotting, sidebar/footer refresh, APVTS active-state sync, and dirty-preset tracking.
- Footer knobs now render real units and move through the right scale: cutoff uses Hz/kHz with logarithmic travel, resonance uses decimals, timing uses milliseconds/seconds, mix-style controls use percentages, volume/rate controls use multipliers, and pan uses L/C/R readouts.
- Processor rendering now captures one immutable `SequencerState::Snapshot` per block and renders segments from that snapshot instead of reading mutable UI-owned sequencer state directly on the audio thread.
- Keyboard focus, arrow navigation, visible focus rings, and space/return activation are now implemented on the StepGrid. Off-cell wheel preset cycling requires an explicit Shift gesture.

## V1 Blockers

1. Finish the AAA interaction polish pass.
1. The compact canvas, header quick menu, persistent sidebar details, wheel preset cycling, unit-aware knobs, and keyboard focus/navigation are in place, but the playhead rail and waveform framing still need dedicated passes.

## Next AAA UI/UX Polish

1. Add a column-wide playhead rail in the step grid and remove double framing around the waveform.

## Release Hardening

1. Run the macOS validation workflow on a real macOS runner; Windows can only syntax-check the script.
2. Add Steinberg VST3 validator paths to Windows/macOS validation where available.
3. Add host-state roundtrip and additional host-layout processor regression tests.
4. Add signing/notarization paths for public V1 artifacts; keep unsigned packages labeled as tester builds.
