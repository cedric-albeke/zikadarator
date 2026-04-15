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

### Windows

- Installer runs successfully
- VST3 lands in `C:\Program Files\Common Files\VST3`
- Standalone launches
- Plugin scans in at least one DAW

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
