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
3. Copy `VST3/ZIKADARATOR.vst3` into `C:\Program Files\Common Files\VST3\`.
4. Optionally run the standalone app from the extracted `Standalone` folder.

## macOS

### Recommended: Unsigned installer package

1. Download `ZIKADARATOR-macos-installer`.
2. In Finder, **right-click** the `.pkg` and choose **Open**.
3. Approve the warning dialog.
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
