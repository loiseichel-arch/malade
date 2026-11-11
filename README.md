# Vibrato v0.1

Plugin de vibrato (AAX pour Pro Tools, plus AU, VST3 et une app autonome pour tester) écrit avec JUCE.

## Ce qu'il fait

- **Rate / Depth** : vitesse et profondeur du vibrato. Depth 100 % = ±1 demi-ton.
- **Formes du LFO** : sinus, triangle, carré, dent de scie montante, dent de scie descendante, aléatoire. La forme est celle du mouvement de hauteur, comme sur un synthé : le carré fait une vraie trille, sans clic.
- **Tempo sync** : Rate (et Rate 1 / Rate 2) passent en valeurs de note calées sur le tempo de Pro Tools.
- **Width** : décale le LFO entre gauche et droite (jusqu'à 180°).
- **Flutter** : petites variations de hauteur irrégulières, façon bande.
- **Vintage** : légère saturation et aigus adoucis sur le signal traité.
- **Mix, Output, On**.
- **Panneau Envelope** (bouton « envelope ») :
  - **Threshold** : niveau à partir duquel une nouvelle attaque est détectée (chaque nouvelle note ou accord relance les mouvements).
  - **Rate Move** : le rate démarre à Rate 1 sur l'attaque et glisse vers Rate 2 en Time.
  - **Depth Move** : aucun vibrato pendant Delay, fondu très court vers Depth 1, puis glissement vers Depth 2 en Time.
  - Quand un mouvement est actif, le grand potard correspondant est verrouillé, s'anime tout seul et sa LED ENV s'allume.

## Ce qu'il faut sur ton Mac

1. **Xcode** (App Store), ouvert une fois pour accepter la licence.
2. **CMake** 3.22 ou plus : `brew install cmake` (avec [Homebrew](https://brew.sh)) ou l'installeur de cmake.org.
3. **Git** (installé avec Xcode).

JUCE est téléchargé automatiquement au premier build, et il contient déjà le SDK AAX.

## Polices (optionnel)

Sans rien faire, l'interface utilise les polices du Mac *Chalkboard SE* et *Marker Felt*.
Pour le rendu exact de la maquette, télécharge sur Google Fonts **Patrick Hand** et **Caveat** (gratuites), puis copie les fichiers `.ttf` dans `Assets/Fonts/` (par exemple `PatrickHand-Regular.ttf` et `Caveat-Bold.ttf`, ce dernier se trouve dans le dossier `static` du zip de Caveat). Relance ensuite la configuration CMake.

## Compiler

Dans le Terminal, depuis le dossier du projet :

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

Le premier build prend plusieurs minutes. Les plugins sont ensuite ici :

```
build/Vibrato_artefacts/Release/AAX/Vibrato.aaxplugin
build/Vibrato_artefacts/Release/AU/Vibrato.component
build/Vibrato_artefacts/Release/VST3/Vibrato.vst3
build/Vibrato_artefacts/Release/Standalone/Vibrato.app
```

Tu peux aussi ouvrir `build/Vibrato.xcodeproj` dans Xcode et compiler depuis là.

## Sans compiler : la compilation automatique sur GitHub

Le dossier `.github/workflows/` contient une recette qui fait compiler le plugin par un Mac de GitHub (gratuit pour un dépôt public, et dans la limite des minutes offertes pour un dépôt privé).

1. Mets ce dossier dans un dépôt GitHub.
2. Va dans l'onglet **Actions** du dépôt : une compilation « Build macOS » démarre à chaque envoi (ou clique sur *Run workflow*). Elle prend environ 10 à 20 minutes.
3. Quand elle est verte, ouvre-la et télécharge **Vibrato-macOS** en bas de la page.
4. Suis `INSTALL_MAC.txt`, qui est inclus dans le téléchargement.

## Tester tout de suite

- **App autonome** : lance `Vibrato.app`, choisis ton entrée audio dans *Options*, et joue. C'est le moyen le plus rapide de régler le son.
- **AU** : copie `Vibrato.component` dans `~/Library/Audio/Plug-Ins/Components/` pour l'essayer dans Logic, GarageBand ou tout hôte AU.

## Dans Pro Tools

Pro Tools ne charge que des plugins AAX **signés**. Deux étapes :

1. **Pour développer** : inscris-toi comme développeur chez Avid et demande une licence **Pro Tools Developer**. Cette version de Pro Tools charge les plugins non signés. Copie le plugin ainsi :

   ```bash
   sudo cp -R build/Vibrato_artefacts/Release/AAX/Vibrato.aaxplugin "/Library/Application Support/Avid/Audio/Plug-Ins/"
   ```

2. **Pour ton Pro Tools habituel** : signe le plugin avec les outils PACE (*wraptool*), qu'Avid fournit aux développeurs inscrits, avec un compte iLok. PACE te donnera la commande exacte avec tes identifiants.

Attention : en 2026, plusieurs développeurs signalent des réponses lentes d'Avid pour ces deux demandes. Fais-les tôt, relance si besoin, et le support PACE (support@paceap.com) est réputé réactif.

Avant de signer et de distribuer, remplace `MyCompany`, le code `Mycp` et `com.mycompany.vibrato` dans `CMakeLists.txt` par les tiens.

## Organisation du code

```
Source/
  dsp/VibratoEngine.h        moteur audio (C++ pur, sans JUCE)
  Parameters.h               tous les paramètres automatisables
  PluginProcessor.*          le plugin : paramètres -> moteur, valeurs live pour l'interface
  PluginEditor.*             la fenêtre et le panneau Envelope
  ui/Sketch.h                couleurs, polices, traits dessinés à la main
  ui/SketchLookAndFeel.*     dessin des potards et interrupteurs
  ui/Components.h            potard avec label/LED, sélecteur de forme, écran du LFO, vumètre
```

## Limites connues de la v0.1

- Pas de système de presets intégré : utilise le menu de presets de Pro Tools en haut de la fenêtre du plugin.
- Le LFO synchronisé suit le tempo mais n'est pas recalé sur la position de lecture (il ne repart pas pile sur le temps).
