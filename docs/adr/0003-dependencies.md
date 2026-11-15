# 3. Dependency policy

- Status: accepted
- Date: 2026-11-14

## Context

Every dependency is code we ship but didn't review, and something that can
break the build.

## Decision

- Required: **Qt 6** (app only) and **SQLite 3**.
- Tests only: **GoogleTest**.
- Optional: **whisper.cpp** (`GLIMMER_WITH_WHISPER`, off by default) for
  local transcription.
- Use the system package if it exists; otherwise fall back to
  `FetchContent` with a **pinned version**. All pins live in
  `cmake/Dependencies.cmake`, so upgrades are one reviewable diff.
- Fetched dependencies are marked `SYSTEM`, so our `-Werror` doesn't apply
  to their headers.
- A new dependency needs an ADR explaining why it's worth it.

## Consequences

Builds are reproducible and offline-friendly when system packages are
installed. Upgrading a pin is an explicit PR.
