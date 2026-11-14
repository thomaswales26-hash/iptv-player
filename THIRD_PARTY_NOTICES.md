# Third-party notices

The macOS installer bundles FFmpeg 9.0.2 and shared libraries from Homebrew
builds. FFmpeg was configured with `--enable-gpl` and includes GPL components.
These components are separate executables/libraries; this project does not
statically link FFmpeg into the IPTV Player executable.

The IPTV Player source is MIT licensed. Third-party components retain their
own licenses. License texts and corresponding upstream source are available
from the links below; FFmpeg's GPL source is at
<https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz>. The build configuration is
reported by `ffmpeg -version` in the installed app.

| Component | Version | License | Source |
|---|---:|---|---|
| FFmpeg | 9.0.2 | GPL-3.0-or-later | <https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz> |
| SDL2-compat | 2.32.74 | Zlib | <https://github.com/libsdl-org/sdl2-compat/releases/tag/release-2.32.74> |
| SDL3 | 3.4.20 | Zlib | <https://github.com/libsdl-org/SDL/releases/tag/release-3.4.20> |
| dav1d | 1.5.4 | BSD-2-Clause | <https://code.videolan.org/videolan/dav1d/-/archive/1.5.4/dav1d-1.5.4.tar.bz2> |
| LAME | 4.0 | LGPL-2.0-or-later | <https://downloads.sourceforge.net/project/lame/lame/4.0/> |
| mpg123 | 1.33.7 | LGPL-2.1-only | <https://www.mpg123.de/download.shtml> |
| libvmaf | 3.2.1 | BSD-2-Clause-Patent | <https://github.com/Netflix/vmaf/releases/tag/v3.2.1> |
| libvpx | 1.17.0 | BSD-3-Clause | <https://github.com/webmproject/libvpx/releases/tag/v1.17.0> |
| OpenSSL | 4.1.0 | Apache-2.0 | <https://github.com/openssl/openssl/releases/tag/openssl-4.1.0> |
| Opus | 1.6.1 | BSD-3-Clause | <https://ftp.osuosl.org/pub/xiph/releases/opus/opus-1.6.1.tar.gz> |
| SVT-AV1 | 4.3.0 | BSD-3-Clause | <https://gitlab.com/AOMediaCodec/SVT-AV1/-/releases/v4.3.0> |
| x264 | r3222 | GPL-2.0-or-later | <https://code.videolan.org/videolan/x264> |
| x265 | 4.3 | GPL-2.0-or-later | <https://github.com/Multicorewareinc/x265/releases/tag/4.3> |
| xz | 5.8.4 | 0BSD and GPL-2.0-or-later | <https://github.com/tukaani-project/xz/releases/tag/v5.8.4> |

System frameworks and libraries supplied by macOS are not included. Homebrew
formula metadata and build recipes can be found at
<https://github.com/Homebrew/homebrew-core>.
