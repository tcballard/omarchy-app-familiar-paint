# Familiar Paint v0.0.2 — Preview

The same familiar canvas, with less getting in the way.

This preview keeps the classic toolbox and colour palette, and makes the everyday controls easier to read and use.

- **Know what is selected.** A stronger marker identifies the active tool, and a tick marks the current palette colour. Keyboard focus has its own treatment.
- **See the options you need.** Text size appears for Text; shape fill appears for Rectangle and Ellipse. The toolbar drops its repeated app title.
- **Keep drawing in a smaller window.** At 800×600, the toolbox becomes compact so every tool and the stroke-width control stay visible. The stroke sample and shortcut hint return when there is room.
- **Keep the old-school feel.** Square buttons, bevelled edges and the bottom palette stay, with tighter spacing and a refreshed app screenshot.

## Try it

Download `familiar-paint-0.0.2-linux-x86_64.tar.gz` and `SHA256SUMS`, verify the checksum, extract, and follow the included README or [installation guide](https://github.com/tcballard/omarchy-app-familiar-paint/blob/v0.0.2/docs/PREVIEW-INSTALL.md). Close Paint before upgrading. The installer retains the previous executable for rollback; saved images stay in place. Only the system Qt runtime is required.

The drawing CLI and its version-1 JSON protocol remain compatible with v0.0.1.

## Checks and preview limits

The UI was built and inspected through Qt's offscreen renderer at 1280×900, 800×600, and 1000×720 with 150% scaling. Automated checks exercise tool-option visibility, compact stroke-width access and Fit. CI also checks the native suite, live CLI/socket round trip, desktop entry and packaged install/upgrade/remove flow.

Real Omarchy/XPS acceptance and current Arch Qt runtime compatibility remain pending. Screenshots are offscreen captures, and the existing welcome video is staged. This release is still a preview.

No autosave/crash recovery, layers, movable pasted objects or non-destructive text yet. Eraser paints white. Save regularly.

If the compact layout gets in your way, include your window size and display scale with the issue.
