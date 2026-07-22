# ZIKADARATOR Install Guide

This guide is for external testers installing CI builds of **ZIKADARATOR**.

## Windows

### Recommended: Installer

1. Download `ZIKADARATOR-windows-installer`.
2. Run `ZIKADARATOR-Setup.exe`.
3. Leave **VST3 plugin** enabled.
4. Optionally install the standalone app too.
5. Rescan plugins in your DAW.

Installed paths:

- VST3: `C:\Program Files\Common Files\VST3\ZIKADARATOR.vst3`
- Standalone: `C:\Program Files\ZIKADARATOR\ZIKADARATOR.exe`

### Manual: ZIP

1. Download `ZIKADARATOR-windows-zip`.
2. Extract it.
3. Run `install.bat` as Administrator, or manually copy root-level `ZIKADARATOR.vst3` into `C:\Program Files\Common Files\VST3\`.
4. Optionally run the root-level standalone app `ZIKADARATOR.exe`.
5. Optionally compare the package contents with `SHA256SUMS.txt` after transfer.

## macOS

### Recommended: Installer package

1. Download `ZIKADARATOR-macos-installer`.
2. Check the included `BUILD-INFO.txt` for signing and notarization status.
3. For an unsigned tester build, in Finder **right-click** the `.pkg`, choose **Open**, and approve the warning.
4. Complete the installer.

Installed paths:

- VST3: `/Library/Audio/Plug-Ins/VST3/ZIKADARATOR.vst3`
- AU: `/Library/Audio/Plug-Ins/Components/ZIKADARATOR.component`
- Standalone: `/Applications/ZIKADARATOR.app`

### Manual: ZIP

1. Download `ZIKADARATOR-macos-zip`.
2. Extract it.
3. Copy the plugin bundles to the paths above.
4. Remove quarantine attributes:

```bash
xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/ZIKADARATOR.vst3
xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/Components/ZIKADARATOR.component
xattr -dr com.apple.quarantine /Applications/ZIKADARATOR.app
```

## After installing

1. Start your DAW.
2. Trigger a plugin rescan if needed.
3. Load ZIKADARATOR on an audio track.
4. If the build is a debug/testing build, collect logs before reporting issues.
