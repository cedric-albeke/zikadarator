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
WizardStyle=modern
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
Source: "{#SourcePath}\assets\brand-logo.bmp"; Flags: dontcopy noencryption

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
  PageTitle, PageDescription, TransferStatus, TransferFile: TLabel;
  ReviewVST3, ReviewStandalone, ReviewDesktop, ReviewPath: TLabel;
  StandaloneToggle, DesktopToggle, LaunchToggle: TNewCheckBox;
  InstallPathEdit: TNewEdit;
  BackButton, NextButton, CancelButton, BrowseButton: TBitmapButton;
  StageLabels: array [0..3] of TLabel;
  StageMarks: array [0..3] of TPanel;
  ProgressCells: array [0..15] of TPanel;
  OptionsPage, ReviewPage: TWizardPage;

function NewPanel(Parent: TWinControl; Color: TColor): TPanel;
begin
  Result := TPanel.Create(WizardForm);
  Result.Parent := Parent;
  Result.Caption := '';
  Result.StyleElements := Result.StyleElements - [seFont, seClient, seBorder];
  Result.Color := Color;
  Result.ParentBackground := False;
  Result.BevelOuter := bvNone;
  Result.BevelInner := bvNone;
end;

function NewText(Parent: TWinControl; Caption: String; X, Y, W, H, Size: Integer;
  Color: TColor; Bold, Mono: Boolean): TLabel;
begin
  Result := TLabel.Create(WizardForm);
  Result.Parent := Parent;
  Result.AutoSize := False;
  Result.WordWrap := True;
  Result.Transparent := True;
  Result.Caption := Caption;
  Result.SetBounds(ScaleX(X), ScaleY(Y), ScaleX(W), ScaleY(H));
  Result.Font.Color := Color;
  Result.Font.Size := Size;
  if Bold then
    Result.Font.Style := [fsBold];
  if Mono then
    Result.Font.Name := 'Consolas'
  else
    Result.Font.Name := 'Segoe UI';
end;

function NewDisplay(Parent: TWinControl; AccentColor: TColor;
  X, Y, W, H: Integer): TPanel;
var
  Frame, TopRail, BottomRail: TPanel;
begin
  Frame := NewPanel(Parent, AccentColor);
  Frame.SetBounds(ScaleX(X), ScaleY(Y), ScaleX(W), ScaleY(H));
  Result := NewPanel(Frame, SignalBlack);
  Result.SetBounds(ScaleX(2), ScaleY(2), Frame.Width - ScaleX(4), Frame.Height - ScaleY(4));
  TopRail := NewPanel(Result, PanelSurface);
  TopRail.SetBounds(0, 0, Result.ClientWidth, ScaleY(4));
  BottomRail := NewPanel(Result, RackSurface);
  BottomRail.SetBounds(0, Result.ClientHeight - ScaleY(7), Result.ClientWidth, ScaleY(7));
end;

procedure AddLane(Name: String; LaneColor: TColor; Y: Integer);
var
  Lane, Accent: TPanel;
begin
  Lane := NewPanel(SidebarPanel, PanelSurface);
  Lane.SetBounds(ScaleX(14), ScaleY(Y), ScaleX(168), ScaleY(27));
  Accent := NewPanel(Lane, LaneColor);
  Accent.SetBounds(0, 0, ScaleX(3), Lane.Height);
  NewText(Lane, Name, 13, 5, 100, 17, 8, LaneColor, True, True);
  NewText(Lane, 'ONLINE', 111, 5, 43, 17, 6, TextMuted, False, True).Alignment := taRightJustify;
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
  Logo.Bitmap.LoadFromFile(ExpandConstant('{tmp}\brand-logo.bmp'));
  Logo.Stretch := True;
  Logo.SetBounds(ScaleX(14), ScaleY(14), ScaleX(48), ScaleY(48));

  NewText(SidebarPanel, 'ZIKADARATOR', 72, 16, 110, 22, 11, TextStrong, True, False);
  NewText(SidebarPanel, 'SEQUENCE THE SIGNAL', 72, 40, 110, 14, 6, PhosphorGreen, False, True);

  AddLane('SLICE', SliceCyan, 78);
  AddLane('LOOP', LoopMagenta, 108);
  AddLane('ENVELOPE', StrToColor('#9258FF'), 138);
  AddLane('FX1', PhosphorGreen, 168);
  AddLane('FILTER', FilterLime, 198);
  AddLane('FX2', StrToColor('#21D8C3'), 228);

  NewText(SidebarPanel, 'SIGNAL ROUTE', 14, 278, 168, 16, 7, TextMuted, True, True);
  StageNames[0] := '01  START';
  StageNames[1] := '02  TARGETS';
  StageNames[2] := '03  REVIEW';
  StageNames[3] := '04  TRANSFER';
  for I := 0 to 3 do begin
    StageMarks[I] := NewPanel(SidebarPanel, PanelSurface);
    StageMarks[I].SetBounds(ScaleX(14), ScaleY(302 + (I * 24)), ScaleX(3), ScaleY(16));
    StageLabels[I] := NewText(SidebarPanel, StageNames[I], 26, 300 + (I * 24), 150, 18, 7, TextMuted, False, True);
  end;

  NewText(SidebarPanel, 'V1  /  WINDOWS X64  /  TEST', 14, 432, 168, 16, 6, TextMuted, False, True);
end;

procedure BuildWelcomePage;
var
  Display: TPanel;
begin
  Display := NewDisplay(WelcomePanel, PhosphorGreen, 22, 14, 480, 236);
  NewText(Display, 'INPUT  //  V1 TEST BUILD', 18, 14, 210, 16, 7, PhosphorGreen, True, True);
  NewText(Display, 'SEQUENCE THE SIGNAL.', 18, 43, 438, 38, 20, TextStrong, True, False);
  NewText(Display,
    'Deploy the ZIKADARATOR multi-FX sequencer as a 64-bit VST3 plugin and optional standalone instrument.',
    18, 86, 438, 42, 9, TextMuted, False, False);
  NewText(Display, '50 PATTERNS', 18, 148, 116, 18, 7, SliceCyan, True, True);
  NewText(Display, '6 FX LANES', 156, 148, 104, 18, 7, LoopMagenta, True, True);
  NewText(Display, '16 STEPS', 282, 148, 90, 18, 7, FilterLime, True, True);
  NewText(Display, 'STATUS  READY FOR ROUTING', 18, 195, 300, 18, 7, TextStrong, False, True);
  NewText(WelcomePanel,
    'Unsigned tester build. Windows may request confirmation before setup starts.',
    24, 268, 476, 24, 7, TextMuted, False, False);
end;

procedure BuildOptionsPage;
var
  Display, Card, Accent: TPanel;
begin
  Display := NewDisplay(OptionsPanel, SliceCyan, 22, 14, 480, 272);
  Card := NewPanel(Display, PanelSurface);
  Card.SetBounds(ScaleX(16), ScaleY(14), ScaleX(444), ScaleY(54));
  Accent := NewPanel(Card, SliceCyan);
  Accent.SetBounds(0, 0, ScaleX(4), Card.Height);
  NewText(Card, 'VST3 PLUGIN', 16, 8, 200, 18, 9, TextStrong, True, True);
  NewText(Card, 'Required  /  Common Files\VST3', 16, 29, 280, 16, 7, TextMuted, False, True);
  NewText(Card, 'ALWAYS ON', 334, 18, 86, 16, 7, SliceCyan, True, True).Alignment := taRightJustify;

  StandaloneToggle := TNewCheckBox.Create(WizardForm);
  StandaloneToggle.Parent := Display;
  StandaloneToggle.SetBounds(ScaleX(18), ScaleY(82), ScaleX(300), ScaleY(22));
  StandaloneToggle.Caption := 'Install standalone application';
  StandaloneToggle.Checked := WizardIsComponentSelected('standalone');
  StandaloneToggle.StyleElements := StandaloneToggle.StyleElements - [seFont, seClient];
  StandaloneToggle.Color := SignalBlack;
  StandaloneToggle.Font.Name := 'Segoe UI';
  StandaloneToggle.Font.Size := 9;
  StandaloneToggle.Font.Color := TextStrong;
  StandaloneToggle.OnClick := @StandaloneChanged;

  DesktopToggle := TNewCheckBox.Create(WizardForm);
  DesktopToggle.Parent := Display;
  DesktopToggle.SetBounds(ScaleX(40), ScaleY(110), ScaleX(300), ScaleY(20));
  DesktopToggle.Caption := 'Create desktop shortcut';
  DesktopToggle.Checked := WizardIsTaskSelected('desktopicon');
  DesktopToggle.Enabled := StandaloneToggle.Checked;
  DesktopToggle.StyleElements := DesktopToggle.StyleElements - [seFont, seClient];
  DesktopToggle.Color := SignalBlack;
  DesktopToggle.Font.Name := 'Segoe UI';
  DesktopToggle.Font.Size := 8;
  DesktopToggle.Font.Color := TextMuted;

  NewText(Display, 'STANDALONE LOCATION', 16, 147, 300, 16, 7, TextMuted, True, True);
  InstallPathEdit := TNewEdit.Create(WizardForm);
  InstallPathEdit.Parent := Display;
  InstallPathEdit.SetBounds(ScaleX(16), ScaleY(168), ScaleX(334), ScaleY(28));
  InstallPathEdit.Text := WizardForm.DirEdit.Text;
  InstallPathEdit.StyleElements := InstallPathEdit.StyleElements - [seFont, seClient];
  InstallPathEdit.Color := SignalBlack;
  InstallPathEdit.Font.Name := 'Consolas';
  InstallPathEdit.Font.Size := 8;
  InstallPathEdit.Font.Color := TextStrong;

  BrowseButton := TBitmapButton.Create(WizardForm);
  BrowseButton.Parent := Display;
  BrowseButton.Left := ScaleX(364);
  BrowseButton.Top := ScaleY(164);
  BrowseButton.Caption := 'Browse for standalone folder';
  BrowseButton.Hint := BrowseButton.Caption;
  BrowseButton.ShowHint := True;
  BrowseButton.OnClick := @BrowseInstallPath;
  LoadButtonBitmap(BrowseButton, 'button-browse.bmp', 92);

  NewText(Display,
    'The plugin is always installed system-wide. The standalone path only applies when the optional app is selected.',
    16, 218, 444, 32, 7, TextMuted, False, False);
end;

procedure BuildReviewPage;
var
  Display: TPanel;
begin
  Display := NewDisplay(ReviewPanel, LoopMagenta, 22, 14, 480, 272);
  NewText(Display, 'SIGNAL ROUTE  //  CONFIRM', 16, 14, 260, 16, 7, LoopMagenta, True, True);
  ReviewVST3 := NewText(Display, '', 16, 43, 444, 40, 8, TextStrong, True, True);
  ReviewStandalone := NewText(Display, '', 16, 88, 444, 40, 8, TextStrong, True, True);
  ReviewDesktop := NewText(Display, '', 16, 137, 210, 40, 8, TextStrong, True, True);
  ReviewPath := NewText(Display, '', 244, 137, 216, 40, 8, TextStrong, True, True);
  NewText(Display,
    'INSTALL writes the selected binaries and replaces an existing ZIKADARATOR VST3 bundle if present.',
    16, 208, 444, 34, 7, TextMuted, False, False);
end;

procedure BuildTransferPage;
var
  Display: TPanel;
  I: Integer;
begin
  Display := NewDisplay(TransferPanel, PhosphorGreen, 22, 14, 480, 272);
  TransferStatus := NewText(Display, 'PREPARING SIGNAL CHAIN', 16, 18, 444, 24, 10, PhosphorGreen, True, True);
  TransferFile := NewText(Display, '', 16, 51, 444, 24, 7, TextMuted, False, True);
  NewText(Display, '16-STEP DEPLOYMENT SEQUENCE', 16, 102, 444, 16, 7, TextMuted, True, True);
  for I := 0 to 15 do begin
    ProgressCells[I] := NewPanel(Display, PanelSurface);
    ProgressCells[I].SetBounds(ScaleX(16 + (I * 27)), ScaleY(130), ScaleX(20), ScaleY(48));
    NewText(ProgressCells[I], IntToStr(I + 1), 0, 16, 20, 14, 6, TextMuted, False, True).Alignment := taCenter;
  end;
  NewText(Display,
    'Keep this window open while the plugin bundle and standalone binary are transferred.',
    16, 208, 444, 32, 7, TextMuted, False, False);
end;

procedure BuildFinishedPage;
var
  Display: TPanel;
begin
  Display := NewDisplay(FinishedPanel, PhosphorGreen, 22, 14, 480, 272);
  NewText(Display, 'OUTPUT  //  TRANSFER COMPLETE', 16, 16, 300, 16, 7, PhosphorGreen, True, True);
  NewText(Display, 'SIGNAL ROUTED.', 16, 46, 438, 38, 20, TextStrong, True, False);
  NewText(Display,
    'ZIKADARATOR is installed. Rescan VST3 plugins in your DAW before loading the first pattern.',
    16, 92, 438, 42, 9, TextMuted, False, False);

  LaunchToggle := TNewCheckBox.Create(WizardForm);
  LaunchToggle.Parent := Display;
  LaunchToggle.SetBounds(ScaleX(18), ScaleY(158), ScaleX(360), ScaleY(22));
  LaunchToggle.Caption := 'Launch the standalone application';
  LaunchToggle.Checked := True;
  LaunchToggle.StyleElements := LaunchToggle.StyleElements - [seFont, seClient];
  LaunchToggle.Color := SignalBlack;
  LaunchToggle.Font.Name := 'Segoe UI';
  LaunchToggle.Font.Size := 9;
  LaunchToggle.Font.Color := TextStrong;
  NewText(Display, 'VST3  /  STANDALONE  /  50 FACTORY PATTERNS', 16, 214, 438, 18, 7, SliceCyan, True, True);
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
  ExtractTemporaryFile('brand-logo.bmp');

  OptionsPage := CreateCustomPage(wpWelcome, 'Targets', 'Choose the signal destinations');
  ReviewPage := CreateCustomPage(OptionsPage.ID, 'Review', 'Confirm the deployment chain');

  WizardForm.Caption := 'ZIKADARATOR / SIGNAL DEPLOYMENT';
  WizardForm.ClientWidth := ScaleX(720);
  WizardForm.ClientHeight := ScaleY(460);
  WizardForm.Color := SignalBlack;
  WizardForm.Font.Name := 'Segoe UI';
  WizardForm.Font.Color := TextStrong;

  SidebarPanel := NewPanel(WizardForm, SignalBlack);
  SidebarPanel.SetBounds(0, 0, ScaleX(196), WizardForm.ClientHeight);
  HeaderPanel := NewPanel(WizardForm, RackSurface);
  HeaderPanel.SetBounds(ScaleX(196), 0, ScaleX(524), ScaleY(84));
  BodyPanel := NewPanel(WizardForm, RackSurface);
  BodyPanel.SetBounds(ScaleX(196), ScaleY(84), ScaleX(524), ScaleY(312));
  NavPanel := NewPanel(WizardForm, SignalBlack);
  NavPanel.SetBounds(ScaleX(196), ScaleY(396), ScaleX(524), ScaleY(64));

  NewText(HeaderPanel, 'ZIKADA DEPLOYMENT CONSOLE', 22, 10, 300, 16, 6, PhosphorGreen, True, True);
  PageTitle := NewText(HeaderPanel, '', 22, 29, 478, 25, 15, TextStrong, True, False);
  PageDescription := NewText(HeaderPanel, '', 22, 57, 478, 18, 7, TextMuted, False, False);

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
