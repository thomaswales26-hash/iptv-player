#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build-macos}"
VERSION="${VERSION:-1.0.2}"
ARCH="$(uname -m)"
OUTPUT="${OUTPUT:-$PROJECT_ROOT/IPTV-Player-${VERSION}-macos-${ARCH}.pkg}"
WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/iptv-player-pkg.XXXXXX")"
trap 'rm -rf "$WORK_DIR"' EXIT

cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel
cmake --install "$BUILD_DIR" --prefix "$WORK_DIR/root/Applications"
APP_PATH="$WORK_DIR/root/Applications/IPTV Player.app"
if [[ -n "${CODESIGN_IDENTITY:-}" ]]; then
    codesign --force --deep --options runtime --timestamp \
        --sign "$CODESIGN_IDENTITY" "$APP_PATH"
else
    codesign --force --deep --sign - --timestamp=none "$APP_PATH"
fi
codesign --verify --deep --strict "$APP_PATH"
chmod -R u+w "$WORK_DIR/root"
xattr -cr "$WORK_DIR/root"

if [[ -n "${PKG_SIGN_IDENTITY:-}" ]]; then
    pkgbuild \
        --root "$WORK_DIR/root" \
        --filter '(^|/)\._[^/]*$' \
        --filter '(^|/)\.DS_Store$' \
        --filter '(^|/)(\.svn|CVS)(/|$)' \
        --identifier com.iptvplayer.app \
        --version "$VERSION" \
        --install-location / \
        --sign "$PKG_SIGN_IDENTITY" \
        "$OUTPUT"
else
    pkgbuild \
        --root "$WORK_DIR/root" \
        --filter '(^|/)\._[^/]*$' \
        --filter '(^|/)\.DS_Store$' \
        --filter '(^|/)(\.svn|CVS)(/|$)' \
        --identifier com.iptvplayer.app \
        --version "$VERSION" \
        --install-location / \
        "$OUTPUT"
fi

echo "Created $OUTPUT"
