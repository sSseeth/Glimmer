# 6. Packaging and distribution

- Status: accepted
- Date: 2026-11-16

## Context

Users need to install Glimmer without a compiler, on Windows, macOS and
Linux, and update it easily. Building from source stays possible but
shouldn't be required.

## Decision

- **One self-contained package per platform**, built in CI with Qt 6.8 (from
  `jurplel/install-qt-action`) and whisper.cpp:
  - Windows: a `.zip` made by `windeployqt`, including the MSVC runtime
    DLLs. SQLite and GoogleTest come from **vcpkg** on the Windows runner,
    because Windows has no system SQLite.
  - macOS (Apple Silicon): a `.dmg` made by `macdeployqt`, ad-hoc signed.
    `Info.plist` declares `NSMicrophoneUsageDescription` for voice entries.
  - Linux (x86_64): an **AppImage** made by `linuxdeploy` and its Qt plugin,
    built on Ubuntu 22.04 so it runs on glibc 2.35 and newer.
- **whisper.cpp is linked statically with portable CPU flags**
  (`GGML_NATIVE=OFF`, `GGML_OPENMP=OFF`, Metal kernels embedded on macOS), so
  release binaries don't depend on the CI runner's CPU or extra runtimes.
- **Install scripts** (`scripts/install.ps1`, `scripts/install.sh`) are the
  recommended install path. They download from GitHub Releases, **verify
  SHA-256 checksums**, install per-user without admin rights, add OS
  integration (Start menu and Apps entry, Applications, desktop entry),
  and uninstall cleanly without touching the diary.
- Packages are built and the installers tested on **every relevant PR**, not
  only at release time.

## Consequences

- New CI-only build tools: vcpkg (Windows), linuxdeploy (Linux, downloaded
  from its "continuous" release channel, which isn't version-pinned; if it
  breaks, pin a specific release URL), and install-qt-action.
- Packages aren't code-signed or notarized yet; users see OS warnings on
  manual downloads. See `docs/releasing.md` for the path to signing.
- No Intel-Mac or ARM-Linux packages yet. Those users build from source.
- The Linux CI and the AppImage use different Qt versions (6.4 from apt
  versus 6.8), which also checks that we stay compatible with both.
