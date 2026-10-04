#!/usr/bin/env bash
set -euo pipefail
prefix="${FAMILIAR_INSTALL_PREFIX:-$HOME/.local}"
[[ "$prefix" == /* ]] || { echo 'Install prefix must be absolute.' >&2; exit 1; }
rm -f -- "$prefix/bin/familiar-paint" "$prefix/bin/familiar-paint.previous" \
  "$prefix/share/applications/io.github.tcballard.FamiliarPaint.desktop" \
  "$prefix/share/icons/hicolor/scalable/apps/io.github.tcballard.FamiliarPaint.svg"
if command -v update-desktop-database >/dev/null; then update-desktop-database "$prefix/share/applications"; fi
echo 'Removed Familiar Paint. Your images have not been touched.'
