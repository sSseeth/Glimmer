# 2. C++20, Qt 6 Widgets and SQLite

- Status: accepted
- Date: 2026-11-14

## Context

glimmer is a desktop diary with a calendar, rich editors (text, audio,
image canvas) and local storage. The maintainer wants to learn modern C++.

## Decision

- **C++20**, for `std::chrono` calendar types, `std::span`, concepts and
  ranges. Supported by GCC 13, Clang 16+ and current Apple Clang, which
  covers every CI runner.
- **Qt 6 Widgets** (≥ 6.4, the version Ubuntu 24.04 ships). It provides
  `QCalendarWidget`, QtMultimedia for recording and playback, and
  `QGraphicsScene` for the collage canvas. We chose Widgets over QML to keep
  the UI in C++.
- **SQLite** through its C API, wrapped in our own small RAII layer
  (`core/database.hpp`). The whole diary is one portable file; WAL mode and
  transactions make it crash-safe.
- **Two layers:** `core/` (no Qt, fully unit-tested) and `app/` (Qt).

## Consequences

- Core tests run in seconds under sanitizers with no display or Qt install.
- Qt is LGPL. We link it dynamically, which is compatible with our MIT
  licence.
- Writing our own SQLite wrapper is deliberate: it's about 250 lines, and
  RAII and move semantics are worth learning on.
