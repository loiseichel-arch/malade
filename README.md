# Malade v0.3

Plugin de vibrato (ex-« Vibrato ») de **Planet Gloria** (AAX pour Pro Tools, plus AU, VST3 et une app autonome pour tester) écrit avec JUCE.

## Ce qu'il fait

- **Rate / Depth** : vitesse et profondeur du vibrato. Depth 100 % = ±1 demi-ton.
- **LED rate** (à côté du titre LFO) : sa luminosité suit le LFO en fondu, comme sur un Jupiter-4, y compris quand l'enveloppe fait bouger le rate.
- **Formes du LFO** : sinus, triangle, carré, dent de scie montante, dent de scie descendante, aléatoire. La forme est celle du mouvement de hauteur, comme sur un synthé : le carré fait une vraie trille, sans clic.
- **Tempo sync** : Rate (et Rate 1 / Rate 2) passent en valeurs de note calées sur le tempo de Pro Tools.
- **Width** : décale le LFO entre gauche et droite (jusqu'à 180°).
- **Flutter** : petites variations de hauteur irrégulières, façon bande.
- **Vintage** : légère saturation et aigus adoucis sur le signal traité.
- **Mix, Output, On**.
- **Panneau Envelope** (bouton « envelope ») :
  - **Detection** (nouveau en v0.3) : ces réglages n'agissent que sur la détection, pas sur le son.
    - **source** : **input** (le son de la piste) ou **sidechain** (une autre piste envoyée sur l'entrée sidechain / « key » de Malade, par exemple une caisse claire qui relance le vibrato d'une nappe). Si aucun sidechain n'est branché, le panneau l'indique.
    - **Threshold** : niveau à partir duquel une nouvelle attaque est détectée (chaque nouvelle note ou accord relance les mouvements).
    - **Contrast** : expandeur sur la détection. Les attaques ressortent du sustain ; plus il est haut, plus les attaques douces (legato) sont prises. À 100 % il peut déclencher en trop.
    - **SC HP / SC LP** : filtres sur la source de détection (par défaut HP à 150 Hz, pour que la basse ne masque pas les attaques).
    - **listen** : fait entendre le signal de détection filtré à la place de l'effet, pour régler les filtres à l'oreille.
    - Le vumètre montre le signal de détection avec le seuil ; la **LED trig** flashe à chaque attaque détectée.
    - Mesuré sur des notes pincées au-dessus d'une basse au même niveau : 0 note sur 20 détectée avant (la basse masquait tout), 19 à 20 sur 20 avec les réglages par défaut.
  - **Rate Move** : le rate démarre à Rate 1 sur l'attaque et glisse vers Rate 2 en Time.
  - **Depth Move** : aucun vibrato pendant Delay, fondu très court vers Depth 1, puis glissement vers Depth 2 en Time.
  - Quand un mouvement est actif, le grand potard correspondant est verrouillé, s'anime tout seul et sa LED ENV s'allume.
- **Alt + clic** sur un potard, un interrupteur ou une forme de LFO : retour à la valeur par défaut (règle commune à tous les plugins Planet Gloria). Le double-clic marche aussi sur les potards.

## Ce qu'il faut sur ton Mac

1. **Xcode** (App Store), ouvert une fois pour accepter la licence.
2. **CMake** 3.22 ou plus : `brew install cmake` (avec [Homebrew](https://brew.sh)) ou l'installeur de cmake.org.
3. **Git** (installé avec Xcode).

JUCE est téléchargé automatiquement au premier build, et il contient déjà le SDK AAX.

## Polices

Les polices **Patrick Hand** et **Caveat** (licence libre SIL OFL, dans `Assets/Fonts/`) sont intégrées au plugin : l'interface est la même sur Mac et sur Windows.

## Compiler

Dans le Terminal, depuis le dossier du projet :

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

Le premier build prend plusieurs minutes. Les plugins sont ensuite ici :

```
build/Malade_artefacts/Release/AAX/Malade.aaxplugin
build/Malade_artefacts/Release/AU/Malade.component
build/Malade_artefacts/Release/VST3/Malade.vst3
build/Malade_artefacts/Release/Standalone/Malade.app
```

Tu peux aussi ouvrir `build/Malade.xcodeproj` dans Xcode et compiler depuis là.

Sur Windows (Visual Studio) : `cmake -B build -A x64` puis `cmake --build build --config Release`.

## Sans compiler : la compilation automatique sur GitHub

Le dossier `.github/workflows/` contient une recette qui fait compiler le plugin par un Mac de GitHub (gratuit pour un dépôt public, et dans la limite des minutes offertes pour un dépôt privé).

1. Mets ce dossier dans un dépôt GitHub.
2. Va dans l'onglet **Actions** du dépôt : une compilation « Build macOS » démarre à chaque envoi (ou clique sur *Run workflow*). Elle prend environ 10 à 20 minutes.
3. Quand elle est verte, ouvre-la et télécharge en bas de la page **Malade-macOS-…** (Mac, universel Apple Silicon + Intel, macOS 11 et plus) ou **Malade-Windows** (Windows 64 bits : VST3, AAX, app).
4. Suis `INSTALL_MAC.txt` ou `INSTALL_WINDOWS.txt`, inclus dans le téléchargement.

## Tester tout de suite

- **App autonome** : lance `Malade.app`, choisis ton entrée audio dans *Options*, et joue. C'est le moyen le plus rapide de régler le son.
- **AU** : copie `Malade.component` dans `~/Library/Audio/Plug-Ins/Components/` pour l'essayer dans Logic, GarageBand ou tout hôte AU.

## Dans Pro Tools

Pro Tools ne charge que des plugins AAX **signés**. Deux étapes :

1. **Pour développer** : inscris-toi comme développeur chez Avid et demande une licence **Pro Tools Developer**. Cette version de Pro Tools charge les plugins non signés. Copie le plugin ainsi :

   ```bash
   sudo cp -R build/Malade_artefacts/Release/AAX/Malade.aaxplugin "/Library/Application Support/Avid/Audio/Plug-Ins/"
   ```

2. **Pour ton Pro Tools habituel** : signe le plugin avec les outils PACE (*wraptool*), qu'Avid fournit aux développeurs inscrits, avec un compte iLok. PACE te donnera la commande exacte avec tes identifiants.

Attention : en 2026, plusieurs développeurs signalent des réponses lentes d'Avid pour ces deux demandes. Fais-les tôt, relance si besoin, et le support PACE (support@paceap.com) est réputé réactif.

Constructeur : **Planet Gloria**, code constructeur `Pglo`, identifiant `com.planetgloria.malade` (dans `CMakeLists.txt`). Les mêmes nom et code pour tous les plugins Planet Gloria, pour qu'ils soient rangés ensemble dans les logiciels audio.

Passage de Vibrato à Malade : l'identité du plugin a changé, les anciennes sessions ne le retrouvent pas. Il faut y réinsérer Malade.

## Organisation du code

```
Source/
  dsp/VibratoEngine.h        moteur audio (C++ pur, sans JUCE)
  Parameters.h               tous les paramètres automatisables
  PluginProcessor.*          le plugin : paramètres -> moteur, valeurs live pour l'interface
  PluginEditor.*             la fenêtre et le panneau Envelope
  ui/Sketch.h                couleurs, polices, traits dessinés à la main
  ui/SketchLookAndFeel.*     dessin des potards et interrupteurs
  ui/Components.h            potard avec label/LED, sélecteur de forme, écran du LFO, LED rate, vumètre, interrupteur
```

## Limites connues

- Pas de système de presets intégré : utilise le menu de presets de Pro Tools en haut de la fenêtre du plugin.
- Le LFO synchronisé suit le tempo mais n'est pas recalé sur la position de lecture (il ne repart pas pile sur le temps).
