# Familiar Paint: welcome CLI demo

23 seconds, 1280×1024, 30 fps, H.264/yuv420p, intentionally silent.

The six PNG stages were actually produced by the supplied Familiar Paint CLI.
The capture helper opens each PNG through the actual Window::open file-loading
path and captures the real Qt window. The edit holds these captures and adds
command captions. This is staged playback, not a continuous desktop recording or
a demonstration of live IPC. That qualification stays visible throughout.

The opening shows the finished card; the following shots explain its creation.
There are no image-generation assets, simulated app controls, music or narration.
Codex authored the JSON artwork, C++ capture helper, JavaScript runner and edit.
FFmpeg encoded the result. No Python is used in the app or demo source.

## Use the artwork

From this directory, with Qt installed:

```sh
../target/familiar-paint --render welcome.json --output my-welcome.png
../target/familiar-paint my-welcome.png
```

Use a new output filename, or explicitly use `--force` to replace an old one.

## Reproduce the captured stages and edit

Requires a C++17 compiler, Qt 6 development files, CMake, Node.js and FFmpeg with
libx264/drawtext. The DejaVu Sans fonts used by the edit must be installed.
Build from this directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target capture --parallel 2
QT_QPA_PLATFORM=offscreen node make-demo.cjs /tmp/paint-welcome-fresh
node edit.cjs /tmp/paint-welcome-fresh
```

The output directory must not already exist. This preserves original captures.
Build the app from the repository root first (`cmake -S . -B target` and
`cmake --build target --parallel 2`). This repository stores source; binaries
are produced by CI.

## What was tested

- Six successful CLI render operations, each using the previous PNG as input.
- Each saved image loaded through the actual app's asynchronous Open path.
- Final image inspected through `--inspect`: 1000×580, PNG.
- Single combined batch and six-stage output are pixel-identical after decoding.
- An invalid command after a valid command returns exit 1 and creates no output.
- Existing output is refused without `--force`.
- Full MP4 decoded without errors; metadata confirms 690 frames / 23 seconds.
- Representative frames inspected for all stages, opening, captions and ending.

Evidence lives in `logs/`; untouched Qt captures live in `captures/`.
No live socket listener was used: it remains blocked in this managed environment.
No real Omarchy/Hyprland session is shown. No timing/performance claim is made.
