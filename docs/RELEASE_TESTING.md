# ZIKADARATOR Release Testing Notes

Use this checklist before sharing a build with external testers.

## Artifact checklist

### Windows

- `ZIKADARATOR-windows-vst3`
- `ZIKADARATOR-windows-zip`
- `ZIKADARATOR-windows-installer`

### macOS

- `ZIKADARATOR-macos-vst3`
- `ZIKADARATOR-macos-au`
- `ZIKADARATOR-macos-zip`
- `ZIKADARATOR-macos-installer`

## Validation checklist

### Local automated gate

Run these before sharing a tester build:

```powershell
node scripts\source-smoke-tests.mjs
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target ZikadaEngineTests
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Debug --output-on-failure
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target ZikadaFX_VST3
.tools\pluginval\pluginval.exe --validate-in-process --strictness-level 5 --validate "C:\Development\zikadarator\build\ZikadaFX_artefacts\Release\VST3\ZIKADARATOR.vst3"
```

Expected:

- Source smoke passes.
- Engine tests pass through CTest.
- Release VST3 builds.
- pluginval exits with code 0 at strictness level 5.

### Windows

- Installer runs successfully
- VST3 lands in `C:\Program Files\Common Files\VST3`
- Standalone launches
- Plugin scans in at least one DAW

### Ableton Live 12 audio acceptance

- Insert ZIKADARATOR on an audio loop track.
- Run transport for at least 2 minutes at 120 BPM with a 128 or 256 sample buffer.
- Toggle 8-12 steps across SLICE, LOOP, ENVELOPE, FX1, FILTER, and FX2.
- Confirm step boundaries do not produce obvious hard clicks in the default pattern.
- Confirm the editor can be resized and keeps its 3:2 aspect ratio.
- Confirm no crash, restore failure, or sustained debug-log growth.

Relevant Windows log locations:

- Ableton: `%APPDATA%\Ableton\Live 12.3.7\Preferences\Log.txt`
- ZIKADARATOR UI debug: `%APPDATA%\ZIKADARATOR\UI-Debug.log`

### macOS

- PKG installs without path mistakes
- VST3 lands in `/Library/Audio/Plug-Ins/VST3`
- AU lands in `/Library/Audio/Plug-Ins/Components`
- Standalone lands in `/Applications`
- Quarantine removal runs successfully in postinstall

## Sign-off notes to include with builds

- exact git commit SHA
- workflow run URL
- known issues / test focus areas
- whether logs are enabled
