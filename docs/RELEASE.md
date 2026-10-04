# Preview release procedure

## Prepared by CI

Every push/PR builds and runs the native tests, including live socket control,
validates the desktop entry, stages a binary archive and checks checksums plus
headless render and temporary install/upgrade/uninstall. It uploads
`familiar-paint-preview-linux-x86_64`, containing the tarball, SHA256SUMS and draft
notes. These are candidates, not published releases.

The archive includes BUILD-INFO with the checked-out source commit, a per-file
checksum manifest, the launcher/icon, install/remove scripts, drawing examples,
acceptance instructions and optional live-demo/recording helpers. Packaging uses
sorted files, fixed ownership and source-commit timestamps for reproducible
archive metadata; this is not a claim of bit-identical compiler output.

## Before tagging

1. Review and merge the preparation PR after CI passes.
2. Download the main-branch candidate and verify SHA256SUMS. Keep BUILD-INFO.
3. Complete docs/XPS-TEST.md against that candidate on the XPS, or explicitly retain pending desktop acceptance for an authorised early preview.
4. Fix any failures through reviewed changes and test the resulting candidate.
5. Add the real screenshot/demo, exact tested environment and honest acceptance
   results. Update RELEASE_NOTES.md, README and VERIFICATION.md; preserve the
   historical local/offscreen evidence as historical.
6. Verify the final source and notes, and obtain the requested publication go-ahead.

## Draft then publish

The version is 0.0.2. Once the final source is approved, create tag `v0.0.2` at
that reviewed commit. The workflow rejects a mismatch with the CMake version,
builds/tests/packages again, and creates an **unpublished prerelease draft** with
the two downloadable assets. It does not automatically publish a release.

Inspect the draft, checksums, BUILD-INFO and final CI result. For desktop-validated releases, test the tagged
archive's install/launch on the XPS, since it was rebuilt. Early previews must
keep any pending desktop checks explicit in the notes.
Publish the draft only when explicitly authorised; keep it marked prerelease.
If a draft/tag already exists, stop and inspect it rather than replacing it.

When publishing through the browser, attach the verified main-commit archive
and checksums and select that exact commit as the new tag target. GitHub creates
the tag on publication. The tag workflow reruns the checks and preserves an
existing release and its reviewed assets.

Omarchy packaging submission follows separately after desktop acceptance.
