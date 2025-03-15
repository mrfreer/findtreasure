#define MyAppName "Find Treasure"
#define MyAppVersion "1.1"
#define MyAppPublisher "Your Name"
#define MyAppURL "https://github.com/mrfreer/findtreasure"
#define MyAppExeName "SideScroller.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
AppId={{A1B2C3D4-E5F6-4A5B-8C7D-9E0F1A2B3C4D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=installer
OutputBaseFilename=FindTreasureSetup
Compression=lzma
SolidCompression=yes
; SetupIconFile=assets\player1.png  ; Commented out - PNG files can't be used as icons
; Require admin rights to install to Program Files
PrivilegesRequired=admin
; Add a license page
LicenseFile=LICENSE.txt
; Add a welcome image
; WizardImageFile=assets\player1.png  ; Commented out - PNG files can't be used here
; WizardSmallImageFile=assets\player1.png  ; Commented out - PNG files can't be used here

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Add the game executable
Source: "build\Debug\SideScroller.exe"; DestDir: "{app}"; Flags: ignoreversion

; Add the assets folder
Source: "assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs

; Add the bat animation frames specifically
Source: "assets\02-Fly\*"; DestDir: "{app}\assets\02-Fly"; Flags: ignoreversion recursesubdirs createallsubdirs

; Add documentation
Source: "INSTALL.md"; DestDir: "{app}"; Flags: ignoreversion; DestName: "README.txt"
Source: "LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion

; Add any required DLLs from Raylib
Source: "build\Debug\*.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

; Add any additional files or dependencies
; If you have any DLLs, add them here
; Source: "build\Debug\*.dll"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppName}}"; Filename: "{#MyAppURL}"
Name: "{group}\README"; Filename: "{app}\README.txt"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent 