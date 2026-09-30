#!/usr/bin/env bash
# Build the FFmpeg submodule into build/ffmpeg-install.
# Linux counterpart of scripts/build-ffmpeg.ps1; keep the paths and configure flags in sync.
set -euo pipefail

jobs="${1:-$(nproc)}"

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_dir="$root/third_party/ffmpeg"
build_dir="$root/build/ffmpeg"
install_dir="$root/build/ffmpeg-install"

if [ ! -x "$source_dir/configure" ]; then
    echo "FFmpeg submodule is missing. Run: git submodule update --init --recursive" >&2
    exit 1
fi

mkdir -p "$build_dir"
cd "$build_dir"

"$source_dir/configure" \
    --prefix="$install_dir" \
    --disable-autodetect --disable-everything \
    --enable-shared --disable-static \
    --disable-programs --disable-doc --disable-network \
    --disable-x86asm \
    --enable-avformat --enable-avcodec --enable-swscale \
    --enable-decoder=png,mjpeg,h264 --enable-encoder=png \
    --enable-demuxer=image2,png_pipe,jpeg_pipe,mov,wav \
    --enable-protocol=file --enable-parser=png,mjpeg,h264 \
    --enable-zlib

make -j"$jobs"
make install
