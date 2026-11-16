#!/bin/bash
# Builds "<Name> <version>.dmg": a disk image with an installer package
# (VST3, AU, AAX and the app, each one can be unticked) and a short guide.
# Files installed by macOS Installer are not quarantined, so nothing has to
# be unblocked in the Terminal afterwards.
#
# usage: installer/mac/make_dmg.sh <Name> <version> <artefacts Release dir> <out dir>
set -euo pipefail

NAME="$1"; VERSION="$2"; R="$3"; OUT="$4"
ID="com.planetgloria.$(echo "$NAME" | tr '[:upper:]' '[:lower:]')"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
mkdir -p "$OUT" "$WORK/pkgs" "$WORK/scripts"

# After install: let macOS rescan Audio Units.
cat > "$WORK/scripts/postinstall" <<'POST'
#!/bin/bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
exit 0
POST
chmod +x "$WORK/scripts/postinstall"

# One package per format.
make_pkg () {   # key, bundle, install folder, title
    local key="$1" bundle="$2" dest="$3"
    [ -d "$bundle" ] || { echo "skip $key (no $bundle)" >&2; return 0; }
    local root="$WORK/root-$key"
    mkdir -p "$root$dest"
    ditto "$bundle" "$root$dest/$(basename "$bundle")" >&2
    # Never "relocate" to an old copy found elsewhere on the disk.
    pkgbuild --analyze --root "$root" "$WORK/$key.plist" >/dev/null
    i=0
    while /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$WORK/$key.plist" 2>/dev/null; do i=$((i+1)); done
    pkgbuild --root "$root" --component-plist "$WORK/$key.plist" --identifier "$ID.$key" --version "$VERSION" \
             --install-location / --scripts "$WORK/scripts" "$WORK/pkgs/$key.pkg" >&2
    echo "$key"
}

PKGS=()
PKGS+=($(make_pkg vst3 "$R/VST3/$NAME.vst3"           "/Library/Audio/Plug-Ins/VST3"))
PKGS+=($(make_pkg au   "$R/AU/$NAME.component"        "/Library/Audio/Plug-Ins/Components"))
PKGS+=($(make_pkg aax  "$R/AAX/$NAME.aaxplugin"       "/Library/Application Support/Avid/Audio/Plug-Ins"))
PKGS+=($(make_pkg app  "$R/Standalone/$NAME.app"      "/Applications"))

title () { case "$1" in vst3) echo "VST3 plug-in";; au) echo "Audio Unit (AU) plug-in";; aax) echo "AAX plug-in (Pro Tools, needs PACE signing)";; app) echo "$NAME app (standalone)";; esac; }

{
  echo '<?xml version="1.0" encoding="utf-8"?>'
  echo '<installer-gui-script minSpecVersion="2">'
  echo "  <title>$NAME $VERSION by Planet Gloria</title>"
  echo '  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>'
  echo '  <domains enable_localSystem="true"/>'
  echo '  <volume-check><allowed-os-versions><os-version min="11.0"/></allowed-os-versions></volume-check>'
  echo '  <readme file="readme.txt"/>'
  echo '  <choices-outline>'
  for k in "${PKGS[@]}"; do echo "    <line choice=\"$k\"/>"; done
  echo '  </choices-outline>'
  for k in "${PKGS[@]}"; do
    start="true"; [ "$k" = "aax" ] && start="false"
    echo "  <choice id=\"$k\" title=\"$(title "$k")\" start_selected=\"$start\"><pkg-ref id=\"$ID.$k\"/></choice>"
    echo "  <pkg-ref id=\"$ID.$k\" version=\"$VERSION\">$k.pkg</pkg-ref>"
  done
  echo '</installer-gui-script>'
} > "$WORK/distribution.xml"

mkdir -p "$WORK/res"
sed "s/@NAME@/$NAME/g; s/@VERSION@/$VERSION/g" "$HERE/readme.txt" > "$WORK/res/readme.txt"

DMGDIR="$WORK/dmg"
mkdir -p "$DMGDIR"
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK/pkgs" --resources "$WORK/res" \
             "$DMGDIR/Install $NAME.pkg"
cp "$WORK/res/readme.txt" "$DMGDIR/Read me.txt"

# Licences of the fonts embedded in the plug-in (SIL Open Font License).
mkdir -p "$DMGDIR/Licenses"
cp "$HERE/../../Assets/Fonts/"OFL-*.txt "$DMGDIR/Licenses/"

hdiutil create -volname "$NAME $VERSION" -srcfolder "$DMGDIR" -ov -format UDZO "$OUT/$NAME-$VERSION-mac.dmg"
echo "built $OUT/$NAME-$VERSION-mac.dmg"
