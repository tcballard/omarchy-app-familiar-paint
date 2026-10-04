# XPS acceptance and live demo

Use an extracted binary preview. Keep the folder's BUILD-INFO with your results.
Record `omarchy-version`, `hyprctl version`, `uname -m` and `pacman -Q qt6-base qt6-wayland`.

## Acceptance checklist

- [ ] Smoke-preview script passes; install needs only system runtime libraries.
- [ ] Launcher name and icon appear; window opens and Super+W closes it.
- [ ] Familiar theme is readable; switching theme updates controls, not artwork.
- [ ] Brush, fill, eraser, shapes, text and eyedropper behave as expected.
- [ ] Undo/redo restore a drawing, crop and resize correctly.
- [ ] Save PNG and JPEG, reopen and compare; paths containing spaces work.
- [ ] Closing/replacing unsaved work offers Save / Discard / Cancel.
- [ ] Clipboard copy and paste work with another desktop app.
- [ ] Window works at your normal scaling and on each monitor you use.
- [ ] Live demo paints onto the same open canvas; export matches it.
- [ ] Edit with the mouse during the demo: it stops on a stale revision instead
      of silently continuing against a changed canvas. Run a fresh demo afterwards.
- [ ] Stop agent blocks further CLI requests; ordinary drawing still works.
- [ ] Upgrade retains previous binary; rollback launches; uninstall keeps images.

Do not check a box until observed. Record failures, stderr and the precise action.
Save real XPS screenshots separately from the existing offscreen preview.

## Record commands painting in real time

Optional demo tools (not required by Paint):

```bash
sudo pacman -S --needed jq wf-recorder slurp
bash scripts/live-demo.sh
```

A new, uniquely named Paint session opens. Arrange it alongside the terminal.
The script waits for Enter before painting. In another terminal, start:

```bash
bash scripts/record-demo.sh "$HOME/Videos/familiar-paint-live.mp4"
```

Ensure the output directory exists. Select just Paint and the drawing terminal,
then press Enter in the drawing terminal. Commands are sent separately, with
0.35 seconds between them for readability; the recording is continuous and silent.
Shapes appear as individual operations; freehand stroke segments within one
command are rendered together. This is not animated interpolation of a batch.

After the export, hold for two seconds and press Ctrl+C in the recording terminal
to finalise the MP4. The script prints the exported image's temporary path.
Copy the MP4, PNG, BUILD-INFO and test results somewhere permanent. Paint remains
open for manual testing. Close the demo window when finished.

The recorder captures only the region you select, without audio. Arrange your
windows and clear unrelated notifications before selecting. Capture commands and
results honestly; do not present the deliberate pacing as drawing latency.
