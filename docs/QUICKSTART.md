# ZIKADARATOR Quickstart for Testers

## Goal

Get the plugin installed quickly and validate the basics in under 5 minutes.

## 1. Install

- **Windows:** use `ZIKADARATOR-Setup.exe`
- **macOS:** use `ZIKADARATOR-Installer.pkg` or the ZIP + `xattr` instructions from `docs/INSTALL.md`

## 2. Load it

Insert **ZIKADARATOR** on an audio track in your DAW.

## 3. Sanity checks

Please test these first:

1. Plugin appears in the DAW plugin browser
2. Editor opens without crashing
3. Sequencer page renders
4. Presets page renders and preset switching works
5. Settings page renders
6. Audio changes when steps/presets are edited

## 4. What to report

When filing feedback, include:

- OS + version
- DAW + version
- Plugin format used (`VST3`, `AU`, `Standalone`)
- What screen you were on (`Sequencer`, `Presets`, `Settings`)
- Whether the problem is visual, audio, or both
- Screenshot or screen recording if possible

## 5. Debug logs

### Windows

If you are testing a debug-enabled build, collect:

- `%APPDATA%\ZIKADARATOR\UI-Debug.log`

### macOS

If available in a debug-enabled build, check the app support/config folder for `UI-Debug.log`.

## 6. Known testing caveats

- Unsigned macOS builds may require the Finder **right-click → Open** bypass and/or `xattr` cleanup.
- Windows may show SmartScreen warnings on unsigned installers.
