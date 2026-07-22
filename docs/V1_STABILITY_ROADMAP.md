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
- Filter-lane modulation now applies the advertised cutoff, resonance, volume, and pan targets instead of only consuming cutoff.
- Footer MOD mode is now enabled only on lanes with backed modulation targets, and saved unsupported targets are sanitized to `OFF` instead of being advertised in the UI.
- SliceEngine now applies a bounded realtime-safe edge fade to frozen slice playback, reducing hard start/end discontinuities on reverse, repeat, and stutter slices.
- Keyboard focus, arrow navigation, visible focus rings, and space/return activation are now implemented on the StepGrid. Off-cell wheel preset cycling requires an explicit Shift gesture.
- A column-wide playhead rail is now drawn in the step grid at the current playing step, spanning all lanes with a subtle neon glow.
- StepGrid layout metrics are centralized across painting, resizing, hit testing, and playhead drawing, and each lane now has subtle four-step beat-group backplates to make the sequencer read more musically.
- Passive sequencer framing and inactive/active step glow have been reduced so the grid hierarchy favors selected, playing, hovered, and active musical events over decoration.
- StepGrid now caches its static ruler, lane chips, beat-group backplates, and control-strip chrome; playhead movement repaints only the previous and current step columns, and chain animation timers run only while chains exist.
- Footer selected-step controls now have lane-aware group headers, making the seven knobs scan as related control families instead of a flat strip.
- Footer selected-step and modulation controls now stay hidden until a step is selected, keeping the empty state honest and preventing default-looking controls from implying active edits.
- Waveform display double framing is removed: the SequencerPanel already draws the device bezel, so WaveformDisplay no longer draws its own redundant bezel.
- Release file logging is opt-in outside debug builds through `ZIKADARATOR_DEBUG_LOG=1`, and footer/workspace interaction helpers no longer write routine UI logs.
- Ableton log scanning now treats a missing ZIKADARATOR UI log as expected for quiet release builds, with `-RequireZikadaLog` available for strict diagnostic-log passes.
- Windows tester packages now include and self-verify `SHA256SUMS.txt` so the standalone, VST3 binary, and module metadata can be checked after transfer.
- Windows ZIP install docs now match the package root layout, and `install.bat` reports Program Files copy failures instead of printing a false success.
- Windows tester packages now include `BUILD_INFO.txt` with git/build metadata for traceable handoff builds.
- Windows tester packages now include `verify-checksums.bat` / `verify-checksums.ps1` so extracted files can be validated without reading the package script.
- Validation scripts now auto-discover Steinberg VST3 validator on common Windows/macOS paths.
- Windows CI now syntax-checks the macOS validation script via Bash (`bash -n`).
- Processor regression tests now cover host-state roundtrip (`exportFullState` / `applyFullState`) and host-layout changes (mono → stereo → mono).
- Processor regression tests now also cover invalid ValueTree state and malformed binary state restore attempts, ensuring DAW restore failures do not wipe current parameters or sequencer data.
- Windows and macOS CI workflows now include optional code signing and notarization steps that run only when signing secrets are present, producing unsigned tester builds otherwise.
- Windows CI now signs the actual VST3 and standalone binaries before invoking the same release packager used locally. The generated ZIP is extracted and checksum-verified before upload, and the installer consumes that canonical package tree.
- Presets now separate search/category controls from source filters, and Settings uses a stable two-column product-control grid instead of overlapping fixed stacks.
- The standalone JUCE audio/MIDI selector is constrained to a scrolling viewport, so its self-sized child controls cannot paint across product settings at compact editor heights.
- Step chain-badge painting and hit testing now use the same bounds, and the duplicate painted `STEP RES` footer label is removed.
- JUCE geometry regressions cover visible Presets/Settings controls at 1184x718 and 900x600; standalone desktop checks also cover the default and compact window layouts.
- SliceEngine now follows the resolved host/free-clock BPM every block; separate free-clock and simulated-host-playhead regressions prove 60 BPM slices render materially longer than 120 BPM slices.
- APVTS step-active parameters are now the canonical realtime gate through cached atomic pointers, while sequencer metadata, export state, StepCell visuals, chains, and header state are synchronized on the message thread.
- Factory presets now use JUCE APVTS `PARAM` child trees, keep their step gates consistent with SequencerState, and load through a versioned migration path that preserves legacy patterns.
- Full-state export serializes an immutable sequencer snapshot and overlays realtime APVTS gates without writing to the live UI-owned state.
- Wrong-root host state is rejected before a nested SequencerState can clear the current pattern.
- Automated step Off-to-On transitions inside the current step have direct behavioral coverage for SLICE retriggering, LOOP history playback, and FILTER reconfiguration.
- EnvFollowerEngine now applies its bipolar flag correctly and has direct engine regression coverage.
- Lane mix knobs now use a bidirectional JUCE parameter attachment, follow synchronous/asynchronous automation and preset restore, and keep host gestures balanced across drag, click, double-click reset, and destruction during an active drag.
- StepGrid drag-to-paint now interpolates across skipped mouse events, copies active presets, erases from inactive sources, synchronizes APVTS gates with complete host gestures, clears overwritten chains, and creates one undo boundary per gesture. Host-enabled blank sources normalize to preset 1, Shift+Drag is reserved for tie drawing, and click-only selection remains non-destructive.
- Hidden waveform consumers now discard queued tap samples and clear retained display history in both page/visibility directions; 16,384-sample fixed UI buffers keep draining bounded at high sample rates. Lock-free operation guards and tap generations make reprepare safe against concurrent audio/UI access and invalidate old-rate display history. The CRT monitor now owns a region-limited 30 Hz animation timer.
- Custom-drawn header and preset actions now expose stable accessible names, help, deterministic global focus order, and Space/Enter parity. Knobs expose native slider roles, ranged values, visible focus, keyboard operation, and complete host gestures. StepGrid is one named keyboard-navigable group without 96 duplicate child focus stops; child keys cannot trigger grid edits, and Footer controls have purpose-specific names.
- `processBlock()` now uses only prepare-time scratch capacity: oversized host buffers are split into bounded chunks without vector growth, while cached atomic pointers replace per-block lane-ID construction and APVTS lookup. A 4097-after-64 regression verifies unchanged capacity, exact chunk count, and complete dry output.
- Host transport now uses bounded signed absolute scheduler steps plus a discontinuity generation derived from sample time and PPQ. Same-step loops, 16-step seeks, stop/restart, negative preroll, host/free and resolution changes, bypassed seeks, state restore, and preset swaps retrigger Slice/Loop/Filter correctly; continuous blocks and chain continuations do not. Stopped host callbacks freeze sequenced engines while retaining output gain/bypass, and scheduler-safe chunking preserves offline blocks beyond the fixed 64-segment stack batch.
- Invalid, non-finite, or sub-10 Hz host sample rates are normalized before every engine and waveform tap is prepared; pitch-path regressions cover the former zero-sized-ring inputs.
- Concurrent sequencer UI/restore writers are serialized across live-grid mutation and fixed-buffer snapshot publication. Live getters return synchronized values, while the audio callback still consumes a lock-free immutable snapshot.
- LOOP and SLICE trigger capture is incremental and allocation-free: each scheduler chunk copies at most 2,048 frames (or the chunk size for larger offline chunks), while uncopied playback reads immediately from pinned, overwrite-protected ring history. SLICE retention now covers all 16 offsets at 20 BPM.
- FILTER modulation now combines cutoff/resonance into one deduplicated coefficient update, keeps filter-type work out of the per-sample path, and skips unused second-stage updates for 12 dB modes. Comb delay parameters are cached, and generation-tagged storage makes Comb enter/leave resets constant-time without exposing stale audio.
- LOOP now prepares 12 seconds of history, covering four user-controlled beats at the supported 20 BPM floor. The maximum tail explicitly documents its 12-second cap. Runtime FX labels and icons now match implemented Comb/Notch, Delay, Filter, Pitch Color, Bitcrush, Ring Mod, and Space paths; alternate FILTER slots have distinct bounded tunings.
- StepGrid advanced keyboard parity is complete: Page Up/Down cycles the selected effect preset, Shift+Left/Right resizes ties from either their root or a consumed step, active-step collisions are protected, and the full command set is exposed through accessibility help.
- The macOS CI release order now signs and verifies VST3/AU/Standalone bundles before staging, verifies the copied payload, signs and optionally notarizes the final installer, then creates verified ZIP/PKG checksums and signed/notarized status metadata before upload.
- The unsigned macOS tester path is proven on macOS 14 by Actions run `29888007903` at commit `e6d454f`: CTest, VST3/AU pluginval level 5, exact `aumf/Zkfx/Zika` system `auval`, staging, ZIP/PKG creation, metadata, checksum verification, and all artifact uploads passed.
- The product-spec factory bank is complete at exactly 50 embedded presets: the original eight states remain unchanged, 42 declarative patterns cover nine useful browser categories, and executable contracts enforce unique names/states, complete APVTS/sequencer schemas, valid lane indices, host binary roundtrips, refresh-stable navigation IDs, validated atomic user saves, legacy collision recovery, multi-token search, and browser selection preservation.

## Open V1 Gates

1. Run a fresh Windows Release build through CTest, pluginval level 5, the CI-style package path, installer smoke, and Ableton Live 12 manual audio acceptance.
2. Configure Apple signing/notarization secrets and retain a credential-backed run proving bundle signatures, final PKG signature, notarization, stapling, and ticket validation. The unsigned tester path is complete.
3. Move product/plugin/package metadata from `0.1.0` to `1.0.0` only after the gates above pass, then publish signed artifacts where credentials are available.

## Release Hardening

1. Make Steinberg VST3 validator availability mandatory for public-release jobs while retaining optional discovery for local tester builds.
2. Require a credential-backed macOS package check proving staged bundle signatures, final PKG signature, notarization ticket, manifest, and SHA-256 files match the intended public-release path.
3. Add a tag-driven GitHub Release workflow with one authoritative version source and explicit signed/unsigned artifact labels.
4. Keep unsigned outputs labeled as tester builds until signing credentials and public-release gates are present.
5. Update `actions/checkout` and `actions/upload-artifact` when Node 24-native releases are available; the macOS runner currently warns while forcing their Node 20 actions onto Node 24.
