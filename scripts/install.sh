#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
prefix="${FAMILIAR_INSTALL_PREFIX:-$HOME/.local}"
[[ "$prefix" == /* ]] || { echo 'Install prefix must be absolute.' >&2; exit 1; }
[[ -x "$root/bin/familiar-paint" ]] || { echo 'Use the binary preview archive, or build from source using the README.' >&2; exit 1; }
if ldd "$root/bin/familiar-paint" | grep -q 'not found'; then
  echo 'Missing runtime libraries. On Omarchy: sudo pacman -S --needed qt6-base qt6-wayland' >&2
  exit 1
fi
install -d "$prefix/bin" "$prefix/share/applications" "$prefix/share/icons/hicolor/scalable/apps"
if [[ -f "$prefix/bin/familiar-paint" ]]; then
  cp -p "$prefix/bin/familiar-paint" "$prefix/bin/familiar-paint.previous"
fi
install -m 755 "$root/bin/familiar-paint" "$prefix/bin/familiar-paint.new"
mv -f "$prefix/bin/familiar-paint.new" "$prefix/bin/familiar-paint"
install -m 644 "$root/packaging/io.github.tcballard.FamiliarPaint.svg" "$prefix/share/icons/hicolor/scalable/apps/"
# Desktop Exec quoting is separate from shell quoting. Escape reserved characters.
exec_path="$prefix/bin/familiar-paint"
exec_path="${exec_path//\\/\\\\\\\\}"
exec_path="${exec_path//\"/\\\\\"}"
exec_path="${exec_path//\$/\\\\\$}"
exec_path="${exec_path//\`/\\\\\`}"
exec_path="${exec_path//%/%%}"
while IFS= read -r line; do
  if [[ "$line" == Exec=* ]]; then printf 'Exec="%s" %%f\n' "$exec_path"; else printf '%s\n' "$line"; fi
done < "$root/packaging/io.github.tcballard.FamiliarPaint.desktop" > "$prefix/share/applications/io.github.tcballard.FamiliarPaint.desktop"
if command -v update-desktop-database >/dev/null; then update-desktop-database "$prefix/share/applications"; fi
printf 'Installed Familiar Paint. Launch it from your app launcher or run: %s/bin/familiar-paint\n' "$prefix"
