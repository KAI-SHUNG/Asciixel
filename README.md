# Asciixel

Asciixel transforms pixels into asciixels (ASCII pixels). It supports static
JPG/PNG images with terminal, PNG or TXT output, and terminal playback of
MP4/MOV/M4V H.264 video.

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

Static JPG/PNG images support terminal, PNG or TXT output. MP4/MOV/M4V H.264
video supports interactive terminal playback; video file export is not supported.

The library provides sequential MP4/MOV H.264 decoding through `VideoLoader`
in `asciixel/io/video_loader.hpp`. `nextFrame()` returns an owned `VideoFrame`
containing an `ImageFrame` and optional source presentation timestamp and
duration in microseconds. It returns `std::nullopt` after draining delayed
frames. Source times are not rebased or synthesized, and audio packets are
skipped. The CLI selects video playback for `.mp4`, `.mov` and `.m4v` input
extensions, case-insensitively, then validates the video stream when opening it.

`Color` stores encoded sRGB as three `std::uint8_t` channels in `[0, 255]`.
`ImagePixel` occupies 3 bytes and `AsciiPixel` occupies 4 bytes including its
character. Loader background colors use the same byte representation.
Sampling decodes bytes through an sRGB lookup table and averages in linear
light. The small sampled grid uses `LinearColor` floats in `[0, 1]` until glyph
matching is complete; final character colors are then encoded back to sRGB8.
Transparent pixels are composited in linear light before byte storage.

Video pixels currently reuse the image normalizer's sRGB interpretation. Video
color metadata, HDR, rotation and sample aspect ratio handling are not yet
implemented; correct display of those inputs is not guaranteed.

| Argument | Default | Description |
| --- | --- | --- |
| `<input-path>` | Required | Path to a single image or supported video |
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

## Video Playback

```bash
./build/asciixel clip.mp4 --columns 80
```

The video pipeline first decodes and converts every frame with `convertFrame`,
caching only character frames and playback-relative timestamps in memory.
Source RGB images are released after each conversion, and the decoder closes
before playback starts. Preparation time delays the start of playback; memory
usage grows with video duration and character grid size (4 bytes per cell,
plus frame metadata). No frames are displayed until preparation finishes.

`convertVideo(config)` in `asciixel/app/video_pipeline.hpp` selects the output
path. `prepareVideo(config)` in `asciixel/app/video_converter.hpp` returns the
converted `AsciiVideo` cache, or `std::nullopt` on interruption when an
interruption callback is supplied. Conversion is independent of output type.
Each cached frame records a playback-relative `timestamp_us` and resolved
`duration_us`, usable for both real-time scheduling and future file encoding.

`core/video_timeline` resolves timestamps and durations without clocks or I/O.
`app/video_player` only schedules cached frames through device-independent
callbacks. `io/terminal_video_output` owns the actual clock, cursor positioning,
writes, and terminal/interrupt state restoration.
The MP4 pipeline branch is separate from real-time playback and currently
reports that export is not implemented; a future exporter will use the ASCII
renderer and a video writer without waiting for playback deadlines.

Playback reuses the text writer and moves upward by the preceding frame's row
count before drawing the next frame. It starts on the current terminal line,
does not clear the screen, and leaves the last displayed frame visible. The
terminal window size does not restrict playback; frames may wrap or scroll
when they exceed the window. Redirected stdout and video `-o` requests are rejected.

Conversion displays a progress bar and percentage estimated from the selected video stream duration, plus the number of converted frames. The percentage stays below 100 until decoding and conversion finish. Unknown durations show an activity indicator and frame count. Updates are limited to roughly once per 100 ms, and the progress line is cleared before playback or on cancellation/failure. The reusable `io/terminal_progress` accepts completed/total numeric units and display text without depending on video data.

The playback clock starts after preparation. Frames follow source presentation
timestamps on a monotonic clock. Missing or
non-increasing timestamps use the preceding duration, then the source frame
interval, then 30 fps. No frames are dropped; slow terminal output may delay
playback. Audio is ignored. Ctrl+C stops preparation or playback with exit code
130 and restores the cursor and terminal modes. FFmpeg diagnostics are suppressed
while terminal output is active to keep positioning intact; runtime failures are still
reported after terminal cleanup.

Run `ctest --test-dir build --output-on-failure` for the automated suite. On
Windows, this additional test requires an interactive terminal and uses an
inactive console buffer to verify row positioning and terminal restoration,
including interruption of a child playback process:

```powershell
./build/test_text_terminal_session.exe ./build/asciixel.exe tests/fixtures/video_bframes.mp4
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
