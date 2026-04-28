# ZIKADARATOR Engine Rebuild Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild the audio engine so ZIKADARATOR becomes a realtime-safe, host-synced, musically reliable sequenced FX plugin instead of a UI-loaded prototype with placeholder DSP.

**Architecture:** Keep the JUCE/VST3 shell, UI, state model, and packaging. Replace the audio core in layers: deterministic engine tests first, then proper history buffers, sample-accurate step scheduling, realtime-safe UI telemetry, corrected lane processors, and Ableton validation gates.

**Tech Stack:** JUCE 8.0.6, C++20, CMake, VST3, Ableton Live 12, pluginval, Node source-smoke checks.

## Progress - 2026-04-28

- Tasks 1-7 are implemented in the working tree, with commits intentionally skipped because this session has not been asked to commit.
- Latest verification after Task 7:
  - `node scripts\source-smoke-tests.mjs`: PASS
  - `cmake --build build --config Debug --target ZikadaEngineTests`: PASS
  - `ctest --test-dir build -C Debug --output-on-failure`: PASS
  - `cmake --build build --config Release --target ZikadaFX_VST3`: PASS
  - `pluginval --validate-in-process --strictness-level 5`: PASS
- Next implementation target: Task 8, pitch/time preset honesty for the alpha.

---

## Current Constraints

- Existing worktree has uncommitted changes in `.gitignore`, `CMakeLists.txt`, `src/PluginProcessor.cpp`, `src/PluginProcessor.h`, `src/engine/SequencerEngine.h`, `src/ui/panels/FooterPanel.cpp`, `src/ui/panels/FooterPanel.h`, and `scripts/source-smoke-tests.mjs`.
- Do not revert those changes.
- The current engine is mostly orchestrated from `src/PluginProcessor.cpp::processBlock`.
- The current `CircularAudioBuffer` uses `juce::AbstractFifo` as a history ring. Replace it for engine history, but keep `AbstractFifo` where the actual need is UI FIFO transfer.
- Ableton Live 12.3.7 logs are at `%APPDATA%\Ableton\Live 12.3.7\Preferences\Log.txt`.
- ZIKADARATOR debug log is at `%APPDATA%\ZIKADARATOR\UI-Debug.log`.

## Target File Structure

- Create `src/engine/RealtimeRingBuffer.h`
  - Owns continuously overwritten float history with deterministic `write`, `getSampleAgo`, and interpolated `getSampleAgoLinear`.
- Create `src/engine/StepScheduler.h`
  - Converts host/free clock positions into sample-accurate processing segments within each audio block.
- Create `src/engine/LaneProcessor.h`
  - Defines small structs and enums used by lane processors without depending on UI.
- Create `src/engine/SliceLaneProcessor.h/.cpp`
  - Replaces current `SliceEngine` trigger-copy behavior with history reads, selectable direction, crossfade, and no allocations during processing.
- Create `src/engine/LoopLaneProcessor.h/.cpp`
  - Replaces current `LoopEngine` integer history reads with interpolated reads, retrigger state, and loop wrap crossfade.
- Create `src/engine/WaveformTap.h`
  - A bounded audio-thread-to-UI FIFO for waveform display, with drops allowed when the UI is behind.
- Modify `src/engine/DelayEngine.h/.cpp`
  - Move to JUCE `dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>` or equivalent fractional delay.
- Modify `src/engine/FilterEngine.h/.cpp`
  - Make filter type names honest: 12 dB TPT low/high/band, implemented notch, implemented comb, and true 24 dB via cascaded sections.
- Modify `src/PluginProcessor.h/.cpp`
  - Use scheduler segments, lane processors, shared history, and waveform tap. Remove audio-thread UI calls and release file logging from hot paths.
- Modify `src/ui/components/WaveformDisplay.h/.cpp`
  - Pull samples from processor/tap on timer, never from `processBlock`.
- Modify `src/PluginEditor.cpp`
  - Wire UI timer/tap polling and keep debug logging compile-gated.
- Create `tests/engine/EngineTestMain.cpp`
  - Minimal assert-based test runner.
- Create `tests/engine/RealtimeRingBufferTests.cpp`
- Create `tests/engine/StepSchedulerTests.cpp`
- Create `tests/engine/LaneTransitionTests.cpp`
- Create `scripts/ableton-log-scan.ps1`
  - Summarizes ZIKADARATOR/Ableton errors, dropouts, parameter count lines, restore failures, and debug-log volume.
- Modify `scripts/source-smoke-tests.mjs`
  - Enforce no `getActiveEditor`/`Logger::writeToLog` in `processBlock`.
- Modify `docs/ARCHITECTURE.md`
  - Replace prototype engine description with actual rebuilt audio core.
- Create `docs/ENGINE_REBUILD.md`
  - Human-readable rationale and validation checklist.

---

## Task 1: Add Deterministic Engine Test Target

**Files:**
- Modify: `CMakeLists.txt`
- Create: `tests/engine/EngineTestMain.cpp`
- Create: `tests/engine/RealtimeRingBufferTests.cpp`
- Create: `tests/engine/StepSchedulerTests.cpp`

- [ ] **Step 1: Create the failing test runner**

Create `tests/engine/EngineTestMain.cpp`:

```cpp
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace zikada::tests {
void addRealtimeRingBufferTests(std::vector<std::pair<std::string, std::function<void()>>>& tests);
void addStepSchedulerTests(std::vector<std::pair<std::string, std::function<void()>>>& tests);
}

int main()
{
    std::vector<std::pair<std::string, std::function<void()>>> tests;
    zikada::tests::addRealtimeRingBufferTests(tests);
    zikada::tests::addStepSchedulerTests(tests);

    int failures = 0;
    for (const auto& [name, test] : tests)
    {
        try
        {
            test();
            std::cout << "PASS " << name << "\n";
        }
        catch (const std::exception& e)
        {
            ++failures;
            std::cerr << "FAIL " << name << ": " << e.what() << "\n";
        }
    }

    return failures == 0 ? 0 : 1;
}
```

- [ ] **Step 2: Create the first ring-buffer tests**

Create `tests/engine/RealtimeRingBufferTests.cpp`:

```cpp
#include "engine/RealtimeRingBuffer.h"

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(float actual, float expected, float tolerance, const char* message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(message);
}
}

namespace zikada::tests {
void addRealtimeRingBufferTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"RealtimeRingBuffer overwrites oldest samples", []
    {
        RealtimeRingBuffer buffer;
        buffer.prepare(4);
        const float first[] = {1.0f, 2.0f, 3.0f, 4.0f};
        const float second[] = {5.0f, 6.0f};
        buffer.write(first, 4);
        buffer.write(second, 2);

        requireNear(buffer.getSampleAgo(0), 6.0f, 0.0001f, "latest sample should be 6");
        requireNear(buffer.getSampleAgo(1), 5.0f, 0.0001f, "one sample ago should be 5");
        requireNear(buffer.getSampleAgo(2), 4.0f, 0.0001f, "two samples ago should be 4");
        requireNear(buffer.getSampleAgo(3), 3.0f, 0.0001f, "three samples ago should be 3");
        requireNear(buffer.getSampleAgo(4), 0.0f, 0.0001f, "out-of-history reads should return silence");
    }});

    tests.push_back({"RealtimeRingBuffer interpolates fractional history", []
    {
        RealtimeRingBuffer buffer;
        buffer.prepare(8);
        const float samples[] = {0.0f, 10.0f, 20.0f, 30.0f};
        buffer.write(samples, 4);

        requireNear(buffer.getSampleAgoLinear(0.5f), 25.0f, 0.0001f, "0.5 sample ago should interpolate");
        requireNear(buffer.getSampleAgoLinear(1.25f), 17.5f, 0.0001f, "1.25 samples ago should interpolate");
    }});
}
}
```

- [ ] **Step 3: Create the first scheduler tests**

Create `tests/engine/StepSchedulerTests.cpp`:

```cpp
#include "engine/StepScheduler.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
}

namespace zikada::tests {
void addStepSchedulerTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"StepScheduler splits blocks at host step boundaries", []
    {
        StepScheduler scheduler;
        const auto segments = scheduler.makeHostSegments({
            48000.0,
            120.0,
            0.5,
            0.499,
            128,
            true
        });

        require(segments.size() == 2, "block crossing a step boundary should produce two segments");
        require(segments[0].startSample == 0, "first segment starts at sample 0");
        require(segments[0].numSamples > 0, "first segment has samples");
        require(segments[0].stepIndex == 0, "first segment belongs to step 0");
        require(segments[1].startSample == segments[0].numSamples, "second segment follows first");
        require(segments[1].stepIndex == 1, "second segment belongs to step 1");
    }});

    tests.push_back({"StepScheduler returns one stopped segment when host is stopped", []
    {
        StepScheduler scheduler;
        const auto segments = scheduler.makeHostSegments({
            48000.0,
            120.0,
            0.5,
            12.0,
            256,
            false
        });

        require(segments.size() == 1, "stopped host should produce one segment");
        require(!segments[0].playing, "segment should be marked stopped");
        require(segments[0].numSamples == 256, "segment should preserve full block length");
    }});
}
}
```

- [ ] **Step 4: Wire the CMake target**

Append near the bottom of `CMakeLists.txt`:

```cmake
enable_testing()

add_executable(ZikadaEngineTests
    tests/engine/EngineTestMain.cpp
    tests/engine/RealtimeRingBufferTests.cpp
    tests/engine/StepSchedulerTests.cpp
)

target_include_directories(ZikadaEngineTests PRIVATE src)

target_sources(ZikadaEngineTests PRIVATE
    src/engine/CircularAudioBuffer.cpp
    src/engine/SequencerEngine.cpp
)

target_link_libraries(ZikadaEngineTests PRIVATE
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME ZikadaEngineTests COMMAND ZikadaEngineTests)
```

- [ ] **Step 5: Run the tests and verify they fail for missing new types**

Run:

```powershell
cmake --build build --config Debug --target ZikadaEngineTests
```

Expected: compile failure for missing `engine/RealtimeRingBuffer.h` and `engine/StepScheduler.h`.

- [ ] **Step 6: Commit**

```powershell
git add CMakeLists.txt tests/engine
git commit -m "test: add engine test harness"
```

If the user does not want commits yet, skip the commit and keep the file changes staged logically.

---

## Task 2: Implement RealtimeRingBuffer

**Files:**
- Create: `src/engine/RealtimeRingBuffer.h`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add the implementation**

Create `src/engine/RealtimeRingBuffer.h`:

```cpp
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace zikada {

class RealtimeRingBuffer
{
public:
    void prepare(int newCapacity)
    {
        capacity = std::max(1, newCapacity);
        data.assign(static_cast<size_t>(capacity), 0.0f);
        writePosition = 0;
        validSamples = 0;
    }

    void reset()
    {
        std::fill(data.begin(), data.end(), 0.0f);
        writePosition = 0;
        validSamples = 0;
    }

    void write(const float* samples, int numSamples)
    {
        if (samples == nullptr || numSamples <= 0 || capacity <= 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            data[static_cast<size_t>(writePosition)] = samples[i];
            writePosition = (writePosition + 1) % capacity;
            validSamples = std::min(capacity, validSamples + 1);
        }
    }

    [[nodiscard]] float getSampleAgo(int samplesAgo) const
    {
        if (samplesAgo < 0 || samplesAgo >= validSamples || capacity <= 0)
            return 0.0f;

        int index = writePosition - 1 - samplesAgo;
        while (index < 0)
            index += capacity;
        return data[static_cast<size_t>(index % capacity)];
    }

    [[nodiscard]] float getSampleAgoLinear(float samplesAgo) const
    {
        if (samplesAgo < 0.0f)
            return getSampleAgo(0);

        const int floorAgo = static_cast<int>(std::floor(samplesAgo));
        const float fraction = samplesAgo - static_cast<float>(floorAgo);
        const float a = getSampleAgo(floorAgo);
        const float b = getSampleAgo(floorAgo + 1);
        return a + (b - a) * fraction;
    }

    [[nodiscard]] int getCapacity() const { return capacity; }
    [[nodiscard]] int getValidSamples() const { return validSamples; }

private:
    std::vector<float> data;
    int capacity{0};
    int writePosition{0};
    int validSamples{0};
};

}
```

- [ ] **Step 2: Add the header to the test target**

No CMake source entry is needed because this is header-only.

- [ ] **Step 3: Run the ring-buffer tests**

Run:

```powershell
cmake --build build --config Debug --target ZikadaEngineTests
ctest --test-dir build -C Debug --output-on-failure
```

Expected: ring-buffer tests pass; scheduler tests still fail until Task 3.

- [ ] **Step 4: Commit**

```powershell
git add src/engine/RealtimeRingBuffer.h
git commit -m "feat: add realtime history ring buffer"
```

---

## Task 3: Implement Sample-Accurate StepScheduler

**Files:**
- Create: `src/engine/StepScheduler.h`
- Test: `tests/engine/StepSchedulerTests.cpp`

- [ ] **Step 1: Add the scheduler**

Create `src/engine/StepScheduler.h`:

```cpp
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace zikada {

class StepScheduler
{
public:
    struct HostBlock
    {
        double sampleRate{44100.0};
        double bpm{120.0};
        double ppqPerStep{0.5};
        double startPpq{0.0};
        int numSamples{0};
        bool playing{false};
    };

    struct Segment
    {
        int startSample{0};
        int numSamples{0};
        int stepIndex{0};
        double phaseStart{0.0};
        double phaseDelta{0.0};
        bool playing{false};
        bool startsNewStep{false};
    };

    [[nodiscard]] std::vector<Segment> makeHostSegments(const HostBlock& block) const
    {
        std::vector<Segment> result;
        if (block.numSamples <= 0)
            return result;

        const double safeBpm = block.bpm > 0.0 ? block.bpm : 120.0;
        const double safeSampleRate = block.sampleRate > 0.0 ? block.sampleRate : 44100.0;
        const double safePpqPerStep = block.ppqPerStep > 0.0 ? block.ppqPerStep : 0.5;
        const double ppqPerSample = (safeBpm / 60.0) / safeSampleRate;
        const double samplesPerStep = safePpqPerStep / ppqPerSample;
        const double phaseDelta = samplesPerStep > 0.0 ? 1.0 / samplesPerStep : 0.0;

        if (!block.playing || ppqPerSample <= 0.0)
        {
            result.push_back({0, block.numSamples, stepFromPpq(block.startPpq, safePpqPerStep),
                              phaseFromPpq(block.startPpq, safePpqPerStep), phaseDelta, false, false});
            return result;
        }

        int cursor = 0;
        bool first = true;
        while (cursor < block.numSamples)
        {
            const double cursorPpq = block.startPpq + static_cast<double>(cursor) * ppqPerSample;
            const int step = stepFromPpq(cursorPpq, safePpqPerStep);
            const double phase = phaseFromPpq(cursorPpq, safePpqPerStep);
            const double nextBoundaryPpq = (std::floor(cursorPpq / safePpqPerStep) + 1.0) * safePpqPerStep;
            int samplesToBoundary = static_cast<int>(std::ceil((nextBoundaryPpq - cursorPpq) / ppqPerSample));
            samplesToBoundary = std::clamp(samplesToBoundary, 1, block.numSamples - cursor);

            result.push_back({cursor, samplesToBoundary, step, phase, phaseDelta, true, !first && phase <= phaseDelta * 1.5});
            cursor += samplesToBoundary;
            first = false;
        }

        if (!result.empty())
            result.front().startsNewStep = phaseFromPpq(block.startPpq, safePpqPerStep) <= phaseDelta * 1.5;

        return result;
    }

private:
    static int stepFromPpq(double ppq, double ppqPerStep)
    {
        const auto rawStep = static_cast<int>(std::floor(ppq / ppqPerStep));
        return ((rawStep % 16) + 16) % 16;
    }

    static double phaseFromPpq(double ppq, double ppqPerStep)
    {
        const double raw = ppq / ppqPerStep;
        return raw - std::floor(raw);
    }
};

}
```

- [ ] **Step 2: Run scheduler tests**

Run:

```powershell
cmake --build build --config Debug --target ZikadaEngineTests
ctest --test-dir build -C Debug --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 3: Commit**

```powershell
git add src/engine/StepScheduler.h tests/engine/StepSchedulerTests.cpp
git commit -m "feat: add sample accurate step scheduler"
```

---

## Task 4: Remove Audio-Thread UI Calls And Hot Logging

**Files:**
- Create: `src/engine/WaveformTap.h`
- Modify: `src/PluginProcessor.h`
- Modify: `src/PluginProcessor.cpp`
- Modify: `src/PluginEditor.cpp`
- Modify: `src/ui/components/WaveformDisplay.h`
- Modify: `src/ui/components/WaveformDisplay.cpp`
- Modify: `scripts/source-smoke-tests.mjs`

- [ ] **Step 1: Strengthen source smoke tests**

In `scripts/source-smoke-tests.mjs`, add:

```js
if (processBlock.includes("getActiveEditor(")) {
  fail("processBlock must not touch the active editor from the audio thread");
}

if (processBlock.includes("Logger::writeToLog")) {
  fail("processBlock must not write logs from the audio thread");
}
```

- [ ] **Step 2: Run smoke test and verify failure**

Run:

```powershell
node scripts/source-smoke-tests.mjs
```

Expected: FAIL because current `processBlock` calls `getActiveEditor()`.

- [ ] **Step 3: Add WaveformTap**

Create `src/engine/WaveformTap.h`:

```cpp
#pragma once

#include <algorithm>
#include <atomic>
#include <vector>

namespace zikada {

class WaveformTap
{
public:
    void prepare(int capacity)
    {
        const int safeCapacity = std::max(1, capacity);
        buffer.assign(static_cast<size_t>(safeCapacity), 0.0f);
        writePosition.store(0);
        available.store(0);
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePosition.store(0);
        available.store(0);
    }

    void pushFromAudioThread(const float* samples, int numSamples)
    {
        if (samples == nullptr || numSamples <= 0 || buffer.empty())
            return;

        int pos = writePosition.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            buffer[static_cast<size_t>(pos)] = samples[i];
            pos = (pos + 1) % static_cast<int>(buffer.size());
        }

        writePosition.store(pos, std::memory_order_release);
        available.store(std::min(static_cast<int>(buffer.size()), available.load(std::memory_order_relaxed) + numSamples),
                        std::memory_order_release);
    }

    int copyLatestForUi(float* destination, int maxSamples) const
    {
        if (destination == nullptr || maxSamples <= 0 || buffer.empty())
            return 0;

        const int count = std::min(maxSamples, available.load(std::memory_order_acquire));
        const int pos = writePosition.load(std::memory_order_acquire);
        const int size = static_cast<int>(buffer.size());
        const int start = (pos - count + size) % size;

        for (int i = 0; i < count; ++i)
            destination[i] = buffer[static_cast<size_t>((start + i) % size)];

        return count;
    }

private:
    std::vector<float> buffer;
    std::atomic<int> writePosition{0};
    std::atomic<int> available{0};
};

}
```

- [ ] **Step 4: Wire processor-side tap**

In `src/PluginProcessor.h`, include `engine/WaveformTap.h`, add:

```cpp
WaveformTap waveformTap;
public:
    WaveformTap& getWaveformTap() { return waveformTap; }
```

In `prepareToPlay`, call:

```cpp
waveformTap.prepare(static_cast<int>(newSampleRate * 2.0));
```

In `processBlock`, replace all editor waveform calls with:

```cpp
waveformTap.pushFromAudioThread(buffer.getReadPointer(0), buffer.getNumSamples());
```

- [ ] **Step 5: Pull waveform data from UI timer**

In `PluginEditor` timer or existing waveform timer path, copy latest tap data into `WaveformDisplay` from the message thread:

```cpp
std::array<float, 2048> waveformScratch{};
const int copied = processor.getWaveformTap().copyLatestForUi(waveformScratch.data(), static_cast<int>(waveformScratch.size()));
if (copied > 0)
    waveformDisplay.pushSamples(waveformScratch.data(), copied);
```

- [ ] **Step 6: Run smoke/build**

Run:

```powershell
node scripts/source-smoke-tests.mjs
cmake --build build --config Release --target ZikadaFX
```

Expected: smoke test passes, release build succeeds.

- [ ] **Step 7: Commit**

```powershell
git add src/engine/WaveformTap.h src/PluginProcessor.h src/PluginProcessor.cpp src/PluginEditor.cpp src/ui/components/WaveformDisplay.* scripts/source-smoke-tests.mjs
git commit -m "fix: remove audio thread ui coupling"
```

---

## Task 5: Port Slice And Loop History To RealtimeRingBuffer

**Files:**
- Modify: `src/engine/SliceEngine.h`
- Modify: `src/engine/SliceEngine.cpp`
- Modify: `src/engine/LoopEngine.h`
- Modify: `src/engine/LoopEngine.cpp`
- Test: `tests/engine/LaneTransitionTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add lane transition test file**

Create `tests/engine/LaneTransitionTests.cpp` with tests that assert no silence after a filled history trigger and no discontinuity larger than a configured threshold at loop wrap:

```cpp
#include "engine/LoopEngine.h"
#include "engine/SliceEngine.h"

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
}

namespace zikada::tests {
void addLaneTransitionTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"SliceEngine produces nonzero audio from filled history", []
    {
        SliceEngine engine;
        engine.prepare(48000.0, 512);
        engine.setTempo(120.0);

        float left[4096]{};
        float right[4096]{};
        for (int i = 0; i < 4096; ++i)
        {
            left[i] = 0.25f;
            right[i] = 0.25f;
        }

        engine.writeToBuffer(left, right, 4096);
        engine.triggerSlice(0);

        float outL[128]{};
        float outR[128]{};
        engine.process(outL, outR, 128);

        float sum = 0.0f;
        for (float sample : outL)
            sum += std::abs(sample);
        require(sum > 0.1f, "triggered slice should emit buffered audio");
    }});
}
}
```

Update `EngineTestMain.cpp` to declare/call `addLaneTransitionTests`.

- [ ] **Step 2: Run and verify current behavior**

Run:

```powershell
cmake --build build --config Debug --target ZikadaEngineTests
ctest --test-dir build -C Debug --output-on-failure
```

Expected: current behavior may pass the nonzero slice case, but additional loop discontinuity tests should fail until crossfades are implemented.

- [ ] **Step 3: Replace `CircularAudioBuffer` usage**

Update `SliceEngine` and `LoopEngine` includes:

```cpp
#include "engine/RealtimeRingBuffer.h"
```

Replace `CircularAudioBuffer bufferL/bufferR` members with:

```cpp
RealtimeRingBuffer bufferL;
RealtimeRingBuffer bufferR;
```

Do the same in `SliceEngine`.

- [ ] **Step 4: Add loop interpolation and crossfade**

In `LoopEngine::process`, replace integer reads with:

```cpp
const float phaseSamples = reverse
    ? phase
    : static_cast<float>(loopLengthSamples - 1) - phase;
const float loopL = bufferL.getSampleAgoLinear(phaseSamples);
const float loopR = bufferR.getSampleAgoLinear(phaseSamples);
```

Add a short crossfade at loop wrap:

```cpp
const int fadeSamples = juce::jlimit(8, 256, loopLengthSamples / 64);
const float fadeIn = juce::jlimit(0.0f, 1.0f, phase / static_cast<float>(fadeSamples));
const float fadeOut = juce::jlimit(0.0f, 1.0f, (static_cast<float>(loopLengthSamples) - phase) / static_cast<float>(fadeSamples));
const float edgeFade = juce::jmin(fadeIn, fadeOut);
```

Apply `edgeFade` to the loop signal before wet mixing.

- [ ] **Step 5: Run engine tests**

Run:

```powershell
cmake --build build --config Debug --target ZikadaEngineTests
ctest --test-dir build -C Debug --output-on-failure
```

Expected: all engine tests pass.

- [ ] **Step 6: Commit**

```powershell
git add src/engine/RealtimeRingBuffer.h src/engine/SliceEngine.* src/engine/LoopEngine.* tests/engine CMakeLists.txt
git commit -m "fix: use realtime history buffers for slice and loop"
```

---

## Task 6: Segment processBlock With StepScheduler

**Files:**
- Modify: `src/PluginProcessor.h`
- Modify: `src/PluginProcessor.cpp`
- Test: `scripts/source-smoke-tests.mjs`

- [ ] **Step 1: Add processor scheduler members**

In `PluginProcessor.h`:

```cpp
#include "engine/StepScheduler.h"

StepScheduler stepScheduler;
void processSegment(float* leftChannel, float* rightChannel, int numSamples,
                    const StepScheduler::Segment& segment, double bpm, double ppqPerStep);
```

- [ ] **Step 2: Refactor current lane processing into `processSegment`**

Move the body of lane processing from `processBlock` into `processSegment`. Keep parameter snapshot reads once per outer `processBlock`. `processSegment` must only process `numSamples` beginning at the segment pointer.

Required call shape:

```cpp
for (const auto& segment : segments)
{
    processSegment(leftChannel + segment.startSample,
                   rightChannel + segment.startSample,
                   segment.numSamples,
                   segment,
                   bpm,
                   blockPpqPerStep);
}
```

- [ ] **Step 3: Trigger lane changes at segment boundaries**

Replace block-level `if (effStep != lastEffectiveSteps[lane])` logic with segment-level logic. The trigger must happen before processing the segment whose `stepIndex` owns the new step.

- [ ] **Step 4: Run validation**

Run:

```powershell
node scripts/source-smoke-tests.mjs
cmake --build build --config Release --target ZikadaFX
```

Expected: no smoke failures and no release build failures.

- [ ] **Step 5: Commit**

```powershell
git add src/PluginProcessor.* scripts/source-smoke-tests.mjs
git commit -m "feat: process audio in sample accurate step segments"
```

---

## Task 7: Correct Delay And Filter DSP Contracts

**Files:**
- Modify: `src/engine/DelayEngine.h`
- Modify: `src/engine/DelayEngine.cpp`
- Modify: `src/engine/FilterEngine.h`
- Modify: `src/engine/FilterEngine.cpp`
- Test: `tests/engine/LaneTransitionTests.cpp`

- [ ] **Step 1: Add delay smoothing test**

Add a test that changes delay time while processing a constant signal and asserts no NaN/Inf and no single-sample jump greater than `1.5f`.

- [ ] **Step 2: Replace integer delay with fractional delay**

Use JUCE `dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>`.

`DelayEngine` members:

```cpp
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine{96000};
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySamplesSmoothed;
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> feedbackSmoothed;
```

In `prepare`, call:

```cpp
delayLine.prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 2});
delayLine.setMaximumDelayInSamples(static_cast<int>(sampleRate * 2.0));
delaySamplesSmoothed.reset(sampleRate, 0.02);
mixSmoothed.reset(sampleRate, 0.01);
feedbackSmoothed.reset(sampleRate, 0.01);
```

- [ ] **Step 3: Fix filter naming and implementation**

Implement `LowPass24` and `HighPass24` as cascaded TPT sections. Implement `BandReject` as dry minus bandpass, not negative bandpass. Implement `Comb` with a short fractional delay feedback/feedforward path or rename it to an implemented type before exposing it.

- [ ] **Step 4: Run tests/build**

```powershell
cmake --build build --config Debug --target ZikadaEngineTests
ctest --test-dir build -C Debug --output-on-failure
cmake --build build --config Release --target ZikadaFX
```

- [ ] **Step 5: Commit**

```powershell
git add src/engine/DelayEngine.* src/engine/FilterEngine.* tests/engine/LaneTransitionTests.cpp
git commit -m "fix: correct delay and filter dsp behavior"
```

---

## Task 8: Decide Pitch/Time Strategy And Remove False Labels

**Files:**
- Modify: `src/engine/PitchEngine.h`
- Modify: `src/engine/PitchEngine.cpp`
- Modify: `src/PluginProcessor.cpp`
- Modify: `src/ui/panels/SidebarPanel.cpp`
- Optional create: `third_party/signalsmith-stretch/`

- [ ] **Step 1: Choose strategy**

For alpha-quality reliability, choose one:

1. Integrate Signalsmith Stretch for pitch/time presets and account for latency.
2. Remove or rename pitch/stretch/grain presets so they do not claim functionality the engine does not implement.

Recommended for next alpha: choose option 2 for immediate honesty, then option 1 as a separate feature branch.

- [ ] **Step 2: Rename misleading presets if not integrating a library**

In `SidebarPanel.cpp`, rename labels such as `Stretch`, `Grain`, `Vinyl`, and `ChaosSynth` to implemented behaviors only. Examples:

```cpp
{"PITCH UP", "Pitch color", "Simple experimental pitch color", 7}
{"PITCH DOWN", "Pitch color", "Simple experimental pitch color", 8}
{"CRUSH", "Bit reduction", "Bit depth and sample-rate reduction", 17}
```

- [ ] **Step 3: Gate non-production pitch behavior**

Add comments and UI naming that mark the current pitch engine as experimental until replaced.

- [ ] **Step 4: Build**

```powershell
cmake --build build --config Release --target ZikadaFX
```

- [ ] **Step 5: Commit**

```powershell
git add src/engine/PitchEngine.* src/PluginProcessor.cpp src/ui/panels/SidebarPanel.cpp
git commit -m "chore: align preset labels with implemented dsp"
```

---

## Task 9: Add Ableton Validation Loop

**Files:**
- Create: `scripts/ableton-log-scan.ps1`
- Modify: `docs/RELEASE_TESTING.md`
- Modify: `docs/QUICKSTART.md`

- [ ] **Step 1: Create log scanner**

Create `scripts/ableton-log-scan.ps1`:

```powershell
$ErrorActionPreference = "Stop"

$abletonLog = Join-Path $env:APPDATA "Ableton\Live 12.3.7\Preferences\Log.txt"
$zikLog = Join-Path $env:APPDATA "ZIKADARATOR\UI-Debug.log"
$usageDir = Join-Path $env:APPDATA "Ableton\Live Reports\Usage"

Write-Host "Ableton log: $abletonLog"
if (Test-Path $abletonLog) {
    Select-String -LiteralPath $abletonLog -Pattern "ZIKADARATOR|Restore .*failed|parameter count|crash|fatal|drop" |
        Select-Object -Last 80 |
        ForEach-Object { $_.Line }
}

Write-Host ""
Write-Host "ZIKADARATOR log: $zikLog"
if (Test-Path $zikLog) {
    $item = Get-Item -LiteralPath $zikLog
    Write-Host ("Size bytes: {0}" -f $item.Length)
    Write-Host ("Last write: {0}" -f $item.LastWriteTime)
    $hotCount = (Select-String -LiteralPath $zikLog -Pattern "setSelectedSlot|processBlock|createEditor|destructed|constructed" | Measure-Object).Count
    Write-Host ("Hot debug line count: {0}" -f $hotCount)
}

Write-Host ""
Write-Host "Latest Ableton usage logs:"
if (Test-Path $usageDir) {
    Get-ChildItem -LiteralPath $usageDir -Filter *.log |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 3 FullName, Length, LastWriteTime
}
```

- [ ] **Step 2: Run after Ableton manual test**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ableton-log-scan.ps1
```

Expected: no restore failure for ZIKADARATOR, no fatal lines, debug log not growing rapidly during idle UI.

- [ ] **Step 3: Document Ableton acceptance test**

Add to `docs/RELEASE_TESTING.md`:

```markdown
### Ableton Live 12 Audio Acceptance

- Insert ZIKADARATOR on an audio loop track.
- Run transport for 2 minutes at 120 BPM with buffer size 128 or 256.
- Toggle 8-12 steps across SLICE, LOOP, ENVELOPE, FX1, FILTER.
- Confirm no audible hard clicks at every step boundary in the default INIT pattern.
- Confirm Ableton's usage report shows no audio dropouts for the test window.
- Run `powershell -ExecutionPolicy Bypass -File scripts\ableton-log-scan.ps1`.
```

- [ ] **Step 4: Commit**

```powershell
git add scripts/ableton-log-scan.ps1 docs/RELEASE_TESTING.md docs/QUICKSTART.md
git commit -m "test: add ableton validation scanner"
```

---

## Task 10: Rebuild Preset And State Contract After DSP Stabilizes

**Files:**
- Modify: `src/state/UserSlotData.h`
- Modify: `src/state/PresetManager.cpp`
- Modify: `src/ui/panels/WorkspacePanel.cpp`
- Modify: `docs/ARCHITECTURE.md`

- [ ] **Step 1: Freeze alpha automation scope**

Keep DAW automation limited to global controls, lane mix/mute/solo, clock, and step active toggles for now. Do not expose all per-step preset selections until the DSP stabilizes.

- [ ] **Step 2: Expand user slot data only for implemented DSP**

Add specific fields only when the corresponding DSP is real. For delay/filter alpha, add:

```cpp
float delayTimeBeats = 0.5f;
float delayFeedback = 0.3f;
float delayTone = 0.5f;
float filterCutoff = 2000.0f;
float filterResonance = 0.707f;
```

Do not add parameters for stretch/grain/formant until those processors exist.

- [ ] **Step 3: Update factory presets**

Create 8-12 reliable factory presets that use only corrected lanes. Replace ambitious placeholder presets until pitch/time/grain is implemented.

- [ ] **Step 4: Commit**

```powershell
git add src/state/UserSlotData.h src/state/PresetManager.cpp src/ui/panels/WorkspacePanel.cpp docs/ARCHITECTURE.md
git commit -m "feat: rebuild alpha preset contract"
```

---

## Task 11: Full Verification Gate

**Files:**
- Modify as needed only if verification finds issues.

- [ ] **Step 1: Source smoke**

```powershell
node scripts/source-smoke-tests.mjs
```

Expected: `source smoke tests passed`.

- [ ] **Step 2: Engine tests**

```powershell
cmake --build build --config Debug --target ZikadaEngineTests
ctest --test-dir build -C Debug --output-on-failure
```

Expected: all CTest tests pass.

- [ ] **Step 3: Release build**

```powershell
cmake --build build --config Release --target ZikadaFX
```

Expected: release VST3 builds successfully.

- [ ] **Step 4: pluginval**

Run pluginval against the built or installed VST3 at strictness 5.

Expected: pluginval success.

- [ ] **Step 5: Ableton Live 12 manual test**

Install/copy the VST3, rescan if needed, load on an audio track, run the acceptance test from Task 9, then run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ableton-log-scan.ps1
```

Expected: no restore failure, no crash, no sustained debug-log growth, no reported dropouts in the test window.

- [ ] **Step 6: Final status note**

Update `docs/ENGINE_REBUILD.md` with:

```markdown
## Latest Verification

- Source smoke: PASS
- Engine tests: PASS
- Release build: PASS
- pluginval strictness 5: PASS
- Ableton Live 12 manual audio test: PASS
- Known remaining DSP limitations:
  - Pitch/time/grain advanced algorithms are not production until Signalsmith/Rubber Band or equivalent is integrated.
```

---

## Scope Gaps Explicitly Deferred

- Reorderable lanes are not part of this rebuild pass. The fixed lane order stays until the segment scheduler and processors are stable.
- 300 factory presets are not part of this rebuild pass. The alpha should ship fewer reliable presets.
- MIDI Learn and performance lists are deferred.
- GPU/vector UI polish is deferred.
- True transient detection for slicing is planned after the history/scheduler rewrite. The rebuild should leave room for Bello-style onset detection but not block on it.

## Done Criteria

- `processBlock` has no UI calls, file logging, dynamic allocation in normal block sizes, or direct component access.
- Step changes are processed at sample offsets inside the block, not rounded to block boundaries.
- Slice and loop history uses overwrite ring buffers, not FIFO readiness semantics.
- Delay/filter names match implemented DSP.
- All transition-heavy lane tests pass.
- Ableton Live 12 can run a 2-minute loop test without hard clicks, restore failures, or log storms.
