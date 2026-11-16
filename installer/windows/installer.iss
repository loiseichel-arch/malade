; Windows installer: <Name>-<version>-windows-setup.exe
; Built by GitHub Actions with Inno Setup:
;   ISCC /DName=Malade /DVersion=0.3.0 /DSourceDir=build\Malade_artefacts\Release installer\windows\installer.iss

#ifndef Name
  #error Name is not defined
#endif

[Setup]
AppId=PlanetGloria.{#Name}
AppName={#Name}
AppVersion={#Version}
AppVerName={#Name} {#Version}
AppPublisher=Planet Gloria
DefaultDirName={autopf}\Planet Gloria\{#Name}
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputBaseFilename={#Name}-{#Version}-windows-setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#Name} (Planet Gloria)
ShowLanguageDialog=auto

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Types]
Name: "full"; Description: "VST3 + app"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: full custom
Name: "app";  Description: "{#Name} app (standalone)"; Types: full custom
Name: "aax";  Description: "AAX plug-in (Pro Tools, needs PACE signing)"; Types: custom

[Files]
Source: "{#SourceDir}\VST3\{#Name}.vst3\*"; DestDir: "{commoncf64}\VST3\{#Name}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\Standalone\{#Name}.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "{#SourceDir}\AAX\{#Name}.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\{#Name}.aaxplugin"; Components: aax; Flags: ignoreversion recursesubdirs createallsubdirs

; Licences of the fonts embedded in the plug-in (SIL Open Font License).
Source: "..\..\Assets\Fonts\OFL-*.txt"; DestDir: "{app}\Licenses"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#Name}"; Filename: "{app}\{#Name}.exe"; Components: app
