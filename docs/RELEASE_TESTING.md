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
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\validate-windows.ps1 -BuildDir build\verify -Configuration Debug -Generator "Visual Studio 17 2022" -Architecture x64
```

Expected:

- Source smoke passes.
- Engine tests pass through CTest.
- VST3 builds.
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

After the manual pass, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ableton-log-scan.ps1
```

For a clean read, delete or archive `%APPDATA%\ZIKADARATOR\UI-Debug.log` immediately before the manual test. Otherwise the scanner may still show historical hot debug lines from earlier builds.

Expected scanner result:

- No ZIKADARATOR crash/fatal/exception lines.
- No restore failures.
- No sustained `processBlock` or `setSelectedSlot` debug-log growth.
- Ableton usage logs exist for the test window when Live report logging is enabled.

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
