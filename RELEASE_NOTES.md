# Familiar Paint v0.0.1 — Preview

Classic Paint tooling, with a CLI your coding agent can use. This first preview
brings quick sketches, annotations and everyday image edits to Omarchy.

- Draw with brushes, fill, lines, arrows, shapes and raster text.
- Select, crop, resize, rotate and flip; save PNG, JPEG or BMP.
- Use bounded undo/redo and atomic saves with external-change checks.
- Render JSON batches without a window, or enable a local agent session to paint
  on the open canvas. Each batch is one undo step; revision checks protect live edits.
- Install the prebuilt Linux x86_64 archive with system Qt runtime dependencies.
  No compiler or language toolchain is required.

## Try the preview

Download `familiar-paint-0.0.1-linux-x86_64.tar.gz` and `SHA256SUMS`.
Verify the checksum, extract, and follow the included README or the
[preview installation guide](https://github.com/tcballard/omarchy-app-familiar-paint/blob/v0.0.1/docs/PREVIEW-INSTALL.md).
Close Paint before upgrading; the installer keeps the previous executable for rollback.

## What has been checked

The release preparation passed Ubuntu CI, including the live CLI/socket round trip,
desktop-file validation, archive checksums and packaged install/upgrade/remove smoke checks.
The tagged build repeats these checks and records its source commit in `BUILD-INFO`.

Real Omarchy/XPS acceptance, current Arch Qt runtime compatibility and continuous
Wayland demo recording remain unverified. This is an early preview for testing.
The [XPS checklist](https://github.com/tcballard/omarchy-app-familiar-paint/blob/v0.0.1/docs/XPS-TEST.md)
covers the remaining desktop checks. Existing screenshots use Qt offscreen captures;
the existing welcome video is staged.

## Known limits

No autosave/crash recovery, layers, movable pasted objects, print support or
non-destructive text. Eraser paints white. Save regularly. Large drawing batches
may briefly pause the window.

If something breaks, please include the steps, your Omarchy/Qt versions and the
source commit from `BUILD-INFO` in an issue.
