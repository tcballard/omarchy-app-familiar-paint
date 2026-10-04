#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
paint="${PAINT_BIN:-$root/bin/familiar-paint}"
[[ -x "$paint" ]] || paint="$root/target/familiar-paint"
[[ -x "$paint" ]] || { echo 'Set PAINT_BIN to the Paint executable.' >&2; exit 1; }
command -v jq >/dev/null || { echo 'Install jq for this optional demo.' >&2; exit 1; }
[[ -n "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ]] || { echo 'Run inside your desktop session.' >&2; exit 1; }
art="$root/examples/welcome.json"
[[ -f "$art" ]] || art="$root/demo-video/welcome.json"
results="$(mktemp -d "${TMPDIR:-/tmp}/familiar-paint-demo.XXXXXX")"
session="welcome-$$"
"$paint" --listen "$session" > "$results/session.json" 2> "$results/session.log" &
pid=$!
ready=false
for ((i=0;i<50;i++)); do
  if "$paint" --send "$session" --state > "$results/state.json" 2>/dev/null; then ready=true; break; fi
  kill -0 "$pid" 2>/dev/null || break
  sleep 0.1
done
if [[ "$ready" != true ]]; then cat "$results/session.json" "$results/session.log" >&2; exit 1; fi
printf '\nArrange this terminal next to the new Paint window.\nOptional: start scripts/record-demo.sh in another terminal.\nPress Enter to draw the welcome card.\n'
read -r
revision="$(jq -er .revision "$results/state.json")"
index=0
while IFS= read -r command; do
  index=$((index+1))
  batch="$results/$(printf '%03d' "$index").json"
  jq -cn --argjson command "$command" '{version:1,commands:[$command]}' > "$batch"
  printf '\n$ familiar-paint --send %s --apply %s --expect %s\n' "$session" "$batch" "$revision"
  cat "$batch"
  "$paint" --send "$session" --apply "$batch" --expect "$revision" | tee "$results/reply.json"
  revision="$(jq -er '.revision' "$results/reply.json")"
  sleep 0.35
done < <(jq -c '.commands[]' "$art")
"$paint" --send "$session" --export "$results/welcome.png" | tee "$results/export.json"
printf '\nSaved %s/welcome.png\nThe Paint window stays open. Close it when finished.\n' "$results"
printf 'Undo: %q --send %q --undo\n' "$paint" "$session"
