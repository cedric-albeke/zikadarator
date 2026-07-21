import { readFileSync } from "node:fs";
import { join } from "node:path";

const root = new URL("..", import.meta.url).pathname.replace(/^\/([A-Za-z]:\/)/, "$1");

function read(path) {
  return readFileSync(join(root, path), "utf8");
}

function fail(message) {
  console.error(`FAIL: ${message}`);
  process.exitCode = 1;
}

function assertContains(source, needle, message) {
  if (!source.includes(needle)) fail(message);
}

function assertBefore(source, firstNeedle, secondNeedle, message) {
  const firstIndex = source.indexOf(firstNeedle);
  const secondIndex = source.indexOf(secondNeedle);
  if (firstIndex < 0 || secondIndex < 0 || firstIndex >= secondIndex) fail(message);
}

function extractFunction(source, signature) {
  const start = source.indexOf(signature);
  if (start < 0) {
    fail(`Missing function: ${signature}`);
    return "";
  }

  const braceStart = source.indexOf("{", start);
  let depth = 0;
  for (let i = braceStart; i < source.length; i += 1) {
    if (source[i] === "{") depth += 1;
    if (source[i] === "}") depth -= 1;
    if (depth === 0) return source.slice(start, i + 1);
  }

  fail(`Could not parse function body: ${signature}`);
  return "";
}

const processor = read("src/PluginProcessor.cpp");
const processorHeader = read("src/PluginProcessor.h");
const editor = read("src/PluginEditor.cpp");
const sidebar = read("src/ui/panels/SidebarPanel.cpp");
const footer = read("src/ui/panels/FooterPanel.cpp");
const workspace = read("src/ui/panels/WorkspacePanel.cpp");
const stepGridHeader = read("src/ui/components/StepGrid.h");
const stepGrid = read("src/ui/components/StepGrid.cpp");
const stepCellHeader = read("src/ui/components/StepCell.h");
const stepCell = read("src/ui/components/StepCell.cpp");
const knobHeader = read("src/ui/components/Knob.h");
const knob = read("src/ui/components/Knob.cpp");
const presetManager = read("src/state/PresetManager.cpp");
const parameterIDs = read("src/state/ParameterIDs.h");
const sequencerState = read("src/state/SequencerState.h");
const processorTests = read("tests/processor/ProcessorTestMain.cpp");
const sequencerEngine = read("src/engine/SequencerEngine.cpp");
const sliceEngine = read("src/engine/SliceEngine.cpp");
const envelopeShape = read("src/engine/EnvelopeShape.cpp");
const envFollower = read("src/engine/EnvFollowerEngine.cpp");
const loopEngine = read("src/engine/LoopEngine.cpp");
const loopEngineHeader = read("src/engine/LoopEngine.h");
const waveformDisplayHeader = read("src/ui/components/WaveformDisplay.h");
const waveformDisplay = read("src/ui/components/WaveformDisplay.cpp");
const headerPanelHeader = read("src/ui/panels/HeaderPanel.h");
const headerPanel = read("src/ui/panels/HeaderPanel.cpp");
const windowsPackageScript = read("scripts/package-windows-release.ps1");
const windowsWorkflow = read(".github/workflows/build-windows-vst3.yml");
const windowsInstallerScript = read("packaging/windows/ZIKADARATOR.iss");
const abletonLogScanScript = read("scripts/ableton-log-scan.ps1");
const processBlock = extractFunction(
  processor,
  "void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)",
);
const processSegment = extractFunction(
  processor,
  "void PluginProcessor::processSegment(float* leftChannel,",
);
const loopPresetMapping = extractFunction(
  processor,
  "void configureLoopEngineForPreset(LoopEngine& loopEngine, int presetIndex, const UserSlotData& slotData,",
);
const exportFullState = extractFunction(
  processor,
  "juce::ValueTree PluginProcessor::exportFullState()",
);
const triggerSlice = extractFunction(
  sliceEngine,
  "void SliceEngine::triggerSlice(int sliceIndex)",
);
const setLoopParameters = extractFunction(
  loopEngine,
  "void LoopEngine::setLoopParameters(float loopLengthSeconds,",
);
const ensureLoopBufferSize = extractFunction(
  loopEngine,
  "void LoopEngine::ensureLoopBufferSize()",
);
const refreshLoopSnapshot = extractFunction(
  loopEngine,
  "void LoopEngine::refreshLoopSnapshot()",
);
const stepGridTimer = extractFunction(
  stepGrid,
  "void StepGrid::timerCallback()",
);
const stepGridConstructor = extractFunction(
  stepGrid,
  "StepGrid::StepGrid(juce::AudioProcessorValueTreeState& state, SequencerState& seqState)",
);

[
  ["ParameterIDs::dryWet", "processBlock must read/apply global Dry/Wet"],
  ["ParameterIDs::outputGain", "processBlock must read/apply Output Gain"],
  ["ParameterIDs::mixMode", "processBlock must read/apply Mix Mode"],
  ["ParameterIDs::bypass", "processBlock must read/apply Bypass"],
  ["ParameterIDs::clockSource", "processBlock must read Clock Source"],
  ["ParameterIDs::tempo", "processBlock must read Free Tempo"],
  ["ParameterIDs::stepResolution", "processBlock must read Step Resolution"],
  ["sequencerEngine.setStepResolution", "processBlock must push step resolution into SequencerEngine"],
  ["stepScheduler.makeHostSegments", "processBlock must split work with StepScheduler"],
  ["processSegment(", "processBlock must dispatch segment processing"],
].forEach(([needle, message]) => assertContains(processBlock, needle, message));

assertContains(processor, "blendGlobalMixSample", "processor must use global mix-mode blending");

if (processBlock.includes("std::vector<float>")) {
  fail("processBlock must not allocate std::vector buffers on the audio thread");
}

if (processBlock.includes("getActiveEditor(")) {
  fail("processBlock must not touch the active editor from the audio thread");
}

if (processBlock.includes("Logger::writeToLog")) {
  fail("processBlock must not write logs from the audio thread");
}
assertContains(editor, "ZIKADARATOR_DEBUG_LOG", "editor file logging must be opt-in for release builds");
assertContains(processor, "ZIKADARATOR_DEBUG_LOG", "processor file logging must be opt-in for release builds");
assertContains(editor, "getEnvironmentVariable(\"ZIKADARATOR_DEBUG_LOG\"", "editor debug logging must read an explicit opt-in environment variable");
assertContains(processor, "getEnvironmentVariable(\"ZIKADARATOR_DEBUG_LOG\"", "processor debug logging must read an explicit opt-in environment variable");
assertContains(editor, "if (! isDebugFileLoggingEnabled())", "editor debug logging must skip file writes unless enabled");
assertContains(processor, "if (! isDebugFileLoggingEnabled())", "processor debug logging must skip file writes unless enabled");
assertContains(abletonLogScanScript, "ZIKADARATOR_DEBUG_LOG", "Ableton log scanner must understand opt-in ZIKADARATOR diagnostic logging");
assertContains(abletonLogScanScript, "RequireZikadaLog", "Ableton log scanner must support strict diagnostic-log checks when requested");
assertContains(abletonLogScanScript, "expected for normal release builds", "Ableton log scanner must treat a missing UI log as expected for quiet release builds");
if (footer.includes("Logger::writeToLog")) {
  fail("FooterPanel interaction helpers must not write release debug logs");
}
if (workspace.includes("Logger::writeToLog")) {
  fail("WorkspacePanel interaction helpers must not write release debug logs");
}

assertContains(processorHeader, "dryLeftBuffer", "processor must own reusable dry scratch buffers");
assertContains(processorHeader, "wetLeftBuffer", "processor must own reusable wet scratch buffers");
assertContains(processorHeader, "laneInputLeftBuffer", "processor must own reusable per-lane input scratch buffers");
assertContains(processorHeader, "monoRightBuffer", "processor must own reusable mono right-side scratch");
assertContains(processorHeader, "ensureScratchBuffers", "processor must expose scratch-buffer sizing helper");
assertContains(processBlock, "monoRightBuffer.data()", "mono processing must avoid aliasing left/right pointers");
assertContains(processSegment, "0.5f * (outLeft + outRight)", "mono processing must fold rendered stereo output to mono");
assertContains(processBlock, "sequencerState.getSnapshot()", "processBlock must capture a sequencer snapshot for audio rendering");
assertContains(processBlock, "sliceEngine.setTempo(bpm)", "processBlock must synchronize SliceEngine with resolved host/free tempo");
assertContains(processorHeader, "stepActiveParameters", "processor must cache realtime-safe step-active parameter pointers");
assertContains(processBlock, "activeParameter->load() > 0.5f", "audio snapshots must use host-automatable step-active gates");
assertContains(processor, "synchronizeSequencerActiveStateFromParameters", "processor must synchronize automated step gates back into sequencer metadata");
assertContains(exportFullState, "sequencerState.getSnapshot()", "state export must serialize an immutable sequencer snapshot");
if (exportFullState.includes("synchronizeSequencerActiveStateFromParameters")) {
  fail("state export must not mutate the live sequencer");
}
assertContains(sequencerState, "return getSnapshot().toValueTree()", "sequencer serialization must read from a published snapshot");
assertContains(parameterIDs, "setParameterStateValue", "preset state helpers must write JUCE APVTS PARAM child trees");
assertContains(presetManager, "appendSequencerState", "factory presets must mirror sequencer gates into APVTS state");
assertContains(presetManager, "setParameterStateValue(state, getLaneMuteID(lane), 0.0f)", "factory presets must reset lane mute state");
assertContains(presetManager, "setParameterStateValue(state, getLaneSoloID(lane), 0.0f)", "factory presets must reset lane solo state");
assertContains(processor, "schemaVersion < currentStateSchemaVersion", "legacy state restore must migrate sequencer gates into APVTS");
assertContains(editor, "step != lastPlayingStep || sequencerActiveStateChanged", "CRT active-lane state must refresh during same-step automation");
assertContains(processor, "!stateTree.hasType(state.getValueTreeState().state.getType())", "full-state restore must reject a wrong root before loading sequencer data");
assertContains(envFollower, "isBipolar    = bipolar;", "envelope follower must apply its bipolar parameter instead of self-assigning");
assertContains(processSegment, "SequencerState::Snapshot", "processSegment must render from an immutable sequencer snapshot");
assertContains(processSegment, "sequencerSnapshot.getStepData", "processSegment must read step data from the captured snapshot");
assertContains(processSegment, "sequencerSnapshot.getUserSlot", "processSegment must read user slots from the captured snapshot");
assertContains(windowsPackageScript, "moduleinfo.json", "Windows package script must validate VST3 moduleinfo metadata");
assertContains(windowsPackageScript, "packaging/windows/moduleinfo.json", "Windows package script must use tracked moduleinfo fallback when JUCE helper is blocked");
assertContains(windowsPackageScript, "Length -eq 0", "Windows package script must repair zero-byte moduleinfo outputs");
assertContains(windowsPackageScript, "Compress-Archive", "Windows package script must create a tester-facing ZIP package");
assertContains(windowsPackageScript, "SHA256SUMS.txt", "Windows package script must include tester-facing SHA-256 checksums");
assertContains(windowsPackageScript, "Get-FileHash", "Windows package script must compute package checksums with SHA-256");
assertContains(windowsPackageScript, "Test-PackageChecksums", "Windows package script must verify checksums before compressing the ZIP");
assertContains(windowsPackageScript, "Checksum mismatch", "Windows package checksum verification must fail on stale or corrupt artifacts");
assertContains(windowsPackageScript, "ERROR: VST3 install failed", "Windows package install.bat must fail loudly when VST3 copy is blocked");
assertContains(windowsPackageScript, "Run this script as Administrator", "Windows package install.bat must tell testers how to fix Program Files write failures");
assertContains(windowsPackageScript, "BUILD_INFO.txt", "Windows package script must include git/build metadata for tester traceability");
assertContains(windowsPackageScript, "rev-parse", "Windows package build metadata must include the git commit");
assertContains(windowsPackageScript, "verify-checksums.ps1", "Windows package must include an extracted-package checksum verifier");
assertContains(windowsPackageScript, "All ZIKADARATOR package checksums verified.", "Windows package checksum verifier must report success clearly");
assertContains(windowsPackageScript, "installer/ZIKADARATOR-Setup.iss", "Windows package checksums must cover the included installer source");
assertContains(windowsPackageScript, "Signing status:", "Windows package build metadata must identify signed versus unsigned tester artifacts");
assertContains(windowsWorkflow, ".\\scripts\\package-windows-release.ps1", "Windows CI must use the same tested package builder as local releases");
assertContains(windowsWorkflow, "$vst3Binary", "Windows CI signing must target the VST3 binary inside the bundle");
assertContains(windowsWorkflow, "$standaloneExe", "Windows CI signing must cover the standalone binary before packaging");
assertContains(windowsWorkflow, "WINDOWS_SIGNING_ENABLED: ${{ secrets.WINDOWS_CERTIFICATE != '' }}", "Windows CI must derive a non-secret job-level signing condition");
assertContains(windowsWorkflow, "if: env.WINDOWS_SIGNING_ENABLED == 'true'", "Windows signing steps must use the non-secret job-level condition");
if (windowsWorkflow.includes("if: env.WINDOWS_CERTIFICATE != ''")) {
  fail("Windows CI must not condition signing directly on a step-local secret environment variable");
}
assertContains(windowsWorkflow, "Verify packaged Windows ZIP", "Windows CI must verify the final extracted tester ZIP");
assertBefore(
  windowsWorkflow,
  "name: Sign Windows plugin binaries",
  "name: Build Windows tester package",
  "Windows plugin binaries must be signed before the ZIP and installer payload are staged",
);
assertBefore(
  windowsWorkflow,
  "name: Create Windows build manifest",
  "name: Upload Windows VST3 artifact",
  "Windows build metadata must exist before release artifacts are uploaded",
);
assertContains(windowsInstallerScript, 'Source: "{#MySourceDir}\\ZIKADARATOR.vst3\\*"', "Windows installer must consume the canonical root-level VST3 package layout");
assertContains(windowsInstallerScript, 'Source: "{#MySourceDir}\\ZIKADARATOR.exe"', "Windows installer must consume the canonical root-level standalone package layout");
if (processSegment.includes("sequencerState.getStepData") || processSegment.includes("sequencerState.getUserSlot")) {
  fail("processSegment must not read mutable SequencerState directly on the audio thread");
}
if (processSegment.includes("left[i] *= mix") || processSegment.includes("right[i] *= mix")) {
  fail("lane mix must dry/wet blend the lane result, not multiply the whole shared wet chain");
}
assertContains(sliceEngine, "playbackBufferLeft.assign", "SliceEngine must preallocate playback buffers during prepare");
if (triggerSlice.includes(".resize(") || triggerSlice.includes(".assign(")) {
  fail("SliceEngine triggerSlice must not allocate on the audio thread");
}
assertContains(read("src/engine/SliceEngine.h"), "enum class PlaybackMode", "SliceEngine must expose real playback modes for slice presets");
assertContains(read("src/engine/SliceEngine.h"), "edgeFadeSamples", "SliceEngine must keep a realtime-safe edge fade for click reduction");
assertContains(read("src/engine/SliceEngine.h"), "getEdgeFadeGain", "SliceEngine must centralize slice de-click gain calculation");
assertContains(sliceEngine, "getEdgeFadeGain(position)", "SliceEngine process must apply de-click gain while rendering frozen slices");
assertContains(sliceEngine, "playbackLength > 8 ? juce::jlimit(1, 64, playbackLength / 16) : 0", "SliceEngine must derive bounded edge fades from the frozen slice length");
assertContains(processor, "getSliceConfigForPreset", "processor must map slice presets to playback configs, not only slice indexes");
assertContains(processor, "SliceEngine::PlaybackMode::Reverse", "slice preset mapping must back the Reverse preset with DSP");
assertContains(processor, "SliceEngine::PlaybackMode::Repeat", "slice preset mapping must back the Repeat preset with DSP");
assertContains(processor, "SliceEngine::PlaybackMode::Stutter", "slice preset mapping must back the Stutter preset with DSP");
if (processor.includes("getSliceIndexForPreset")) {
  fail("slice preset mapping must not collapse back to index-only configuration");
}
assertContains(processor, "computeEnvelopeShape", "processor must use the shared envelope shape mapper");
assertContains(envelopeShape, "case 13:", "EnvelopeShape must back the advertised Swell preset");
assertContains(envelopeShape, "case 15:", "EnvelopeShape must back the advertised Tremolo preset");
assertContains(envelopeShape, "case 16:", "EnvelopeShape must back the advertised Wobble preset");
assertContains(envelopeShape, "case 20:", "EnvelopeShape must back the advertised Hold preset");
assertContains(loopEngine, "loopBufferL.assign", "LoopEngine must preallocate snapshot buffers during prepare");
if ([setLoopParameters, ensureLoopBufferSize, refreshLoopSnapshot].some((body) => body.includes(".resize(") || body.includes(".assign("))) {
  fail("LoopEngine render-called loop setup/snapshot paths must not allocate on the audio thread");
}
assertContains(processSegment, "blendLaneOutput", "processSegment must blend each lane output against that lane input");
assertContains(processSegment, "ModulationTarget::FilterCutoff", "filter lane modulation must apply cutoff target");
assertContains(processSegment, "ModulationTarget::FilterResonance", "filter lane modulation must apply resonance target");
assertContains(processSegment, "ModulationTarget::Volume", "filter lane modulation must apply volume target");
assertContains(processSegment, "ModulationTarget::Pan", "filter lane modulation must apply pan target");
assertContains(processor, "applyGainPanSample", "processor must support per-sample gain/pan modulation for filter lane targets");
assertContains(processorTests, "invalid host state is ignored without corrupting current state", "processor tests must guard invalid host state restore safety");
assertContains(processorTests, "malformed binary host state is ignored without corrupting current state", "processor tests must guard malformed binary host state restore safety");
assertContains(footer, "getSupportedModTargetsForLane", "footer MOD target picker must use lane-supported targets");
assertContains(footer, "hasSupportedModTargetsForLane", "footer MOD mode must be disabled for lanes without backed modulation");
assertContains(footer, "sanitizeModTargetForLane", "footer must sanitize saved unsupported MOD targets before showing or writing them");
assertContains(footer, "std::array<ModulationTarget, 5>", "footer supported MOD target list must be explicit and bounded");
assertContains(footer, "kFilterLaneIndex = 4", "footer must identify the filter lane as the backed modulation lane");
assertContains(footer, "modModeButton.setEnabled(hasSelection && hasSupportedModTargetsForLane(selectedLane))", "footer MOD button must only enable when the selected lane has backed targets");
if (footer.includes("% (static_cast<int>(ModulationTarget::NumTargets) + 1)")) {
  fail("footer MOD target cycling must use lane-supported targets instead of every enum value");
}
assertContains(editor, "setFixedAspectRatio", "plugin editor must constrain resizing to a fixed aspect ratio");
assertContains(editor, "layoutEditorCanvas", "plugin editor must lay out panels on a logical canvas");
assertContains(editor, "editorCanvas.setTransform(juce::AffineTransform::scale", "plugin editor must scale the logical canvas for compact sizes");
assertContains(read("src/PluginEditor.h"), "editorCanvas", "plugin editor must own a logical canvas component");
assertContains(editor, "juce::PopupMenu presetMenu", "header preset control must open an anchored quick menu");
assertContains(editor, "FAVORITES", "header preset quick menu must expose favorite presets");
assertContains(editor, "RECENTS", "header preset quick menu must expose recent presets");
assertContains(editor, "ALL PRESETS", "header preset quick menu must expose the full preset list");
assertContains(editor, "Open browser...", "header preset quick menu must offer full browser access");
if (editor.includes("redirecting to preset browser")) {
  fail("header preset menu must not redirect straight to the full preset browser");
}
assertContains(headerPanelHeader, "setFxDisplayState", "header must expose the CRT FX display state API");
assertContains(headerPanelHeader, "setFxDisplayPlayhead", "header must expose the CRT playhead pulse API");
assertContains(headerPanelHeader, "fxDisplayBounds", "header must reserve stable bounds for the CRT FX monitor");
assertContains(headerPanel, "drawFxCrtDisplay", "header must draw a dedicated CRT-style FX monitor");
assertContains(headerPanel, "FX MON", "CRT FX monitor must be labeled as a live FX monitor");
assertContains(headerPanel, "scanLineY", "CRT FX monitor must include scanline motion");
assertContains(headerPanel, "phosphorPath", "CRT FX monitor must include a triggered phosphor waveform");
assertContains(editor, "headerPanel.setFxDisplayState", "editor must update the CRT display from selected step state");
assertContains(editor, "headerPanel.setFxDisplayPlayhead", "editor must pulse the CRT display from the playhead");
assertContains(sidebar, "selectedPresetIndex", "sidebar must track selected preset details separately from hover");
assertContains(sidebar, "updateInfoForSelection", "sidebar must restore selected preset details when hover leaves");
assertContains(sidebar, "kFactoryGridCols = 4", "sidebar preset browser must use a scan-friendly four-column factory grid");
assertContains(sidebar, "kFactoryPresetCount = 16", "sidebar preset browser must distinguish factory presets from user slots");
assertContains(sidebar, "kUserSlotCols = 4", "sidebar preset browser must place U1-U4 in a separated four-slot row");
assertContains(sidebar, "drawPresetLabel", "sidebar preset browser must draw visible short labels, not icon-only cells");
assertContains(sidebar, "USER SLOTS", "sidebar preset browser must visually separate user slots from factory presets");
assertContains(read("src/ui/panels/SidebarPanel.h"), "keyPressed", "sidebar preset browser must support keyboard navigation");
assertContains(sidebar, "setWantsKeyboardFocus(true)", "sidebar preset browser must be keyboard focusable");
assertContains(sidebar, "focusedPresetButtonIndex", "sidebar preset browser must track keyboard focus separately from hover");
assertContains(sidebar, "moveFocusedPresetBy", "sidebar preset browser must support arrow-key focus movement");
assertContains(sidebar, "activateFocusedPreset", "sidebar preset browser must activate focused presets from keyboard");
assertContains(sidebar, "juce::KeyPress::spaceKey", "sidebar preset browser must handle space/return activation");
assertContains(sidebar, "kInfoH = 84", "sidebar detail panel must reserve enough room for title and body copy");
assertContains(read("src/ui/panels/SidebarPanel.h"), "infoTitleLabel", "sidebar detail panel must split title from body copy");
assertContains(read("src/ui/panels/SidebarPanel.h"), "infoDetailLabel", "sidebar detail panel must split detail body from title");
assertContains(sidebar, "setInfoText", "sidebar detail panel must update structured title/detail copy through one helper");
if (sidebar.includes("Hover a preset to see details...")) {
  fail("sidebar detail panel must not depend on hover-only copy");
}
if (sidebar.includes("\"Selected: \" +")) {
  fail("sidebar detail panel must not concatenate selected preset title and body into one cramped line");
}
assertContains(stepGrid, "mouseWheelMove", "step grid must support mouse-wheel preset cycling");
assertContains(stepGrid, "cyclePresetAt", "step grid must cycle presets through a dedicated helper");
assertContains(stepGrid, "onStepPresetEditStarting", "step grid preset cycling must expose a pre-edit undo hook");
assertContains(stepGrid, "onStepPresetChanged", "step grid preset cycling must notify the editor");
assertContains(stepGrid, "setWantsKeyboardFocus(true)", "step grid must be keyboard focusable");
assertContains(stepGrid, "grabKeyboardFocus()", "step grid must take focus when clicked");
assertContains(stepGridHeader, "keyPressed", "step grid must handle keyboard navigation");
assertContains(stepGrid, "moveSelectionBy", "step grid must expose arrow-key selection movement");
assertContains(stepGrid, "toggleSelectedStep", "step grid must expose keyboard activation for the selected step");
assertContains(stepGrid, "drawKeyboardFocusRing", "step grid must draw a visible keyboard focus ring");
assertContains(stepGrid, "drawPlayheadRail", "step grid must draw a column-wide playhead rail");
assertContains(stepGridHeader, "staticGridLayer", "step grid must cache its static background chrome");
assertContains(stepGrid, "renderStaticGridLayer", "step grid must render static chrome through a dedicated cache painter");
assertContains(stepGrid, "invalidateStaticGridLayer", "step grid must expose a cheap cache invalidation path");
assertContains(stepGrid, "getStepColumnBounds", "step grid must compute bounded repaint regions for playhead movement");
assertContains(stepGrid, "repaint(getStepColumnBounds(previousPlayingStep)", "step grid must repaint the old playhead column only");
assertContains(stepGrid, "repaint(getStepColumnBounds(lastPlayingStep)", "step grid must repaint the new playhead column only");
assertContains(stepGrid, "namespace StepGridMetrics", "step grid layout metrics must be centralized instead of repeated across paint/resized/hit-test");
assertContains(stepGrid, "kBeatGroupSize = 4", "step grid must explicitly model four-step beat groups");
assertContains(stepGrid, "drawBeatGroupBackgrounds", "step grid must draw subtle beat-group backplates so the sequencer reads musically");
assertContains(stepGrid, "beatGroupBounds", "step grid beat-group drawing must use stable group bounds instead of ad hoc separator lines only");
assertContains(stepGrid, "juce::KeyPress::leftKey", "step grid must handle left/right arrow keys");
assertContains(stepGrid, "juce::KeyPress::spaceKey", "step grid must handle space/return activation");
assertContains(stepGrid, "e.mods.isShiftDown()", "off-cell wheel preset cycling must require an explicit Shift gesture");
if (stepGrid.includes("lane = hoverLane >= 0 ? hoverLane : selectedLane")) {
  fail("step grid wheel cycling must not silently fall back from off-cell wheel movement to hover/selection");
}

const laf = read("src/ui/ZikadaLookAndFeel.cpp");
assertContains(laf, "drawPremiumPanel", "ZikadaLookAndFeel must draw premium panels with depth");
assertContains(laf, "drawDeviceDisplay", "ZikadaLookAndFeel must draw device displays with bezel");
assertContains(laf, "Colours::black.withAlpha(0.30f)", "premium panels must have a subtle drop shadow");
assertContains(laf, "Colours::panelRaised.withAlpha(0.40f)", "premium panels must have a depth gradient from panelRaised");
assertContains(laf, "Colours::white.withAlpha(0.08f)", "premium panels must have an outer rim stroke");
assertContains(laf, "Colours::black.withAlpha(0.40f)", "premium panels must have an inner bezel shadow");
assertContains(laf, "Colours::white.withAlpha(0.04f)", "premium panels must have an inner bezel highlight");
assertContains(laf, "Colours::neonGreen.withAlpha(0.65f)", "premium panel top accent must be bright neon green");
assertContains(laf, "Colours::neonGreen.withAlpha(0.25f)", "premium panel top accent must have a glow");
assertContains(laf, "juce::Colours::black.withAlpha(0.50f)", "device displays must have an inset shadow");
assertContains(laf, "juce::Colours::black.withAlpha(0.30f)", "device displays must have a bottom shadow");
assertContains(laf, "Colours::neonGreen.withAlpha(0.25f)", "device displays must have a subtle green accent border");
assertContains(editor, "onStepPresetEditStarting", "editor must snapshot before wheel-driven preset changes");
assertContains(editor, "onStepPresetChanged", "editor must listen for wheel-driven preset changes");
assertContains(knobHeader, "KnobDisplayMode", "knobs must expose unit-aware display modes");
assertContains(knobHeader, "KnobScaleMode", "knobs must expose linear/log scale modes");
assertContains(knob, "outerRing", "knobs must have an outer ring stroke");
assertContains(knob, "backgroundArc", "knobs must have a background arc track");
assertContains(knob, "valueArc", "knobs must have a colored value arc indicator");
assertContains(knob, "capGrad", "knobs must have a 3D center cap with radial gradient");
assertContains(knob, "isMouseOverOrDragging", "knobs must react to hover state");
assertContains(knob, "glowAlpha", "knobs must have a dynamic glow that responds to hover");
assertContains(knob, "0.18f", "knobs must have a base glow alpha of 0.18");
assertContains(knob, "accentColour.withAlpha(0.90f", "knobs must have a precise core value arc");
assertContains(knob, "juce::Colours::black.withAlpha(0.40f)", "knobs must have a center cap shadow for depth");
assertContains(knob, "dotRadius", "knobs must have a center dot that responds to hover");
assertContains(knob, "getSpaceMonoFont(10.0f)", "knobs must use compact 10px value labels");
assertContains(knob, "getSpaceMonoFont(11.0f)", "knobs must use compact 11px labels");

assertContains(stepCell, "playing", "step cell must support playing state indicator");
assertContains(stepCell, "kPlayingGlowAlpha = 0.26f", "playing step glow must stay visible but restrained");
assertContains(stepCell, "Colours::neonGreen.withAlpha(0.65f)", "playing step must have a neon green border");
assertContains(stepCell, "getSpaceMonoFont(9.0f)", "active step numbers must be compact 9px");
assertContains(stepCell, "getSpaceMonoFont(11.0f)", "inactive step numbers must be 11px");
assertContains(stepCell, "juce::Colours::black.withAlpha(0.25f)", "active step cells must have a bottom shadow");
assertContains(stepCell, "kActiveGlowAlpha = 0.12f", "active step cells must use restrained glow now that beat groups provide structure");
assertContains(stepCell, "kActiveHoverGlowAlpha = 0.24f", "hovered active step glow must be clearer than idle without flooding the grid");
assertContains(stepCell, "kInactiveHoverBorderAlpha = 0.38f", "inactive hover borders must stay below selected/playing emphasis");
assertContains(stepCell, "cellColour.withAlpha(0.68f)", "active step cells must keep a strong lane color border without overemphasis");
assertContains(stepCell, "Colours::white.withAlpha(0.28f)", "hovered active cells must use a restrained white highlight border");
assertContains(read("src/ui/panels/SequencerPanel.cpp"), "kGridSurfaceAlpha = 0.30f", "sequencer passive grid surface must be quieter than active step/panel states");
assertContains(read("src/ui/panels/SequencerPanel.cpp"), "kGridBorderAlpha = 0.045f", "sequencer passive grid border must be quieter than active step/panel states");
assertContains(footer, "kDetailGroupHeaderH = 12", "footer detail dock must reserve stable space for lane-aware knob group headers");
assertContains(footer, "drawDetailGroupHeaders", "footer detail dock must draw knob groups instead of a flat seven-control strip");
assertContains(footer, "selectedLane == 1 ? \"LOOP\" : \"TONE\"", "footer group labels must adapt to loop lane versus tone-style lanes");
assertContains(footer, "selectedLane == 1 ? \"TEXTURE\" : \"SPACE\"", "footer group labels must adapt the middle knob group to lane context");
assertContains(footer, "drawGroupHeader", "footer group headers must use a shared painter for consistent hierarchy");
assertContains(footer, "const bool showStepControls = hasSelection && !modModeActive", "footer must hide step knobs until a step is selected");
assertContains(footer, "const bool showModControls = hasSelection && modModeActive", "footer must hide modulation controls until a step is selected");
assertContains(footer, "modModeButton.setEnabled(hasSelection && hasSupportedModTargetsForLane(selectedLane))", "footer mod mode must be disabled until a backed modulation lane is selected");
assertContains(footer, "if (!hasSelection || !hasSupportedModTargetsForLane(selectedLane))", "footer must leave modulation mode when selection or lane support is unavailable");
assertContains(footer, "setDisplayMode(KnobDisplayMode::Hertz)", "filter cutoff knobs must display Hz/kHz");
assertContains(footer, "setScaleMode(KnobScaleMode::Logarithmic)", "filter cutoff knobs must use logarithmic movement");
assertContains(footer, "setDisplayMode(KnobDisplayMode::Pan)", "pan knobs must display L/C/R position");
assertContains(workspace, "auto searchRow = browserInner.removeFromTop(30);", "preset search and category controls must have a dedicated row");
assertContains(workspace, "auto sourceRow = browserInner.removeFromTop(28);", "preset source filters must have a separate row instead of overflowing search controls");
assertContains(workspace, "const int productHeight = 170;", "settings product controls must reserve a stable compact panel height");
assertContains(workspace, "layoutSettingsControl", "settings controls must use the shared two-column layout helper");
assertContains(workspace, "standaloneDeviceViewport.setViewedComponent(standaloneDeviceSelector.get(), false);", "standalone device selector must be hosted in a clipping viewport");
assertContains(workspace, "standaloneDeviceViewport.setBounds(deviceInner);", "standalone device viewport must stay inside the device panel");
assertContains(parameterIDs, 'juce::StringArray{"1/16", "1/8", "1/4", "1/2"}, 1', "step resolution choices must include 1/16 while defaulting to 1/8");
assertContains(processor, "case 0: return 0.25; // 1/16 note", "processor must map step resolution index 0 to 1/16");
assertContains(processBlock, "juce::jlimit(0, 3, getChoiceIndex(apvts, ParameterIDs::stepResolution, 1))", "processBlock must clamp four step resolution choices and default to 1/8");
assertContains(sequencerEngine, "case 0: stepDuration *= 0.25; break;", "SequencerEngine must support 1/16 timing");
assertContains(processorHeader, "processedWaveformTap", "processor must own a processed-output waveform tap");
assertContains(processorHeader, "getCurrentPpqPerStep", "processor must expose current musical step duration to the UI waveform");
assertContains(editor, "getProcessedWaveformTap().popForUi", "editor must read processed waveform samples from the processor");
assertContains(editor, "setVisibleSampleCount", "editor must sync waveform visible window to the 16-step musical loop span");
assertContains(waveformDisplayHeader, "pushInputSamples", "waveform display must expose an input waveform feed");
assertContains(waveformDisplayHeader, "pushOutputSamples", "waveform display must expose a processed-output waveform feed");
assertContains(waveformDisplayHeader, "setVisibleSampleCount", "waveform display must support a musical visible sample window");
assertContains(waveformDisplayHeader, "historyData", "waveform display must retain rolling sample history");
assertContains(waveformDisplayHeader, "displayGain", "waveform display must normalize quiet waveform windows for readability");
assertContains(waveformDisplay, "rebuildDisplayBins", "waveform display must render bins from rolling history, not only the latest timer chunk");
assertContains(waveformDisplay, "targetGain", "waveform display must compute a display-only waveform normalization gain");
assertContains(waveformDisplay, "isBeat", "waveform display must distinguish beat markers from regular step boundaries");
assertContains(waveformDisplay, "getSpaceMonoFont(9.0f)", "waveform display must show beat numbers in compact 9px font");
assertContains(waveformDisplay, "Colours::neonGreen.withAlpha(0.35f)", "playhead glow must be brighter for AAA visibility");
assertContains(waveformDisplay, "Colours::neonGreen.withAlpha(0.45f)", "playhead shadow lines must be visible");
assertContains(waveformDisplay, "Colours::white10.withAlpha(0.15f)", "waveform lane must have quarter-amplitude grid lines");
assertContains(waveformDisplay, "colour.withAlpha(0.88f)", "waveform stroke must be crisp and bright");
assertContains(processBlock, "processedWaveformTap.pushFromAudioThread(leftChannel, numSamples)", "processBlock must publish processed output waveform samples");
if (stepGridTimer.includes("\n    repaint();")) {
  fail("StepGrid timer must not repaint the whole grid on every animation tick");
}
if (stepGridConstructor.includes("startTimerHz(12);")) {
  fail("StepGrid chain animation timer must not run permanently when no chains exist");
}
assertContains(stepGridHeader, "hasAnimatedChains", "StepGrid must detect whether chain animation is needed");
assertContains(stepGridHeader, "updateChainAnimationTimer", "StepGrid must gate chain animation timer by actual chain state");
assertContains(stepGrid, "if (hasAnimatedChains())", "StepGrid must start chain animation only when animated chains exist");
assertContains(stepGrid, "if (! isTimerRunning())", "StepGrid must avoid restarting an already-running chain timer");
assertContains(stepGrid, "else if (isTimerRunning())", "StepGrid must stop the chain timer when chains are removed");
assertContains(stepCell, "if (tied == t)", "StepCell tied setter must skip no-op repaints");
assertContains(stepCell, "if (chained == c)", "StepCell chained setter must skip no-op repaints");
assertContains(stepCell, "if (chainable == c)", "StepCell chainable setter must skip no-op repaints");
assertContains(stepCell, "if (playing == p)", "StepCell playing setter must skip no-op repaints");
assertContains(stepCell, "if (selected == s)", "StepCell selected setter must skip no-op repaints");
assertContains(stepCellHeader, "getChainBadgeBounds", "StepCell must expose the chain badge geometry used for hit testing");
assertContains(stepCell, "const auto plusBounds = getChainBadgeBounds();", "StepCell must paint the shared chain badge bounds");
assertContains(stepGrid, "getChainBadgeBounds()", "StepGrid chain extension hit testing must use StepCell badge geometry");

[
  ["Grain", "sidebar must not advertise granular DSP until it exists"],
  ["Vinyl", "sidebar must not advertise vinyl stop/scratch DSP until it exists"],
  ["Stretch", "sidebar must not advertise time-stretch DSP until it exists"],
  ["Chaos", "sidebar must not advertise chaos synth DSP until it exists"],
  ["Formant", "sidebar must not advertise formant DSP until it exists"],
  ["Vowel", "sidebar must not advertise vowel filter DSP until it exists"],
  ["Talk", "sidebar must not advertise talk-box DSP until it exists"],
].forEach(([needle, message]) => {
  if (sidebar.includes(needle)) fail(message);
});

if (footer.includes("setSelectedSlot lane=")) {
  fail("footer must not log setSelectedSlot on hot UI selection paths");
}

if (footer.includes('g.drawText("STEP RES"')) {
  fail("footer must not paint STEP RES behind the existing stepResLabel");
}

if (sidebar.includes("Loop Alt")) {
  fail("loop lane must not advertise placeholder Loop Alt presets");
}

[
  "Forward 1/16",
  "Reverse 1/16",
  "Speed x4",
  "Tail 8x",
].forEach((presetName) => {
  if (!sidebar.includes(presetName)) fail(`missing loop lane preset: ${presetName}`);
});

assertContains(loopPresetMapping, "case 20:", "loop engine preset mapping must cover preset 20");
assertContains(loopPresetMapping, "beatSeconds * 0.25", "loop engine preset mapping must include 1/16 note windows");
assertContains(loopPresetMapping, "presetIndex >= 1 && presetIndex <= 4", "loop user slots must configure real loop parameters instead of falling through to dry defaults");
assertContains(loopEngineHeader, "loopBufferL", "LoopEngine must keep a frozen snapshot buffer for triggered loop playback");
assertContains(loopEngine, "refreshLoopSnapshot", "LoopEngine must refresh a frozen loop snapshot on trigger");
assertContains(loopEngine, "std::round(loopLengthSeconds * sampleRate)", "LoopEngine must round loop durations to sample counts instead of truncating");
assertContains(footer, '"LEN"', "loop footer controls must use loop-specific labels");
assertContains(footer, '"RATE"', "loop footer controls must use loop-specific labels");

[
  "DELAY PULSE",
  "FILTER CUTS",
  "CRUSH GRID",
  "LOOP CHOP",
  "NOTCH MOTION",
].forEach((presetName) => {
  if (!presetManager.includes(presetName)) fail(`missing alpha factory preset: ${presetName}`);
});

if (!process.exitCode) console.log("source smoke tests passed");
