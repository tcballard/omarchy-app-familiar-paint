# Painting with an agent

Familiar Paint exposes its C++ drawing engine through a CLI and an opt-in local
socket. Your agent turns a prompt into JSON drawing commands, runs them, exports
the canvas to inspect the result, and revises it. No model, account, cloud service
or Python runtime is bundled. The GUI and CLI use the same raster primitives.

From the extracted archive directory, open a window with control enabled:

```sh
./bin/familiar-paint --listen studio
```

In another terminal (or through your agent's shell):

```sh
./bin/familiar-paint --send studio --state
./bin/familiar-paint --send studio --apply examples/house.json
./bin/familiar-paint --send studio --export house.png
./bin/familiar-paint --send studio --undo
./bin/familiar-paint --send studio --redo
```

Each successful drawing batch is one undo step, shared with the GUI's Ctrl+Z.
A failed batch leaves the image and history untouched. A batch with no pixel
change creates no undo step. Export writes a copy; it does not mark the open
document saved or change its path. Existing outputs require `--force`.
The status bar shows the session name; **Stop agent** immediately closes access.
Normal GUI startup exposes no endpoint.

## Headless rendering

```sh
./bin/familiar-paint --render examples/house.json --output house.png
./bin/familiar-paint --inspect house.png
./bin/familiar-paint --schema
```

Use `--input original.png` with `--render` to edit an existing image.
Use `--render -` or `--apply -` to read JSON from stdin. Drawing batches contain
only `version` and `commands`. `--schema` prints a compact command reference,
not a formal JSON Schema validator. Images save as PNG, JPEG or BMP.

## Agent workflow

1. Inspect the live state and retain its `revision`.
2. Export a snapshot when you need to see the actual pixels. Its response also
   carries the revision: inspect and export are separate requests.
3. Write a version-1 batch, then apply it with `--expect 'REVISION'` using the
   revision you planned against. A conflict means inspect and reconsider.
4. Export and inspect the result. Adjust with another batch, or undo.

For example, after obtaining the real revision from `--state`:

```sh
./bin/familiar-paint --send studio --apply drawing.json --expect 'REVISION'
```

Without `--expect`, the CLI reads the revision immediately before sending a
mutation. That protects against edits between those two requests, but cannot
protect plans based on an earlier view. Agents should pass `--expect` explicitly.
A revision is an opaque document UUID plus mutation counter. Undo and redo also
advance it. Replacing the GUI document generates a new identity.

## Command format

Coordinates are integer canvas pixels, with origin at the top left. Drawing
outside the canvas is clipped; fill points and crop rectangles must be inside.
Colours accept Qt colour strings such as `#efc95b`, `white`, or `#80ff0000`
(alpha-first hex). Width is 1–80 pixels, default 5; text size is 8–200 pixels,
default 20. Text is rasterised at a top-left anchor; line breaks are supported.
Font availability affects text rendering across machines.

| `op` | Fields beyond `op` |
| --- | --- |
| `new` | `width`, `height`, `color`; first command only |
| `line`, `arrow`, `rect`, `ellipse` | `from:[x,y]`, `to:[x,y]`, `color`, optional `width`, optional `filled` (shapes only) |
| `stroke` | `points:[[x,y],…]`, `color`, optional `width` |
| `fill` | `at:[x,y]`, `color` |
| `text` | `at:[x,y]`, `text`, `color`, optional `size`, optional `font` family |
| `crop` | `x`, `y`, `width`, `height` |
| `resize` | `width`, `height` (stretches) |
| `rotate` | `degrees`: -270, -180, -90, 0, 90, 180 or 270 |
| `flip` | `axis`: `horizontal` or `vertical` |

Live `new` replaces pixels as an undoable edit and **retains the current file
association**. Use GUI Save As for a different file, or export a copy.
Brush erasing can be expressed as a white stroke, matching the GUI's white eraser.
Commands do not select GUI tools or change its brush/colour preferences.

Requests are limited to 1 MiB, 128 commands, 8192 total stroke points and a
256-million-pixel work budget. Canvas dimensions are 1–16384 per side, with at
most 16 million pixels. Unknown fields, fractional coordinates and invalid
colours are errors. Split large work into batches; each becomes its own undo step.
The history budget is 192 MiB. Large batches execute synchronously and can briefly
pause the window.

## Raw protocol and errors

Use `--send studio --request request.json` (or `-` for stdin). A mutation request:

```json
{
  "version": 1,
  "op": "apply",
  "expected_revision": "REPLACE_WITH_CURRENT_REVISION",
  "commands": [
    {"op":"ellipse","from":[20,20],"to":[120,120],"color":"#efc95b","filled":true}
  ]
}
```

Other operations are `inspect`, `snapshot`, `undo` and `redo`. Undo/redo require
`expected_revision`; inspect/snapshot accept only `version` and `op`.
Snapshot returns a full-resolution PNG as `png_base64`; the CLI's `--export`
decodes and writes it client-side. The server accepts no filesystem paths.

Command results are JSON on stdout with `version:1` and `ok`. Errors include an
`error` string and a nonzero exit status. Qt diagnostics go to stderr; `--help`,
`--version` and command-line parsing errors use Qt's normal text output.
Mutations and snapshots reject active mouse drags, modal dialogs and file I/O.
After a timeout/disconnect, inspect before retrying: a mutation may have committed.

Transport is one compact JSON object plus newline per connection, with one JSON
response plus newline. Connections time out after 30 seconds; at most four are
accepted concurrently. Session names contain 1–32 letters, digits, `_` or `-`.
The socket lives in a checked, user-owned private directory under XDG_RUNTIME_DIR,
or `/tmp/familiar-paint-UID` when that variable is unset. A named-session lock
prevents a second instance from taking over. There is no TCP listener. Any process
running as your user can control an enabled session; session names are not secrets.
