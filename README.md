# IPTV Player

A lightweight terminal IPTV player for extended M3U/M3U8 channel playlists. Playlist parsing is built in; media playback and recording are handled by FFmpeg's `ffplay` and `ffmpeg` command-line tools.

## Requirements

- CMake 3.20 or newer
- A C++20 compiler
- FFmpeg (`ffplay` and `ffmpeg`) available when building, or installed on `PATH` when running
- `curl` on `PATH` to load a playlist from an HTTP(S) URL

When found during CMake configuration, `ffplay` and `ffmpeg` are copied beside the player and included in `cmake --install`. On macOS, CMake builds an `IPTV Player.app` bundle and includes non-system shared-library dependencies under `Contents/Frameworks`, with load paths rewritten to use the bundled libraries. Install the app with:

```sh
cmake --install build --prefix /Applications
```

Or build the macOS `.pkg` installer:

```sh
./scripts/build-macos-pkg.sh
```

The package is architecture-specific. Without signing identities it is
ad-hoc-signed and not notarized, so macOS may block it on first launch. In
Finder, Control-click the app, choose **Open**, then confirm **Open**. For
trusted distribution, set `CODESIGN_IDENTITY` to a Developer ID Application
identity and `PKG_SIGN_IDENTITY` to a Developer ID Installer identity when
building; notarization additionally requires Apple Developer credentials.
Downloadable packages are published on the GitHub Releases page.

Double-clicking the app opens a playlist picker and launches the player in
Terminal. To start it directly from Terminal instead:

```sh
"/Applications/IPTV Player.app/Contents/MacOS/IPTV Player" channels.m3u
```

The macOS package contains a Homebrew-built FFmpeg distribution, including GPL
components. Review `THIRD_PARTY_NOTICES.md` in the app's Resources folder for
component licenses and source links.

The player prefers bundled tools; set `IPTV_FFPLAY` and/or `IPTV_FFMPEG` to override them. Otherwise, it searches `PATH`.

Bundles remain architecture-specific and use the destination system's native macOS frameworks. For redistribution, use an FFmpeg build compatible with its license and include the required license notices and source/offers.

## Build and run

```sh
cmake -S . -B build
cmake --build build
```

On macOS, run `build/IPTV Player.app/Contents/MacOS/IPTV Player channels.m3u`.
On other platforms, run `./build/iptv-player channels.m3u`.

You can also pass an HTTP(S) URL to an extended M3U playlist. Local playlists may use relative stream paths.

## Commands

- `list` — list channels
- `play 2` — play channel 2; press `q` in the FFplay window to stop
- `record 2 3600 "evening recording.ts"` — record channel 2 for one hour
- `help` — show commands
- `quit` — exit

Recording uses stream copy, so it does not re-encode the stream. Choose a filename extension that matches the desired container. Only record streams you are authorized to access and record.
