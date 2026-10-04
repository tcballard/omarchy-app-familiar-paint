#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
[[ -x "$root/bin/familiar-paint" ]] || { echo 'Run this from an extracted binary preview.' >&2; exit 1; }
results="$(mktemp -d)"
trap 'rm -rf -- "$results"' EXIT
(cd "$root" && sha256sum -c FILES.sha256)
"$root/bin/familiar-paint" --version
"$root/bin/familiar-paint" --render "$root/examples/house.json" --output "$results/house.png"
"$root/bin/familiar-paint" --inspect "$results/house.png"
FAMILIAR_INSTALL_PREFIX="$results/install space" bash "$root/scripts/install.sh"
"$results/install space/bin/familiar-paint" --version
FAMILIAR_INSTALL_PREFIX="$results/install space" bash "$root/scripts/install.sh"
cmp "$results/install space/bin/familiar-paint" "$results/install space/bin/familiar-paint.previous"
cp "$results/install space/bin/familiar-paint.previous" "$results/install space/bin/familiar-paint"
"$results/install space/bin/familiar-paint" --version
FAMILIAR_INSTALL_PREFIX="$results/install space" bash "$root/scripts/uninstall.sh"
[[ ! -e "$results/install space/bin/familiar-paint" ]]
[[ -f "$results/house.png" ]]
echo 'Archive, render, inspection, temporary install/upgrade/remove passed. Your installation was not changed.'
