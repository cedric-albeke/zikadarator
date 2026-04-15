# ZIKADARATOR Quickstart for Testers

## Goal

Get the plugin installed quickly and validate the basics in under 5 minutes.

## 1. Install

### Windows

Recommended:

1. Download `ZIKADARATOR-windows-installer`.
2. Run `ZIKADARATOR-Setup.exe`.
3. Keep **VST3 plugin** enabled.
4. Finish the installer.

Manual fallback:

1. Download `ZIKADARATOR-windows-zip`.
2. Extract it.
3. Copy `VST3/ZIKADARATOR.vst3` to `C:\Program Files\Common Files\VST3\`.

### macOS

Recommended:

1. Download `ZIKADARATOR-macos-installer`.
2. In Finder, **right-click** the installer and choose **Open**.
3. Finish the install.

Manual fallback:

1. Download `ZIKADARATOR-macos-zip`.
2. Extract it.
3. Copy the plugin bundles into `/Library/Audio/Plug-Ins/`.
4. Run the quarantine cleanup commands from `docs/INSTALL.md`.

## 2. Load it

Insert **ZIKADARATOR** on an audio track in your DAW.

- **Windows:** load the VST3 build
- **macOS:** load either the VST3 or AU build

## 3. Sanity checks

Please test these first:

1. Plugin appears in the DAW plugin browser
2. Editor opens without crashing
3. Sequencer page renders
4. Presets page renders and preset switching works
5. Settings page renders
6. Audio changes when steps/presets are edited

## 4. Quick per-platform notes

### Windows

- SmartScreen may warn because the installer is unsigned
- If your DAW does not see the plugin, trigger a VST3 rescan

### macOS

- The installer is unsigned, so Finder’s **right-click → Open** bypass is expected
- ZIP installs may require the `xattr -dr com.apple.quarantine ...` cleanup

## 5. What to report

When filing feedback, include:

- OS + version
- DAW + version
- Plugin format used (`VST3`, `AU`, `Standalone`)
- What screen you were on (`Sequencer`, `Presets`, `Settings`)
- Whether the problem is visual, audio, or both
- Screenshot or screen recording if possible

## 6. Debug logs

### Windows

If you are testing a debug-enabled build, collect:

- `%APPDATA%\ZIKADARATOR\UI-Debug.log`

### macOS

If available in a debug-enabled build, check the app support/config folder for `UI-Debug.log`.

## 7. Known testing caveats

- Unsigned macOS builds may require the Finder **right-click → Open** bypass and/or `xattr` cleanup.
- Windows may show SmartScreen warnings on unsigned installers.
