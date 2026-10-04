#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build="${1:-$root/target}"
output="${2:-$root/dist}"
[[ "$(uname -m)" == x86_64 ]] || { echo 'This preview package targets x86_64.' >&2; exit 1; }
version="$(sed -n 's/^project(FamiliarPaint VERSION \([^ ]*\).*/\1/p' "$root/CMakeLists.txt")"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || exit 1
[[ -x "$build/familiar-paint" ]] || { echo 'Build familiar-paint first.' >&2; exit 1; }
[[ "$("$build/familiar-paint" --version)" == "Familiar Paint $version" ]] || { echo "Binary version mismatch." >&2; exit 1; }
commit="${SOURCE_COMMIT:-working-tree}"
if [[ "${GITHUB_REF:-}" == refs/tags/* && "${GITHUB_REF_NAME:-}" != "v$version" ]]; then
  echo 'Tag and compiled project version differ.' >&2; exit 1
fi
mkdir -p "$output"
output="$(cd "$output" && pwd)"
name="familiar-paint-$version-linux-x86_64"
[[ ! -e "$output/$name.tar.gz" ]] || { echo 'Output exists; use a fresh directory.' >&2; exit 1; }
stage="$(mktemp -d)"
trap 'rm -rf -- "$stage"' EXIT
cmake --install "$build" --prefix "$stage/install"
mkdir -p "$stage/$name"/{bin,packaging,scripts,examples,docs}
package="$stage/$name"
install -m 755 "$stage/install/bin/familiar-paint" "$package/bin/"
cp "$root"/packaging/* "$package/packaging/"
cp "$root"/scripts/{install,uninstall,live-demo,record-demo,smoke-preview}.sh "$package/scripts/"
cp "$root"/examples/*.json "$package/examples/"
cp "$root/demo-video/welcome.json" "$package/examples/welcome.json"
cp "$root/docs/"{AGENT-CLI,XPS-TEST,PREVIEW-INSTALL}.md "$package/docs/"
cp "$root/docs/PREVIEW-INSTALL.md" "$package/README.md"
cp "$root/"{LICENSE,CREDITS.md,RELEASE_NOTES.md} "$package/"
printf 'version=%s\nsource_commit=%s\narchitecture=x86_64\n' "$version" "$commit" > "$package/BUILD-INFO"
(cd "$package" && find . -type f ! -name FILES.sha256 -print0 | sort -z | xargs -0 sha256sum > FILES.sha256)
epoch="${SOURCE_DATE_EPOCH:-0}"
[[ "$epoch" =~ ^[0-9]+$ ]] || exit 1
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -C "$stage" -cf - "$name" | gzip -n > "$output/$name.tar.gz"
(cd "$output" && sha256sum "$name.tar.gz" > SHA256SUMS)
printf '%s\n' "$output/$name.tar.gz"
