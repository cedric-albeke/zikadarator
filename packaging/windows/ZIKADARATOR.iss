#ifndef MyAppName
  #define MyAppName "ZIKADARATOR"
#endif

#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif

#ifndef MySourceDir
  #error MySourceDir must be provided to the installer build
#endif

#ifndef MyOutputDir
  #error MyOutputDir must be provided to the installer build
#endif

[Setup]
AppId={{2C333E6D-28C0-48DE-A4B8-03679C7A6D5E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=Zikada
DefaultDirName={autopf}\ZIKADARATOR
DefaultGroupName=ZIKADARATOR
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma
SolidCompression=yes
WizardStyle=modern
OutputDir={#MyOutputDir}
OutputBaseFilename=ZIKADARATOR-Setup
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\ZIKADARATOR.exe

[Types]
Name: "full"; Description: "Full installation"
Name: "pluginonly"; Description: "Plugin only"

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full pluginonly; Flags: fixed
Name: "standalone"; Description: "Standalone app"; Types: full

[Files]
Source: "{#MySourceDir}\VST3\ZIKADARATOR.vst3\*"; DestDir: "{commoncf}\VST3\ZIKADARATOR.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#MySourceDir}\Standalone\ZIKADARATOR.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\ZIKADARATOR"; Filename: "{app}\ZIKADARATOR.exe"; Components: standalone
Name: "{autodesktop}\ZIKADARATOR"; Filename: "{app}\ZIKADARATOR.exe"; Tasks: desktopicon; Components: standalone

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Components: standalone

[Run]
Filename: "{app}\ZIKADARATOR.exe"; Description: "Launch ZIKADARATOR standalone"; Flags: nowait postinstall skipifsilent; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\ZIKADARATOR.vst3"
