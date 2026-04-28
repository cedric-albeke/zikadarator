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
const processBlock = extractFunction(
  processor,
  "void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)",
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
assertContains(processorHeader, "ensureScratchBuffers", "processor must expose scratch-buffer sizing helper");
assertContains(editor, "setFixedAspectRatio", "plugin editor must constrain resizing to a fixed aspect ratio");

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

if (!process.exitCode) console.log("source smoke tests passed");
