# Verification — 4 October 2026

Input identity: `EVIDENCE.sha256` hashes the repository import, docs and demo.
The tested binary is distributed in the separate preview archive; repository CI
builds a fresh executable from source. No upstream implementation was copied. This is a development
preview, not a production release or live Omarchy acceptance.

## Agent-control revision

Added shared C++ drawing primitives, versioned JSON batches, headless rendering,
image inspection, opt-in QLocalServer control, revision-checked live mutations,
atomic batch undo, snapshot export and a Stop agent control. The classic UI
layout remains. No language model or Python component is bundled.

Environment: Linux x86_64, Ubuntu 24.04, GCC 13.3, CMake 3.28.3, Qt 6.4.2.
Qt/CMake packages were downloaded into a private temporary toolchain because
system package installation was unavailable. The build used CMAKE_PREFIX_PATH
and explicit OpenGL runtime paths; tests used that toolchain's LD_LIBRARY_PATH
and QT_PLUGIN_PATH. The installed binary has no temporary build RPATH.

| Check | Observed result |
| --- | --- |
| CMake Release build, app/tests/preview | Exit 0 |
| Native Qt tests with `PAINT_TEST_NO_LOCAL_SOCKETS=1` | 25 passes, 0 failures, 1 explicit skip; see `docs/agent-test-results.txt` |
| CLI render plus inspect, overwrite refusal | Passed within the native suite before its live-transport skip |
| Sample `--render examples/house.json --output docs/agent-house.png` | Exit 0; 800×520 image visually inspected |
| Atomic batch rollback, one-step undo/redo, stale-revision refusal | Passed against the real Window/Document command handler |
| Snapshot encode/decode | Pixel-equal to the live handler's canvas |
| GUI arrow and CLI arrow | Pixel-equal using shared drawing primitives |
| Existing GUI/document tests | Passed, including asynchronous save/conflict handling and theme replacement |
| Binary dependency and RPATH inspection | Qt/system libraries only; no Python or temporary RPATH |

**Live socket transport is not verified here.** An attempted `--listen` failed
with `QLocalServer::listen: Unknown error 1`; this managed environment disallows
local Unix socket listeners. The first unskipped suite consequently failed its
live round trip. The explicit test flag skips only the transport portion, after
headless CLI checks. The default test run and supplied CI workflow do not set
that flag and require the real socket round trip to succeed.

On an unrestricted Linux desktop, run:

```sh
cmake -S . -B target -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build target --parallel 2
ctest --test-dir target --output-on-failure
```

The live test covers CLI state/apply/export, pixel equality with headless render,
stale revisions, undo/redo, and duplicate-session refusal. It remains an
acceptance gate, not a claimed pass. Manually check the Stop agent button too.

## Retained evidence from 3 October

The classic-layout app was captured at 1280×900 and 800×580 using Qt's offscreen
backend. `docs/preview.png` is that actual app capture, not an Omarchy desktop
screenshot. Its sample was drawn through GUI tools. The installer/uninstaller
were syntax-checked and run against a temporary prefix containing spaces. These
scripts and desktop integration were not changed in this revision. Omarchy's
palette contract was inspected at blob
`58e6785259b83a72a5921e2cfec4c66192860b7e`.

## Outstanding desktop acceptance

- Real Omarchy/Hyprland launch, app_id/icon, Super+W and launcher file opening.
- Live local socket round trip, Stop agent, and simultaneous human/agent editing.
- Wayland clipboard exchange, native dialogs, IME and accessibility review.
- Fractional scaling, multiple monitors and real Omarchy theme switching.
- Binary compatibility with current Arch Qt, package installation and removal.
- `desktop-file-validate` and GitHub CI execution.

Offscreen clipboard tests cover only Qt's in-process clipboard. Optional XKB
headers were absent; the offscreen backend reports its expected lack of
propagateSizeHints support.

## Known limitations

No crash recovery/autosave, layers, movable pasted objects, print support or
non-destructive text. File I/O has no stalled-filesystem timeout. Large canvas
operations and agent batches are synchronous and may briefly pause interaction.
Other editors can race a final fingerprint check because they do not honour
Paint's save lock. An agent timeout does not prove its mutation was rolled back;
inspect the revision before retrying.
