@NAME@ @VERSION@ - Planet Gloria

INSTALL
1. Double-click "Install @NAME@.pkg" and follow the steps.
   You can choose which formats to install (VST3, Audio Unit, app).
2. The installer is not signed by Apple, so the first time macOS may say
   it "cannot be opened". Then:
     - right-click (or Ctrl-click) "Install @NAME@.pkg" > Open > Open,
     - or, on recent macOS: System Settings > Privacy & Security, scroll
       down and click "Open Anyway" next to "Install @NAME@.pkg".
3. Restart your audio software and rescan plug-ins.
   @NAME@ is listed under the maker "Planet Gloria".

Installed to:
  VST3 : /Library/Audio/Plug-Ins/VST3/@NAME@.vst3
  AU   : /Library/Audio/Plug-Ins/Components/@NAME@.component
  AAX  : /Library/Application Support/Avid/Audio/Plug-Ins/@NAME@.aaxplugin
  App  : /Applications/@NAME@.app

The AAX version is not PACE-signed: it only loads in Pro Tools Developer.
In regular Pro Tools, load the VST3 or AU inside a host such as Blue Cat
PatchWork.

Works on macOS 11 and later, Apple Silicon and Intel.
To uninstall, delete the files listed above.

-------------------------------------------------------------------------
FRANÇAIS
1. Double-clique sur « Install @NAME@.pkg » et suis les étapes (tu peux
   choisir les formats : VST3, Audio Unit, app).
2. L'installateur n'est pas signé par Apple : si macOS refuse de l'ouvrir,
   fais clic droit > Ouvrir > Ouvrir, ou va dans Réglages Système >
   Confidentialité et sécurité > « Ouvrir quand même ».
3. Relance ton logiciel audio et refais un scan des plugins.
   @NAME@ est rangé sous le constructeur « Planet Gloria ».
