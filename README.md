# Asciixel

Asciixel is a tool that transforms pixels into asciixels(ASCII pixels)! Now it supports JPG, PNG. More formats will be supported in the future. It can output to terminal or PNG file.

<img src="examples/cat.jpg" width="200" alt="Raw image"/> <img src="examples/cat-ascii.png" width="200" alt="PNG output"/>

<img src="examples/kant.jpg" width="200" alt="Raw image"/> <img src="examples/kant-ascii.png" width="200" alt="PNGoutput"/>

## Quick Start

### Build Dependencies

- **Common**: CMake ≥ 3.16, a C/C++ toolchain supporting C++17, and zlib development headers and libraries.
- **Linux**: GCC/G++ and GNU make.
  - Debian/Ubuntu: `sudo apt install build-essential cmake zlib1g-dev`
  - Fedora: `sudo dnf install gcc gcc-c++ make cmake zlib-ng-compat-devel`
- **Windows**: MinGW-w64 (`gcc`, `g++`, and `make` available in PATH) and Git for Windows (provides Bash).
  - The MinGW environment must include zlib headers and libraries; nuwen MinGW 19.0 already bundles them.

### 1. Clone the repository and init submodules

```bash
git clone --recurse-submodules <仓库地址>
cd Asciixel
git submodule update --init --recursive
```

### 2. Build FFmpeg

Use provided scripts to build FFmpeg with only the necessary components. The scripts will download and build FFmpeg in `build/ffmpeg-install`. The default configuration disables all unnecessary features, including network support, and enables only the required decoders and encoders.

```bash
./scripts/build-ffmpeg.sh
```

```powershell
./scripts/build-ffmpeg.ps1
```

### 3. Configure and build the project

```bash
cmake -S . -B build
cmake --build build --parallel
```

### 4. Run

> [!NOTE]
> You can see some examples pictures on `examples/`.

```bash
./build/asciixel <photo-path>                                    # to terminal
./build/asciixel <photo-path> --output <output.png> # to PNG file

# test for terminal output
# it will use the pictures in directory examples
bash examples/test.sh # Linux
```

Currently, the program uses a default font path(Linux: `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf`, Windows: `C:/Windows/Fonts/consola.ttf`) and size. If the font is missing, it will report an error. You can install the required font or specify another font with `--font <path>`. Use `--font-size <N>` (1–256, default 24) and `--columns <N>` (1–4096, default 200) to customize rendering.

## Supported Arguments

Currently supports static JPG and PNG images, with terminal, PNG, or TXT output. Video is not yet supported.

The library provides sequential MP4/MOV H.264 decoding through `VideoLoader`
in `asciixel/io/video_loader.hpp`. `nextFrame()` returns an owned `VideoFrame`
containing an `ImageFrame` and optional source presentation timestamp and
duration in microseconds. It returns `std::nullopt` after draining delayed
frames. Source times are not rebased or synthesized, and audio packets are
skipped. The CLI does not yet dispatch video input.

Video pixels currently reuse the image normalizer's sRGB conversion. Video
color metadata, HDR, rotation and sample aspect ratio handling are not yet
implemented; correct display of those inputs is not guaranteed.

| Argument | Default | Description |
| --- | --- | --- |
| `<image-path>` | Required | Path to a single input image |
| `-o, --output [path]` | Terminal output | Save to a `.png` or `.txt` file; omit the path to create `<input-stem>_asciixel.png` beside the input |
| `--font <path>` | Platform default, see above | Monospace font file used for glyph calibration |
| `--font-size <N>` | 24 | Font size in pixels, integer from 1 to 256 |
| `--columns <N>` | 200 | Character columns, integer from 1 to 4096, capped at the source image width; rows follow the image and font cell proportions |
| `-h, --help` | — | Show help; must be used alone |
| `--` | — | End option parsing; subsequent tokens are treated as positional arguments |

Option values accept spaces or equals signs, such as `--columns 80`, `--columns=80`, and `-o=art.txt`.
Output extensions are case-insensitive. Existing output files are never overwritten. For terminal output, use the same font as specified by `--font` for a closer match.

```bash
./build/asciixel photo.jpg --columns 80
./build/asciixel photo.jpg -o art.txt
./build/asciixel photo.jpg -o
```

## Project Structure

```text
Asciixel/
├── CMakeLists.txt          # Build and test configuration
├── include/asciixel/      # Headers: app, config, core, io, model
├── src/
│   ├── main.cpp           # CLI entry point and output dispatch
│   ├── app/               # Single-frame conversion orchestration
│   ├── config/            # Defaults, option mapping, and validation
│   ├── core/              # Font calibration, grid, sampling, matching, rendering
│   └── io/                # Argument parsing, decoding, normalization, file output
├── tests/                 # Unit tests, CLI tests, and fixtures
├── scripts/               # FFmpeg build scripts
├── third_party/           # FFmpeg and FreeType
├── examples/              # Sample images and output examples
└── docs/                  # Design documents and review history
```
