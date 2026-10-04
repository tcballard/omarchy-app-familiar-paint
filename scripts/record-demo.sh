#!/usr/bin/env bash
set -euo pipefail
for cmd in wf-recorder slurp; do command -v "$cmd" >/dev/null || { echo "Install $cmd for optional Wayland recording." >&2; exit 1; }; done
[[ -n "${WAYLAND_DISPLAY:-}" ]] || { echo 'Run inside the Wayland desktop.' >&2; exit 1; }
output="${1:-$PWD/familiar-paint-live-$(date +%Y%m%d-%H%M%S).mp4}"
[[ ! -e "$output" ]] || { echo 'Output already exists.' >&2; exit 1; }
printf 'Select only Paint and its terminal. Recording starts after selection.\nPress Ctrl+C here to stop and finalise the silent MP4.\n'
geometry="$(slurp)"
[[ -n "$geometry" ]] || exit 1
exec wf-recorder -g "$geometry" -c libx264 -r 30 -f "$output"
