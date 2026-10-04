# Architecture

`main.cpp` owns application identity and startup. `Window` owns the active `Document`, UI commands and the one file-operation watcher. `Canvas` owns transient drag previews and selections; it never writes documents to disk. `Document` owns the authoritative raster, immutable implicitly-shared history snapshots, saved-state identity and loaded-file fingerprint. `theme.cpp` adapts a small supported subset of Omarchy's semantic colour output into a Qt palette.

Qt Widgets was chosen for conventional menus, native text input, clipboard, accessibility and the mature QPainter raster engine. There are no language bindings or Python components. The rendering backend is selected by Qt; Wayland uses its platform plugin. No shell singleton imports, global shortcuts, background services or TCP network listeners.

## Agent commands

`drawing.cpp` contains shared QPainter primitives, text and fill. Both `Canvas`
and `commands.cpp` use them. A version-1 batch validates and draws into a detached
image, then commits once. A failing command never changes the active document.
The CLI supports headless rendering, image inspection and local-session requests.

`AgentServer` runs on the GUI thread only when `--listen SESSION` is specified.
It uses QLocalServer inside a checked private user directory, plus a named lock.
It never accepts server-side file paths. Export transfers a full-resolution PNG
to the CLI, which owns the explicit atomic output write. Stop agent closes the
listener and connections; window exit releases the lock. Other same-user
processes are trusted clients; no network authentication claim is made.

Every live pixel mutation requires the document UUID/revision returned by state
inspection. Commit, undo and redo advance it; GUI replacement changes identity.
Drawing requests reject active drags, dialogs and I/O. Batches are synchronous,
bounded by size/count/work limits, and share GUI history. Socket requests and
responses are bounded, with four concurrent clients and a 30-second deadline.
Live `new` is an undoable pixel replacement, retaining the document's save path.
The CLI refuses overwriting exports unless `--force` is supplied.

## Document and worker lifecycle

Starting I/O cancels unfinished canvas input and disables editing. The worker receives image/path/hash values; no widgets or mutable documents are accessed from the worker. Completion is delivered through QFutureWatcher on the UI thread. Because replacement and close are blocked while I/O is pending, results cannot attach to another document. Destruction waits for the worker to finish. Disk operations cannot be cancelled mid-commit; the event loop remains running, but there is no timeout for a stalled filesystem.

Saves use QSaveFile without direct-write fallback and a same-directory QLockFile. A changed loaded-file fingerprint requires Save As to a different path. Fingerprints stream through SHA-256. Open checks the fingerprint before and after decoding. Atomic replacement protects an existing file against encoding/commit failure, not against every power-loss or concurrent external-write scenario.

New/Open/Paste/Close resolve dirty state with Save/Discard/Cancel. Cancelling the following file dialog leaves the document intact. Drag previews commit only on release; Escape, focus loss and window deactivation discard them. Undo includes crop, resize, raster text and fill; saved-state IDs remain correct when a history branch is discarded. Snapshot storage is capped at 192 MiB; each raster is at most 64 MB.

## Storage and recovery

Only user-selected image files persist. Preferences and crash recovery are explicitly not implemented in this preview. A short-lived lock alongside a destination coordinates Paint instances; each process can own its own independent document. PNG saves retain transparency; JPEG/BMP use a white composite. No metadata preservation or colour-managed editing claim is made.

## Theme boundary

Inspected Omarchy `themes/tokyo-night/colors.toml`, blob `58e6785259b83a72a5921e2cfec4c66192860b7e`, on 3 October 2026. The adapter reads valid quoted six-digit hex assignments for four semantic colour keys; it is deliberately not a general TOML parser. Absolute XDG_CONFIG_HOME or ~/.config is used. Re-reading the path on each timer tick handles file and symlink replacement. Missing/malformed values fall back to the built-in palette. The palette never recolours the user's raster.
