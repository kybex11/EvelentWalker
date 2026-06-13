; EvelentWalker installer (Inno Setup 6)
; -----------------------------------------------------------------------------
; Installs every packed EvelentWalker tool (single-file exes built by
; build-release.ps1) into Program Files, creates Start Menu shortcuts for each
; tool, an optional desktop icon, and a full uninstaller / Add-Remove entry.
;
; Build with:
;     ISCC.exe build\installer\EvelentWalker.iss
; or via:
;     powershell -ExecutionPolicy Bypass -File build\build-installer.ps1

#define AppName "EvelentWalker"
#define AppVersion "1.0.0"
#define AppPublisher "EvelentWalker"
#define AppExe "EvelentWalker.exe"

; Folder that holds the assembled binaries (output of build-release.ps1).
#define DistDir "..\dist"

[Setup]
AppId={{8F3D5C2A-1E4B-4C77-9A2E-EVELENTWALKER01}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExe}
OutputDir=output
OutputBaseFilename=EvelentWalker-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Ship the whole assembled distribution (exes, configs, Shaders, icons, data).
Source: "{#DistDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
; Start Menu shortcuts for each tool.
Name: "{group}\EvelentWalker";              Filename: "{app}\EvelentWalker.exe"
Name: "{group}\RPF Explorer";               Filename: "{app}\EvelentWalker RPF Explorer.exe"
Name: "{group}\Vehicle Viewer";             Filename: "{app}\EvelentWalker Vehicle Viewer.exe"
Name: "{group}\Ped Viewer";                 Filename: "{app}\EvelentWalker Ped Viewer.exe"
Name: "{group}\Gen9 Converter";             Filename: "{app}\EvelentWalker Gen9 Converter.exe"
Name: "{group}\Mod Manager";                Filename: "{app}\EvelentWalker Mod Manager.exe"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
; Optional desktop icon for the main app.
Name: "{autodesktop}\EvelentWalker";        Filename: "{app}\EvelentWalker.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,EvelentWalker}"; Flags: nowait postinstall skipifsilent
