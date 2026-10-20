; Installateur Windows de Blocs (Inno Setup 6.3+).
;
; Utilisation (depuis la racine du projet) :
;   1) cmake --preset mingw-release
;   2) cmake --build --preset mingw-release
;   3) cmake --install build/mingw-release --prefix dist/Blocs      (copie aussi les DLL Qt)
;   4) iscc installer\Blocs.iss                               (ou : iscc /DMyAppVersion=1.2.3 installer\Blocs.iss)
; Résultat : dist\installer\Blocs-<version>-Setup.exe
;
; Alternative à l'étape 3 : scripts\deploy.bat "chemin\vers\Blocs.exe" (produit aussi dist\Blocs).

#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif
#define MyAppName      "Blocs"
#define MyAppExeName   "Blocs.exe"
#define MyAppPublisher "Blocs"
#define MyAppURL       ""

[Setup]
; GUID propre à Blocs : ne jamais le changer (il identifie l'application pour les mises à jour).
AppId={{01F18C40-7256-43DD-B818-7BEF489798F8}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
#if MyAppURL != ""
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
#endif
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupIconFile=..\resources\blocs.ico
OutputDir=..\dist\installer
OutputBaseFilename=Blocs-{#MyAppVersion}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog
VersionInfoVersion={#MyAppVersion}
; Pour fournir une licence, décommenter (le fichier LICENSE doit exister à la racine) :
; LicenseFile=..\LICENSE

[Languages]
Name: "french";  MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "associate";   Description: "Ouvrir les fichiers .blocs avec Blocs"; GroupDescription: "Associations de fichiers :"

[Files]
Source: "..\dist\Blocs\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKA; Subkey: "Software\Classes\.blocs"; ValueType: string; ValueName: ""; ValueData: "Blocs.Project"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Blocs.Project"; ValueType: string; ValueName: ""; ValueData: "Projet Blocs"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Blocs.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Blocs.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: associate

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
