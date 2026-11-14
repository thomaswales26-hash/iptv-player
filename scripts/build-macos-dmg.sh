#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="${VERSION:-1.0.2}"
ARCH="$(uname -m)"
OUTPUT="${OUTPUT:-$PROJECT_ROOT/IPTV-Player-${VERSION}-macos-${ARCH}.dmg}"
WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/iptv-player-dmg.XXXXXX")"
trap 'rm -rf "$WORK_DIR"' EXIT

PACKAGE="$WORK_DIR/IPTV-Player-${VERSION}-macos-${ARCH}.pkg"
VERSION="$VERSION" OUTPUT="$PACKAGE" "$PROJECT_ROOT/scripts/build-macos-pkg.sh"
(
    cd "$WORK_DIR"
    shasum -a 256 "IPTV-Player-${VERSION}-macos-${ARCH}.pkg" \
        > "$WORK_DIR/IPTV-Player-${VERSION}-macos-${ARCH}.pkg.sha256"
)

mkdir "$WORK_DIR/image"
cp "$PACKAGE" "$WORK_DIR/image/"
cp "$WORK_DIR/IPTV-Player-${VERSION}-macos-${ARCH}.pkg.sha256" "$WORK_DIR/image/"
cat > "$WORK_DIR/image/Install.txt" <<EOF
IPTV Player ${VERSION} for macOS ${ARCH}

Open the .pkg installer and follow the prompts. The app is installed in
/Applications/IPTV Player.app.

The app and installer are ad-hoc signed, not notarized. If Gatekeeper blocks
the app after installation, Control-click it in Finder, choose Open, then
confirm Open. For details, see THIRD_PARTY_NOTICES.md inside the app bundle.
EOF

hdiutil create \
    -volname "IPTV Player ${VERSION}" \
    -srcfolder "$WORK_DIR/image" \
    -format UDZO \
    -imagekey zlib-level=9 \
    -ov "$OUTPUT"
hdiutil verify "$OUTPUT"

echo "Created $OUTPUT"
