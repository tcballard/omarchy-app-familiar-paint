# Demo QA

Verified: H.264, 1280×1024, 30 fps, 690 frames, exactly 23 seconds, 418248 bytes.
Intentionally no audio stream. Full decode to null succeeded.

Inspected the final source capture, 10-second encoded frame and an eight-frame
contact sheet covering the opening, each step and ending. Artwork and command
captions are legible without clipping. The entire application surface is kept;
editorial titles and CLI captions sit outside it. Input is deterministic artwork;
no personal information or unrelated applications appear in the captures.

Claims ledger:

| Claim | Evidence | Status |
| --- | --- | --- |
| Drawn through the CLI | batches, logs/manifest.json and PNG outputs | Verified |
| Six batches produce the final card | logs/pixel-equality.txt compares decoded pixels against one complete batch | Verified |
| Real Paint window | capture.cpp calls Window::open and Window::capture | Verified, offscreen |
| Commands succeeded | Per-stage JSON stdout contains ok=true and expected dimensions | Verified |
| Live agent control | Not shown; listener restriction disclosed | Unverified, excluded |

This cut is an edited sequence of actual application captures. It is not
continuous screen footage. The constant footer explicitly identifies staged
playback and excludes live socket control. Visual review used extracted frames;
no human playback review is claimed. Review on your intended social surface
before publication. No publication was performed.
