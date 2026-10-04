# Familiar Paint guide

## Development preview

Version 0.0.2 is an early preview. Downloadable previews are available from GitHub Releases; live desktop validation remains pending. Intended target: Omarchy 4 / Hyprland on Linux x86_64. Tested locally: Ubuntu 24.04.3, GCC 13.3, Qt 6.4.2, offscreen rendering. **No installed Omarchy version has been tested.** The badge identifies a community app, not official endorsement.

### What's here

- Brush, white eraser, contiguous colour fill, line, arrow, rectangle and ellipse.
- Filled or outlined shapes, colour swatches, custom colours and eyedropper.
- Multiline text with independent 8–200 px sizing, using the system font; text rasterises when placed.
- Rectangular selection, copy selection, crop, resize, rotate and flip.
- Paste a clipboard image as a new document, with an unsaved-work prompt.
- Open PNG/JPEG/BMP/WebP where the Qt image plugin is installed; save PNG/JPEG/BMP.
- Undo/redo with a 192 MiB snapshot budget; maximum canvas size 16 million pixels.
- Classic two-column icon toolbox, live brush preview and a 28-swatch square palette.
- 25–400% zoom slider, Fit and 1:1 controls, keyboard shortcuts, save/discard/cancel on close and replacement.
- Atomic file replacement, external-change detection and coordination between Paint saves.
- Background file loading/saving; editing pauses until the operation completes.
- JSON drawing CLI, headless rendering, image inspection and opt-in live agent control.
- Atomic drawing batches with one undo step and revision checks for live edits.
- Omarchy semantic theme colours, refreshed every two seconds, with fallback colours.

### Deliberate preview limits

No layers, movable pasted objects, arbitrary selection transforms, print support, autosave or crash recovery yet. Save regularly. Eraser paints white; it does not erase to transparency. PNG preserves alpha, while JPEG/BMP flatten onto white. Resize currently stretches to the entered dimensions. Text size is controlled independently from brush width.

External-change detection catches modifications before saving, and Paint windows coordinate through a short-lived lock. Other editors do not honour that lock, so simultaneous writes from unrelated apps still have a race window.

## Run Paint

This repository contains the standalone native app for Omarchy.

On Omarchy/Arch, build and install for your user:

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-wayland
git clone https://github.com/tcballard/omarchy-app-familiar-paint.git
cd omarchy-app-familiar-paint
cmake -S . -B target -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build target --parallel 2
cmake --install target --prefix "$HOME/.local"
familiar-paint
```

CI also uploads a Linux x86_64 executable after successful tests. It uses
system Qt libraries and is not an AppImage or a published release.

The app does not change default image associations or Hyprland keybindings. Super+W is handled by the desktop's normal close-window action. Its real-session behaviour still needs checking.

## Let your agent paint

Start an agent-enabled window, then give your agent the [CLI guide](AGENT-CLI.md):

```bash
./target/familiar-paint --listen studio
```

From another terminal, draw the supplied example and export it:

```bash
./target/familiar-paint --send studio --apply examples/house.json
./target/familiar-paint --send studio --export house.png
./target/familiar-paint --send studio --undo
```

Or render without a window:

```bash
./target/familiar-paint --render examples/house.json --output house.png
```

Your agent generates structured drawing commands; no language model is bundled.
Live control is opt-in, local to your user and switchable off in the status bar.
Headless rendering and the live CLI/socket round trip passed on GitHub's Ubuntu
runner in [Native checks](https://github.com/tcballard/omarchy-app-familiar-paint/actions/runs/37185638348).
Testing on an actual Omarchy desktop remains outstanding.

## Build from source

Requires CMake 3.22+, a C++17 compiler, Qt 6.4+ Widgets/Concurrent/Network, and Qt Test for tests. On Arch, the development components are included in `qt6-base`.

```bash
cmake -S . -B target -DCMAKE_BUILD_TYPE=Release
cmake --build target --parallel 2
ctest --test-dir target --output-on-failure
./target/familiar-paint
```

For a conventional prefix install: `cmake --install target --prefix "$HOME/.local"`.

## Remove or roll back

Close the app, then run `bash scripts/uninstall.sh`. Saved images remain wherever you saved them. This preview creates no persistent app settings or recovery files.

The binary-preview installer backs up an existing binary to `~/.local/bin/familiar-paint.previous`. To roll back, close the app and replace `~/.local/bin/familiar-paint` with that backup. `FAMILIAR_INSTALL_PREFIX` can override the install/uninstall prefix; use the same value for both. Source installs through CMake do not create this backup.

## CLI welcome demo

[Watch the 23-second welcome demo](../demo-video/familiar-paint-welcome.mp4).
These are real Qt captures of CLI-generated images, edited into staged playback.
Live socket control is not demonstrated. [Reproduce it](../demo-video/README.md).

## Verification and next acceptance

See [VERIFICATION.md](../VERIFICATION.md) for observed checks and outstanding real desktop tests, [ARCHITECTURE.md](../ARCHITECTURE.md) for ownership, and [CREDITS.md](../CREDITS.md) for dependencies and artwork.
