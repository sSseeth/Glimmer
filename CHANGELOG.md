# Changelog

All notable changes are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions
follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Downloadable packages for every release: Windows `.zip`, macOS
  (Apple Silicon) `.dmg` and Linux `.AppImage`, all with voice transcription
  built in.
- One-line installers: `install.ps1` (PowerShell) and `install.sh`
  (macOS/Linux). They verify SHA-256 checksums, install per-user with
  OS integration, update in place and uninstall without touching the diary.
- App icon, Linux desktop entry, Windows executable icon, and a macOS bundle
  with the microphone-permission prompt.
- Installation guide (`docs/installing.md`) and an Install section in the
  README.
- CI builds and tests the core with MSVC on Windows, and builds and
  installs all three packages on pull requests that affect them.

### Changed

- whisper.cpp is linked statically without `-march=native` or OpenMP, so
  release binaries run on any AVX2-capable x86-64 CPU or Apple Silicon.
- The database path is passed to SQLite as UTF-8, fixing diaries in
  folders with non-ASCII names on Windows.

## [0.1.0] - 2026-11-14

### Added

- Calendar that highlights journaled days, tinted by entry type, with tooltips.
- Written, voice and collage entries; search across all entries.
- Voice recording with playback and optional local transcription
  (whisper.cpp, `GLIMMER_WITH_WHISPER=ON`).
- Collage canvas: add or drop images, then move, resize, rotate and layer
  them.
- AI companion panel: weekly and monthly summaries, journaling prompts, and
  questions about your entries, through any OpenAI-compatible gateway
  (OmniRoute by default). History is stored separately from entries.
- SQLite storage with versioned migrations; media library with
  path-traversal protection.
- CI: format, GCC/Clang/Apple Clang builds with sanitizers, clang-tidy,
  CodeQL, app build and smoke test, AI-disclosure and commit-message checks,
  and a tag-driven release workflow.
- Documentation: README, CONTRIBUTING, AI_POLICY, AGENTS, SECURITY,
  architecture and ADRs.

[Unreleased]: https://github.com/sSseeth/Glimmer/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/sSseeth/Glimmer/releases/tag/v0.1.0
