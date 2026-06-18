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
const stepGridHeader = read("src/ui/components/StepGrid.h");
const stepGrid = read("src/ui/components/StepGrid.cpp");
const knobHeader = read("src/ui/components/Knob.h");
const knob = read("src/ui/components/Knob.cpp");
const presetManager = read("src/state/PresetManager.cpp");
const parameterIDs = read("src/state/ParameterIDs.h");
const sequencerEngine = read("src/engine/SequencerEngine.cpp");
const sliceEngine = read("src/engine/SliceEngine.cpp");
const envelopeShape = read("src/engine/EnvelopeShape.cpp");
const loopEngine = read("src/engine/LoopEngine.cpp");
const loopEngineHeader = read("src/engine/LoopEngine.h");
const waveformDisplayHeader = read("src/ui/components/WaveformDisplay.h");
const waveformDisplay = read("src/ui/components/WaveformDisplay.cpp");
const headerPanelHeader = read("src/ui/panels/HeaderPanel.h");
const headerPanel = read("src/ui/panels/HeaderPanel.cpp");
const windowsPackageScript = read("scripts/package-windows-release.ps1");
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

assertContains(processorHeader, "dryLeftBuffer", "processor must own reusable dry scratch buffers");
assertContains(processorHeader, "wetLeftBuffer", "processor must own reusable wet scratch buffers");
assertContains(processorHeader, "laneInputLeftBuffer", "processor must own reusable per-lane input scratch buffers");
assertContains(processorHeader, "monoRightBuffer", "processor must own reusable mono right-side scratch");
assertContains(processorHeader, "ensureScratchBuffers", "processor must expose scratch-buffer sizing helper");
assertContains(processBlock, "monoRightBuffer.data()", "mono processing must avoid aliasing left/right pointers");
assertContains(processSegment, "0.5f * (outLeft + outRight)", "mono processing must fold rendered stereo output to mono");
assertContains(processBlock, "sequencerState.getSnapshot()", "processBlock must capture a sequencer snapshot for audio rendering");
assertContains(processSegment, "SequencerState::Snapshot", "processSegment must render from an immutable sequencer snapshot");
assertContains(processSegment, "sequencerSnapshot.getStepData", "processSegment must read step data from the captured snapshot");
assertContains(processSegment, "sequencerSnapshot.getUserSlot", "processSegment must read user slots from the captured snapshot");
assertContains(windowsPackageScript, "moduleinfo.json", "Windows package script must validate VST3 moduleinfo metadata");
assertContains(windowsPackageScript, "packaging/windows/moduleinfo.json", "Windows package script must use tracked moduleinfo fallback when JUCE helper is blocked");
assertContains(windowsPackageScript, "Length -eq 0", "Windows package script must repair zero-byte moduleinfo outputs");
assertContains(windowsPackageScript, "Compress-Archive", "Windows package script must create a tester-facing ZIP package");
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

const stepCell = read("src/ui/components/StepCell.cpp");
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
assertContains(footer, "setDisplayMode(KnobDisplayMode::Hertz)", "filter cutoff knobs must display Hz/kHz");
assertContains(footer, "setScaleMode(KnobScaleMode::Logarithmic)", "filter cutoff knobs must use logarithmic movement");
assertContains(footer, "setDisplayMode(KnobDisplayMode::Pan)", "pan knobs must display L/C/R position");
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
