# Familiar Paint preview

Native Paint for Omarchy, version 0.0.2. This is an early preview.
The archive contains a prebuilt Linux x86_64 executable: no compiler, Cargo,
Node.js or Python is needed to install or run Paint. It uses system Qt libraries.

On Omarchy, install runtime dependencies, then from the extracted folder:

```bash
sudo pacman -S --needed qt6-base qt6-wayland
sha256sum -c FILES.sha256
bash scripts/smoke-preview.sh
bash scripts/install.sh
"$HOME/.local/bin/familiar-paint"
```

The smoke test uses a temporary installation and keeps your existing install
untouched. Open Familiar Paint from the launcher after installation.
Before extracting, verify the downloaded archive with `sha256sum -c SHA256SUMS`
from the folder containing the archive and checksum file.

## Update, roll back, remove

Close all Paint windows before installing another build. Re-run install.sh from
the new archive to update; it retains the previous executable at
`~/.local/bin/familiar-paint.previous`. To roll back, close Paint and run:

```bash
cp "$HOME/.local/bin/familiar-paint.previous" "$HOME/.local/bin/familiar-paint"
```

This preview stores no settings or schema requiring migration. Saved images are
ordinary files. Runtime Qt compatibility still applies to the old executable.
Remove the app with `bash scripts/uninstall.sh`; saved images remain.
`FAMILIAR_INSTALL_PREFIX` overrides the default `~/.local` prefix for both scripts.
Do not use these scripts to replace files owned by pacman.

## Test and record

Read `docs/XPS-TEST.md` for desktop acceptance and the live CLI demo. The optional
demo/recording helpers need jq, wf-recorder and slurp; Paint itself does not.
`docs/AGENT-CLI.md` contains the command reference.

No layers, autosave or crash recovery yet: save regularly. Desktop acceptance is
pending; successful CI is not a claim that this build has been tested on Omarchy.
See BUILD-INFO for source identity, RELEASE_NOTES.md for scope and LICENSE for MIT terms.
