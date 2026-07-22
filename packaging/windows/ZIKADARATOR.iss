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
AppPublisherURL=https://github.com/cedric-albeke/zikadarator
AppSupportURL=https://github.com/cedric-albeke/zikadarator/issues
AppUpdatesURL=https://github.com/cedric-albeke/zikadarator/releases
AppComments=Sequence the signal.
DefaultDirName={autopf}\ZIKADARATOR
DefaultGroupName=ZIKADARATOR
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma
SolidCompression=yes
WizardStyle=modern dynamic
WizardBackColor=#06110F
WizardBackColorDynamicDark=#06110F
WizardImageFile={#SourcePath}\assets\wizard-large.bmp
WizardSmallImageFile={#SourcePath}\assets\wizard-small.bmp
SetupIconFile={#SourcePath}\assets\setup-icon.ico
OutputDir={#MyOutputDir}
OutputBaseFilename=ZIKADARATOR-Setup
DisableProgramGroupPage=yes
DisableWelcomePage=no
DisableReadyPage=no
UninstallDisplayIcon={app}\ZIKADARATOR.exe
VersionInfoCompany=Zikada
VersionInfoDescription=ZIKADARATOR VST3 and standalone installer
VersionInfoProductName=ZIKADARATOR
VersionInfoProductVersion={#MyAppVersion}
VersionInfoVersion={#MyAppVersion}

[Types]
Name: "full"; Description: "Full installation"
Name: "pluginonly"; Description: "Plugin only"

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full pluginonly; Flags: fixed
Name: "standalone"; Description: "Standalone app"; Types: full

[Files]
Source: "{#MySourceDir}\ZIKADARATOR.vst3\*"; DestDir: "{commoncf}\VST3\ZIKADARATOR.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#MySourceDir}\ZIKADARATOR.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
Source: "{#SourcePath}\assets\button-back.bmp"; Flags: dontcopy noencryption
Source: "{#SourcePath}\assets\button-next.bmp"; Flags: dontcopy noencryption
Source: "{#SourcePath}\assets\button-install.bmp"; Flags: dontcopy noencryption
Source: "{#SourcePath}\assets\button-finish.bmp"; Flags: dontcopy noencryption
Source: "{#SourcePath}\assets\button-cancel.bmp"; Flags: dontcopy noencryption
Source: "{#SourcePath}\assets\button-browse.bmp"; Flags: dontcopy noencryption

[Icons]
Name: "{autoprograms}\ZIKADARATOR"; Filename: "{app}\ZIKADARATOR.exe"; Components: standalone
Name: "{autodesktop}\ZIKADARATOR"; Filename: "{app}\ZIKADARATOR.exe"; Tasks: desktopicon; Components: standalone

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Components: standalone

[Run]
Filename: "{app}\ZIKADARATOR.exe"; Description: "Launch ZIKADARATOR standalone"; Flags: nowait postinstall skipifsilent; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\ZIKADARATOR.vst3"

[Code]
var
  SignalBlack, RackSurface, PanelSurface, PhosphorGreen: TColor;
  SliceCyan, LoopMagenta, FilterLime, TextStrong, TextMuted: TColor;
  SidebarPanel, HeaderPanel, BodyPanel, NavPanel: TPanel;
  WelcomePanel, OptionsPanel, ReviewPanel, TransferPanel, FinishedPanel: TPanel;
  PageTitle, PageDescription, TransferStatus, TransferFile: TNewStaticText;
  ReviewVST3, ReviewStandalone, ReviewDesktop, ReviewPath: TNewStaticText;
  StandaloneToggle, DesktopToggle, LaunchToggle: TNewCheckBox;
  InstallPathEdit: TNewEdit;
  BackButton, NextButton, CancelButton, BrowseButton: TBitmapButton;
  StageLabels: array [0..3] of TNewStaticText;
  StageMarks: array [0..3] of TPanel;
  ProgressCells: array [0..15] of TPanel;
  OptionsPage, ReviewPage: TWizardPage;

function NewPanel(Parent: TWinControl; Color: TColor): TPanel;
begin
  Result := TPanel.Create(WizardForm);
  Result.Parent := Parent;
  Result.Caption := '';
  Result.Color := Color;
  Result.ParentBackground := False;
  Result.BevelOuter := bvNone;
  Result.BevelInner := bvNone;
end;

function NewText(Parent: TWinControl; Caption: String; X, Y, W, H, Size: Integer;
  Color: TColor; Bold, Mono: Boolean): TNewStaticText;
begin
  Result := TNewStaticText.Create(WizardForm);
  Result.Parent := Parent;
  Result.AutoSize := False;
  Result.WordWrap := True;
  Result.Caption := Caption;
  Result.SetBounds(ScaleX(X), ScaleY(Y), ScaleX(W), ScaleY(H));
  Result.Color := TPanel(Parent).Color;
  Result.Font.Color := Color;
  Result.Font.Size := Size;
  if Bold then
    Result.Font.Style := [fsBold];
  if Mono then
    Result.Font.Name := 'Consolas'
  else
    Result.Font.Name := 'Segoe UI';
  Result.StyleElements := Result.StyleElements - [seFont, seClient];
end;

procedure AddLane(Name: String; LaneColor: TColor; Y: Integer);
var
  Lane, Accent: TPanel;
begin
  Lane := NewPanel(SidebarPanel, PanelSurface);
  Lane.SetBounds(ScaleX(18), ScaleY(Y), ScaleX(184), ScaleY(30));
  Accent := NewPanel(Lane, LaneColor);
  Accent.SetBounds(0, 0, ScaleX(3), Lane.Height);
  NewText(Lane, Name, 14, 6, 110, 18, 9, LaneColor, True, True);
  NewText(Lane, 'READY', 127, 6, 44, 18, 7, TextMuted, False, True).Alignment := taRightJustify;
end;

procedure LoadButtonBitmap(Button: TBitmapButton; FileName: String; W: Integer);
begin
  Button.Bitmap.LoadFromFile(ExpandConstant('{tmp}\' + FileName));
  Button.Stretch := True;
  Button.Center := True;
  Button.SetBounds(Button.Left, Button.Top, ScaleX(W), ScaleY(36));
end;

procedure NativeNext(Sender: TObject);
begin
  if WizardForm.CurPageID = wpFinished then begin
    if WizardForm.RunList.Items.Count > 0 then
      WizardForm.RunList.Checked[0] := LaunchToggle.Checked;
  end;
  WizardForm.NextButton.OnClick(WizardForm.NextButton);
end;

procedure NativeBack(Sender: TObject);
begin
  WizardForm.BackButton.OnClick(WizardForm.BackButton);
end;

procedure NativeCancel(Sender: TObject);
begin
  WizardForm.CancelButton.OnClick(WizardForm.CancelButton);
end;

procedure BrowseInstallPath(Sender: TObject);
var
  Directory: String;
begin
  Directory := InstallPathEdit.Text;
  if BrowseForFolder('Choose the standalone application folder', Directory, True) then
    InstallPathEdit.Text := Directory;
end;

procedure StandaloneChanged(Sender: TObject);
begin
  DesktopToggle.Enabled := StandaloneToggle.Checked;
  if not StandaloneToggle.Checked then
    DesktopToggle.Checked := False;
end;

procedure ApplySelections;
begin
  WizardForm.DirEdit.Text := InstallPathEdit.Text;
  if StandaloneToggle.Checked then
    WizardSelectComponents('vst3,standalone')
  else
    WizardSelectComponents('vst3');

  if StandaloneToggle.Checked and DesktopToggle.Checked then
    WizardSelectTasks('desktopicon')
  else
    WizardSelectTasks('');
end;

procedure UpdateReview;
var
  Value: String;
begin
  ApplySelections;
  ReviewVST3.Caption := 'VST3 PLUGIN'#13#10 +
    ExpandConstant('{commoncf}\VST3\ZIKADARATOR.vst3');

  if StandaloneToggle.Checked then
    ReviewStandalone.Caption := 'STANDALONE APP'#13#10 + InstallPathEdit.Text
  else
    ReviewStandalone.Caption := 'STANDALONE APP'#13#10'Not selected';

  if DesktopToggle.Checked and StandaloneToggle.Checked then
    Value := 'Create shortcut'
  else
    Value := 'No shortcut';
  ReviewDesktop.Caption := 'DESKTOP'#13#10 + Value;
  ReviewPath.Caption := 'BUILD CHANNEL'#13#10'Unsigned V1 tester build / Windows x64';
end;

procedure SetStage(ActiveStage: Integer);
var
  I: Integer;
begin
  for I := 0 to 3 do begin
    if I = ActiveStage then begin
      StageMarks[I].Color := PhosphorGreen;
      StageLabels[I].Font.Color := TextStrong;
      StageLabels[I].Font.Style := [fsBold];
    end else if I < ActiveStage then begin
      StageMarks[I].Color := SliceCyan;
      StageLabels[I].Font.Color := TextMuted;
      StageLabels[I].Font.Style := [];
    end else begin
      StageMarks[I].Color := PanelSurface;
      StageLabels[I].Font.Color := TextMuted;
      StageLabels[I].Font.Style := [];
    end;
  end;
end;

procedure PositionNativeButtons;
begin
  WizardForm.BackButton.Left := -ScaleX(400);
  WizardForm.NextButton.Left := -ScaleX(300);
  WizardForm.CancelButton.Left := -ScaleX(200);
  WizardForm.BackButton.TabStop := False;
  WizardForm.NextButton.TabStop := False;
  WizardForm.CancelButton.TabStop := False;
end;

procedure UpdateNavigation(PageID: Integer);
var
  NextWidth: Integer;
begin
  BackButton.Visible := (PageID = OptionsPage.ID) or (PageID = ReviewPage.ID);
  CancelButton.Visible := PageID <> wpFinished;
  NextButton.Visible := PageID <> wpInstalling;

  if PageID = ReviewPage.ID then begin
    LoadButtonBitmap(NextButton, 'button-install.bmp', 132);
    NextButton.Caption := 'Install ZIKADARATOR';
    NextWidth := 132;
  end else if PageID = wpFinished then begin
    LoadButtonBitmap(NextButton, 'button-finish.bmp', 112);
    NextButton.Caption := 'Finish setup';
    NextWidth := 112;
  end else begin
    LoadButtonBitmap(NextButton, 'button-next.bmp', 112);
    NextButton.Caption := 'Continue';
    NextWidth := 112;
  end;

  NextButton.Left := NavPanel.ClientWidth - ScaleX(24 + NextWidth);
  NextButton.Top := ScaleY(20);
  CancelButton.Left := NextButton.Left - CancelButton.Width - ScaleX(10);
  CancelButton.Top := NextButton.Top;
  BackButton.Left := ScaleX(24);
  BackButton.Top := NextButton.Top;
  PositionNativeButtons;
end;

procedure ShowPage(Panel: TPanel; Show: Boolean);
begin
  Panel.Visible := Show;
  if Show then
    Panel.BringToFront;
end;

procedure BuildSidebar;
var
  Logo: TBitmapImage;
  I: Integer;
  StageNames: array [0..3] of String;
begin
  Logo := TBitmapImage.Create(WizardForm);
  Logo.Parent := SidebarPanel;
  Logo.Bitmap := WizardForm.WizardSmallBitmapImage.Bitmap;
  Logo.Stretch := True;
  Logo.SetBounds(ScaleX(18), ScaleY(20), ScaleX(58), ScaleY(58));

  NewText(SidebarPanel, 'ZIKADARATOR', 86, 25, 120, 24, 13, TextStrong, True, False);
  NewText(SidebarPanel, 'SEQUENCE THE SIGNAL', 86, 52, 120, 16, 6, PhosphorGreen, False, True);

  AddLane('SLICE', SliceCyan, 104);
  AddLane('LOOP', LoopMagenta, 138);
  AddLane('ENVELOPE', StrToColor('#9258FF'), 172);
  AddLane('FX1', PhosphorGreen, 206);
  AddLane('FILTER', FilterLime, 240);
  AddLane('FX2', StrToColor('#21D8C3'), 274);

  NewText(SidebarPanel, 'DEPLOYMENT CHAIN', 18, 334, 184, 18, 7, TextMuted, True, True);
  StageNames[0] := '01  START';
  StageNames[1] := '02  TARGETS';
  StageNames[2] := '03  REVIEW';
  StageNames[3] := '04  TRANSFER';
  for I := 0 to 3 do begin
    StageMarks[I] := NewPanel(SidebarPanel, PanelSurface);
    StageMarks[I].SetBounds(ScaleX(18), ScaleY(362 + (I * 28)), ScaleX(4), ScaleY(18));
    StageLabels[I] := NewText(SidebarPanel, StageNames[I], 32, 360 + (I * 28), 160, 20, 8, TextMuted, False, True);
  end;

  NewText(SidebarPanel, 'V1 / WINDOWS X64 / TEST CHANNEL', 18, 486, 184, 18, 6, TextMuted, False, True);
end;

procedure BuildWelcomePage;
begin
  NewText(WelcomePanel, 'V1 TEST BUILD', 28, 22, 130, 18, 7, PhosphorGreen, True, True);
  NewText(WelcomePanel, 'SEQUENCE'#13#10'THE SIGNAL.', 28, 55, 450, 82, 25, TextStrong, True, False);
  NewText(WelcomePanel,
    'Deploy the ZIKADARATOR multi-FX sequencer as a 64-bit VST3 plugin and optional standalone instrument.',
    28, 151, 452, 52, 10, TextMuted, False, False);
  NewText(WelcomePanel, '50 FACTORY PATTERNS', 28, 229, 150, 20, 8, SliceCyan, True, True);
  NewText(WelcomePanel, '6 FX LANES', 192, 229, 100, 20, 8, LoopMagenta, True, True);
  NewText(WelcomePanel, '16 STEPS', 306, 229, 90, 20, 8, FilterLime, True, True);
  NewText(WelcomePanel,
    'This build is unsigned and intended for hands-on testing before the public V1 release.',
    28, 274, 452, 36, 8, TextMuted, False, True);
end;

procedure BuildOptionsPage;
var
  Card, Accent: TPanel;
begin
  Card := NewPanel(OptionsPanel, PanelSurface);
  Card.SetBounds(ScaleX(28), ScaleY(18), ScaleX(484), ScaleY(66));
  Accent := NewPanel(Card, SliceCyan);
  Accent.SetBounds(0, 0, ScaleX(4), Card.Height);
  NewText(Card, 'VST3 PLUGIN', 18, 10, 220, 20, 10, TextStrong, True, True);
  NewText(Card, 'Required  /  Common Files\VST3', 18, 35, 280, 18, 7, TextMuted, False, True);
  NewText(Card, 'ALWAYS ON', 360, 22, 96, 18, 7, SliceCyan, True, True).Alignment := taRightJustify;

  StandaloneToggle := TNewCheckBox.Create(WizardForm);
  StandaloneToggle.Parent := OptionsPanel;
  StandaloneToggle.SetBounds(ScaleX(30), ScaleY(103), ScaleX(300), ScaleY(24));
  StandaloneToggle.Caption := 'Install standalone application';
  StandaloneToggle.Checked := WizardIsComponentSelected('standalone');
  StandaloneToggle.Color := RackSurface;
  StandaloneToggle.Font.Name := 'Segoe UI';
  StandaloneToggle.Font.Size := 10;
  StandaloneToggle.Font.Color := TextStrong;
  StandaloneToggle.StyleElements := StandaloneToggle.StyleElements - [seFont, seClient];
  StandaloneToggle.OnClick := @StandaloneChanged;

  DesktopToggle := TNewCheckBox.Create(WizardForm);
  DesktopToggle.Parent := OptionsPanel;
  DesktopToggle.SetBounds(ScaleX(52), ScaleY(135), ScaleX(300), ScaleY(22));
  DesktopToggle.Caption := 'Create desktop shortcut';
  DesktopToggle.Checked := WizardIsTaskSelected('desktopicon');
  DesktopToggle.Enabled := StandaloneToggle.Checked;
  DesktopToggle.Color := RackSurface;
  DesktopToggle.Font.Name := 'Segoe UI';
  DesktopToggle.Font.Size := 9;
  DesktopToggle.Font.Color := TextMuted;
  DesktopToggle.StyleElements := DesktopToggle.StyleElements - [seFont, seClient];

  NewText(OptionsPanel, 'STANDALONE LOCATION', 28, 184, 300, 18, 7, TextMuted, True, True);
  InstallPathEdit := TNewEdit.Create(WizardForm);
  InstallPathEdit.Parent := OptionsPanel;
  InstallPathEdit.SetBounds(ScaleX(28), ScaleY(208), ScaleX(368), ScaleY(30));
  InstallPathEdit.Text := WizardForm.DirEdit.Text;
  InstallPathEdit.Color := SignalBlack;
  InstallPathEdit.Font.Name := 'Consolas';
  InstallPathEdit.Font.Size := 8;
  InstallPathEdit.Font.Color := TextStrong;
  InstallPathEdit.StyleElements := InstallPathEdit.StyleElements - [seFont, seClient];

  BrowseButton := TBitmapButton.Create(WizardForm);
  BrowseButton.Parent := OptionsPanel;
  BrowseButton.Left := ScaleX(410);
  BrowseButton.Top := ScaleY(205);
  BrowseButton.Caption := 'Browse for standalone folder';
  BrowseButton.Hint := BrowseButton.Caption;
  BrowseButton.ShowHint := True;
  BrowseButton.OnClick := @BrowseInstallPath;
  LoadButtonBitmap(BrowseButton, 'button-browse.bmp', 92);

  NewText(OptionsPanel,
    'The plugin is always installed system-wide. The standalone path only applies when the optional app is selected.',
    28, 258, 472, 38, 8, TextMuted, False, False);
end;

procedure BuildReviewPage;
begin
  NewText(ReviewPanel, 'SIGNAL ROUTE', 28, 18, 180, 18, 7, PhosphorGreen, True, True);
  ReviewVST3 := NewText(ReviewPanel, '', 28, 50, 470, 48, 8, TextStrong, True, True);
  ReviewStandalone := NewText(ReviewPanel, '', 28, 106, 470, 48, 8, TextStrong, True, True);
  ReviewDesktop := NewText(ReviewPanel, '', 28, 162, 220, 48, 8, TextStrong, True, True);
  ReviewPath := NewText(ReviewPanel, '', 270, 162, 228, 48, 8, TextStrong, True, True);
  NewText(ReviewPanel,
    'INSTALL writes the selected binaries and replaces an existing ZIKADARATOR VST3 bundle if present.',
    28, 245, 470, 38, 8, TextMuted, False, False);
end;

procedure BuildTransferPage;
var
  I: Integer;
begin
  TransferStatus := NewText(TransferPanel, 'PREPARING SIGNAL CHAIN', 28, 30, 470, 28, 10, PhosphorGreen, True, True);
  TransferFile := NewText(TransferPanel, '', 28, 68, 470, 28, 8, TextMuted, False, True);
  NewText(TransferPanel, '16-STEP DEPLOYMENT SEQUENCE', 28, 128, 470, 18, 7, TextMuted, True, True);
  for I := 0 to 15 do begin
    ProgressCells[I] := NewPanel(TransferPanel, PanelSurface);
    ProgressCells[I].SetBounds(ScaleX(28 + (I * 29)), ScaleY(158), ScaleX(22), ScaleY(54));
    NewText(ProgressCells[I], IntToStr(I + 1), 0, 18, 22, 16, 6, TextMuted, False, True).Alignment := taCenter;
  end;
  NewText(TransferPanel,
    'Keep this window open while the plugin bundle and standalone binary are transferred.',
    28, 248, 470, 38, 8, TextMuted, False, False);
end;

procedure BuildFinishedPage;
begin
  NewText(FinishedPanel, 'TRANSFER COMPLETE', 28, 24, 260, 18, 7, PhosphorGreen, True, True);
  NewText(FinishedPanel, 'SIGNAL'#13#10'ROUTED.', 28, 57, 450, 82, 25, TextStrong, True, False);
  NewText(FinishedPanel,
    'ZIKADARATOR is installed. Rescan VST3 plugins in your DAW before loading the first pattern.',
    28, 151, 460, 48, 10, TextMuted, False, False);

  LaunchToggle := TNewCheckBox.Create(WizardForm);
  LaunchToggle.Parent := FinishedPanel;
  LaunchToggle.SetBounds(ScaleX(28), ScaleY(229), ScaleX(360), ScaleY(24));
  LaunchToggle.Caption := 'Launch the standalone application';
  LaunchToggle.Checked := True;
  LaunchToggle.Color := RackSurface;
  LaunchToggle.Font.Name := 'Segoe UI';
  LaunchToggle.Font.Size := 10;
  LaunchToggle.Font.Color := TextStrong;
  LaunchToggle.StyleElements := LaunchToggle.StyleElements - [seFont, seClient];
  NewText(FinishedPanel, 'VST3  /  STANDALONE  /  50 FACTORY PATTERNS', 28, 278, 460, 20, 7, SliceCyan, True, True);
end;

procedure InitializeWizard;
begin
  SignalBlack := StrToColor('#06110F');
  RackSurface := StrToColor('#0A1E19');
  PanelSurface := StrToColor('#0D2A23');
  PhosphorGreen := StrToColor('#00F5A0');
  SliceCyan := StrToColor('#16D9F4');
  LoopMagenta := StrToColor('#D63BC6');
  FilterLime := StrToColor('#B8FF00');
  TextStrong := StrToColor('#F3FFF9');
  TextMuted := StrToColor('#7EA89A');

  ExtractTemporaryFile('button-back.bmp');
  ExtractTemporaryFile('button-next.bmp');
  ExtractTemporaryFile('button-install.bmp');
  ExtractTemporaryFile('button-finish.bmp');
  ExtractTemporaryFile('button-cancel.bmp');
  ExtractTemporaryFile('button-browse.bmp');

  OptionsPage := CreateCustomPage(wpWelcome, 'Targets', 'Choose the signal destinations');
  ReviewPage := CreateCustomPage(OptionsPage.ID, 'Review', 'Confirm the deployment chain');

  WizardForm.Caption := 'ZIKADARATOR / SIGNAL DEPLOYMENT';
  WizardForm.ClientWidth := ScaleX(760);
  WizardForm.ClientHeight := ScaleY(520);
  WizardForm.Color := SignalBlack;
  WizardForm.Font.Name := 'Segoe UI';
  WizardForm.Font.Color := TextStrong;

  SidebarPanel := NewPanel(WizardForm, SignalBlack);
  SidebarPanel.SetBounds(0, 0, ScaleX(220), WizardForm.ClientHeight);
  HeaderPanel := NewPanel(WizardForm, RackSurface);
  HeaderPanel.SetBounds(ScaleX(220), 0, ScaleX(540), ScaleY(110));
  BodyPanel := NewPanel(WizardForm, RackSurface);
  BodyPanel.SetBounds(ScaleX(220), ScaleY(110), ScaleX(540), ScaleY(330));
  NavPanel := NewPanel(WizardForm, SignalBlack);
  NavPanel.SetBounds(ScaleX(220), ScaleY(440), ScaleX(540), ScaleY(80));

  NewText(HeaderPanel, 'ZIKADA DEPLOYMENT CONSOLE', 28, 18, 300, 18, 7, PhosphorGreen, True, True);
  PageTitle := NewText(HeaderPanel, '', 28, 42, 470, 30, 17, TextStrong, True, False);
  PageDescription := NewText(HeaderPanel, '', 28, 75, 470, 22, 8, TextMuted, False, False);

  WelcomePanel := NewPanel(BodyPanel, RackSurface);
  WelcomePanel.Align := alClient;
  OptionsPanel := NewPanel(BodyPanel, RackSurface);
  OptionsPanel.Align := alClient;
  ReviewPanel := NewPanel(BodyPanel, RackSurface);
  ReviewPanel.Align := alClient;
  TransferPanel := NewPanel(BodyPanel, RackSurface);
  TransferPanel.Align := alClient;
  FinishedPanel := NewPanel(BodyPanel, RackSurface);
  FinishedPanel.Align := alClient;

  BuildSidebar;
  BuildWelcomePage;
  BuildOptionsPage;
  BuildReviewPage;
  BuildTransferPage;
  BuildFinishedPage;

  BackButton := TBitmapButton.Create(WizardForm);
  BackButton.Parent := NavPanel;
  BackButton.Caption := 'Back';
  BackButton.Hint := 'Return to the previous step';
  BackButton.ShowHint := True;
  BackButton.OnClick := @NativeBack;
  LoadButtonBitmap(BackButton, 'button-back.bmp', 92);

  CancelButton := TBitmapButton.Create(WizardForm);
  CancelButton.Parent := NavPanel;
  CancelButton.Caption := 'Cancel setup';
  CancelButton.Hint := CancelButton.Caption;
  CancelButton.ShowHint := True;
  CancelButton.OnClick := @NativeCancel;
  LoadButtonBitmap(CancelButton, 'button-cancel.bmp', 92);

  NextButton := TBitmapButton.Create(WizardForm);
  NextButton.Parent := NavPanel;
  NextButton.Caption := 'Continue';
  NextButton.Hint := NextButton.Caption;
  NextButton.ShowHint := True;
  NextButton.OnClick := @NativeNext;
  LoadButtonBitmap(NextButton, 'button-next.bmp', 112);

  WizardForm.OuterNotebook.Visible := False;
  WizardForm.MainPanel.Visible := False;
  WizardForm.WizardBitmapImage.Visible := False;
  WizardForm.WizardBitmapImage2.Visible := False;
  WizardForm.WizardSmallBitmapImage.Visible := False;
  WizardForm.BeveledLabel.Visible := False;
  WizardForm.Bevel.Visible := False;
  WizardForm.Bevel1.Visible := False;
  PositionNativeButtons;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = OptionsPage.ID then begin
    if InstallPathEdit.Text = '' then begin
      MsgBox('Choose a standalone application folder before continuing.', mbError, MB_OK);
      Result := False;
    end else begin
      ApplySelections;
      UpdateReview;
    end;
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = wpSelectDir) or
    (PageID = wpSelectComponents) or
    (PageID = wpSelectTasks) or
    (PageID = wpReady);
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  WizardForm.OuterNotebook.Visible := False;
  ShowPage(WelcomePanel, CurPageID = wpWelcome);
  ShowPage(OptionsPanel, CurPageID = OptionsPage.ID);
  ShowPage(ReviewPanel, CurPageID = ReviewPage.ID);
  ShowPage(TransferPanel, CurPageID = wpInstalling);
  ShowPage(FinishedPanel, CurPageID = wpFinished);

  if CurPageID = wpWelcome then begin
    PageTitle.Caption := 'Signal deployment';
    PageDescription.Caption := 'Install the sequencer-driven multi-FX engine.';
    SetStage(0);
  end else if CurPageID = OptionsPage.ID then begin
    PageTitle.Caption := 'Choose destinations';
    PageDescription.Caption := 'VST3 is required. Standalone and desktop shortcut are optional.';
    SetStage(1);
  end else if CurPageID = ReviewPage.ID then begin
    UpdateReview;
    PageTitle.Caption := 'Confirm the route';
    PageDescription.Caption := 'Review every destination before files are written.';
    SetStage(2);
  end else if CurPageID = wpInstalling then begin
    PageTitle.Caption := 'Transferring signal';
    PageDescription.Caption := 'Writing the plugin and application binaries.';
    SetStage(3);
  end else if CurPageID = wpFinished then begin
    PageTitle.Caption := 'Deployment complete';
    PageDescription.Caption := 'ZIKADARATOR is ready for the next DAW scan.';
    SetStage(3);
  end;

  UpdateNavigation(CurPageID);
end;

procedure CurInstallProgressChanged(CurProgress, MaxProgress: Integer);
var
  I, ActiveCells, Percent: Integer;
begin
  if MaxProgress > 0 then begin
    ActiveCells := (CurProgress * 16) div MaxProgress;
    Percent := (CurProgress * 100) div MaxProgress;
  end else begin
    ActiveCells := 0;
    Percent := 0;
  end;

  for I := 0 to 15 do begin
    if I < ActiveCells then begin
      case I mod 6 of
        0: ProgressCells[I].Color := SliceCyan;
        1: ProgressCells[I].Color := LoopMagenta;
        2: ProgressCells[I].Color := StrToColor('#9258FF');
        3: ProgressCells[I].Color := PhosphorGreen;
        4: ProgressCells[I].Color := FilterLime;
        5: ProgressCells[I].Color := StrToColor('#21D8C3');
      end;
    end else
      ProgressCells[I].Color := PanelSurface;
  end;

  TransferStatus.Caption := 'WRITING SIGNAL CHAIN  /  ' + IntToStr(Percent) + '%';
  TransferFile.Caption := ExtractFileName(CurrentFilename);
end;
