# Releasing

Glimmer uses [Semantic Versioning](https://semver.org/): `MAJOR.MINOR.PATCH`.
Before 1.0, minor versions may change things.

## What a release produces

`.github/workflows/release.yml` builds, tests and packages:

| File | Built on | How |
|---|---|---|
| `glimmer-vX.Y.Z-windows-x64.zip` | windows-2022, MSVC | `windeployqt` + MSVC runtime + `install.ps1` |
| `glimmer-vX.Y.Z-macos-arm64.dmg` | macos-15 | `macdeployqt`, ad-hoc signed, `hdiutil` |
| `glimmer-vX.Y.Z-linux-x86_64.AppImage` | ubuntu-22.04 | `linuxdeploy` + Qt plugin |
| `SHA256SUMS` | | checksums of the three packages |

All three use Qt 6.8 and include whisper.cpp transcription. Each job
installs its package with the real install script, smoke-tests it and
uninstalls it again.

The same workflow runs on any **pull request** that touches the app, core,
build system or installers, so packaging breakage is caught before a
release. Those builds are kept as workflow artifacts for 14 days (look
under the PR's checks → "Package & release" → Artifacts) and are never
published.

## Cutting a release

1. Make sure `main` is green.
2. Open a PR `chore(release): vX.Y.Z` that:
   - bumps `VERSION` in the `project()` call in `CMakeLists.txt`;
   - in `CHANGELOG.md`, renames `## [Unreleased]` to
     `## [X.Y.Z] - YYYY-MM-DD`, adds a fresh empty `## [Unreleased]`, and
     updates the compare links at the bottom.
3. Merge it once the package jobs pass on the PR.
4. Tag the merge commit and push the tag:
   ```sh
   git switch main && git pull
   git tag -a vX.Y.Z -m "Glimmer vX.Y.Z"
   git push origin vX.Y.Z
   ```
5. The workflow checks the tag matches `CMakeLists.txt`, builds all three
   packages, and publishes a GitHub Release. The notes are that version's
   CHANGELOG section plus install instructions.
6. Check the release page, then run the one-line installer on at least one
   machine as a final check.

Tagging is a maintainer decision. AI agents working in this repo must not
create tags or releases (see `AGENTS.md`).

## Code signing (not yet set up)

The packages are **not signed** by a paid certificate, so:

- **Windows** SmartScreen may warn on a manual download ("More info → Run
  anyway"). The PowerShell installer avoids most of this because files
  downloaded with `Invoke-WebRequest` aren't marked as coming from the
  internet.
- **macOS** builds are ad-hoc signed (needed to run on Apple Silicon) but not
  notarized, so Gatekeeper warns on first open. `install.sh` clears the
  quarantine flag.

To sign properly later:

- **macOS:** an Apple Developer ID ($99/year). Add the certificate and
  notarization credentials as repository secrets, replace
  `codesign --sign -` with your Developer ID and `--options runtime`, then run
  `xcrun notarytool submit --wait` and `xcrun stapler staple` on the dmg.
- **Windows:** an OV/EV code-signing certificate or
  [Azure Trusted Signing](https://learn.microsoft.com/azure/trusted-signing/).
  Sign `glimmer.exe` before zipping.

Record that change in a new ADR when you do it.
