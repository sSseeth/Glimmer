# Releasing

glimmer uses [Semantic Versioning](https://semver.org/): `MAJOR.MINOR.PATCH`.
Before 1.0, minor versions may change things.

1. Make sure `main` is green.
2. Bump `VERSION` in the `project()` call in `CMakeLists.txt`.
3. In `CHANGELOG.md`, rename `## [Unreleased]` to `## [X.Y.Z] - YYYY-MM-DD`
   and add a fresh empty `## [Unreleased]` above it.
4. Commit through a PR: `chore(release): vX.Y.Z`.
5. After merging, tag and push:
   ```sh
   git switch main && git pull
   git tag -a vX.Y.Z -m "glimmer vX.Y.Z"
   git push origin vX.Y.Z
   ```
6. `.github/workflows/release.yml` checks the tag matches `CMakeLists.txt`,
   builds and tests, packages a Linux tarball with SHA-256 checksums, and
   publishes a GitHub Release using that version's CHANGELOG section as
   the notes.

The Linux tarball links against the system Qt 6 (install `qt6-base` and
`qt6-multimedia` from your distro). Self-contained AppImage, macOS and
Windows packages are on the roadmap.
