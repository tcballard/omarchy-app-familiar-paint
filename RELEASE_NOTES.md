# Familiar Paint v0.0.1 — preview draft

A familiar canvas for quick sketches, annotations and everyday image edits on
Omarchy. This first preview combines the classic toolbox and colour palette with
a CLI your coding agent can use.

- Draw with brushes, fill, lines, arrows, shapes and raster text.
- Select, crop, resize, rotate and flip; save PNG, JPEG or BMP.
- Use bounded undo/redo and atomic saves with external-change checks.
- Render JSON batches without a window, or enable a local agent session to edit
  the open canvas. Each batch is one undo step; revisions protect live edits.
- Install the prebuilt Linux x86_64 archive with system Qt runtime dependencies.
  No compiler or language toolchain is required on the user's machine.

## Download and install

Planned assets: `familiar-paint-0.0.1-linux-x86_64.tar.gz` and `SHA256SUMS`.
Verify the checksum, extract, and follow the included README. Close Paint before
upgrading; the installer keeps the previous executable for rollback.

## Testing and remaining acceptance

The initial source passed Ubuntu CI, including the live CLI/socket round trip
and desktop-file validation. Every candidate runs those checks plus archive,
install/upgrade/remove and checksum smoke checks. Inspect the candidate's actual
workflow result and BUILD-INFO; this document does not certify a future run.

Real Omarchy/XPS acceptance and continuous desktop demo remain pending. Complete
`docs/XPS-TEST.md` and record the source commit and environment before publishing.
Existing screenshots/video show Qt offscreen captures; the video is staged.

## Preview limits

No autosave/crash recovery, layers, movable pasted objects, print support or
non-destructive text. Eraser paints white. Save regularly. Large drawing batches
may briefly pause the window. Compatibility with the current Arch Qt runtime
must be checked on the target desktop.

This file is a draft for a prerelease, not a publication announcement.
