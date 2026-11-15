# glimmer

[![CI](https://github.com/sSseeth/Glimmer/actions/workflows/ci.yml/badge.svg)](https://github.com/sSseeth/Glimmer/actions/workflows/ci.yml)
[![CodeQL](https://github.com/sSseeth/Glimmer/actions/workflows/codeql.yml/badge.svg)](https://github.com/sSseeth/Glimmer/actions/workflows/codeql.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A small, private desktop diary written in C++20 and Qt 6.

- **Calendar.** Every day you journaled is highlighted, tinted by what kind of
  entry it holds. Hover a day to see a summary, click it to see its entries.
- **Three ways to journal:**
  - ✎ **Write.** Plain text; Markdown is stored as you type it.
  - ♪ **Voice.** Record from your microphone, play it back, and transcribe it
    **locally** with [whisper.cpp](https://github.com/ggml-org/whisper.cpp).
    No audio leaves your machine.
  - ▦ **Collage.** Drop images onto a canvas, then drag, resize, rotate and
    layer them.
- **AI companion** in its own side panel: summarise a week or month, get a
  journaling prompt, or ask questions about your entries. It talks to any
  OpenAI-compatible gateway; [OmniRoute](https://github.com/diegosouzapw/OmniRoute)
  is the default. AI output is stored separately and never mixed into what
  you wrote.
- **Search** across every entry.
- **Your data stays local:** one SQLite file plus a media folder, so backing
  up means copying one directory.

## Install

**Windows** (PowerShell):

```powershell
irm https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.ps1 | iex
```

**macOS** (Apple Silicon) and **Linux** (x86_64):

```sh
curl -fsSL https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.sh | bash
```

The installers fetch the latest release, verify its SHA-256 checksum, and
install it for your user only (no admin rights needed), with a Start menu,
Launchpad or app-menu entry. Run them again to update. To uninstall, use
Settings → Apps on Windows, or add `-s -- --uninstall` to the bash command.
Your diary is never deleted.

Prefer a manual download, want to read the script first, or need
troubleshooting? See the **[installation guide](docs/installing.md)**.

| Platform | Package | Needs |
|---|---|---|
| Windows | `.zip` | Windows 10/11, 64-bit |
| macOS | `.dmg` | macOS 13.3+, Apple Silicon |
| Linux | `.AppImage` | x86-64, glibc 2.35+ (Ubuntu 22.04+, Debian 12+, Fedora 36+) |

## Build from source

You need CMake ≥ 3.25, Ninja, a C++20 compiler (GCC 13, Clang 16+ or a
recent Apple Clang), SQLite 3 and Qt ≥ 6.4 (Widgets, Multimedia, Network,
Concurrent).

```sh
# Ubuntu 24.04
sudo apt install build-essential cmake ninja-build libsqlite3-dev libgtest-dev \
                 qt6-base-dev qt6-multimedia-dev

# macOS
brew install cmake ninja qt googletest

cmake --preset dev            # Debug + AddressSanitizer/UBSan
cmake --build --preset dev
ctest --preset dev
./build/dev/app/glimmer
```

Other presets: `core` (library and tests only, no Qt), `dev-whisper` (adds
transcription), `release`. See [docs/development.md](docs/development.md).

### Voice transcription

The prebuilt downloads include transcription. In a source build it's
optional, because whisper.cpp takes a few minutes to compile:

```sh
cmake --preset dev-whisper && cmake --build --preset dev-whisper
```

Either way, download a speech model once, e.g.
[ggml-base.en.bin](https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin)
(about 150 MB), and choose it under **File → Settings → Whisper model**.

## Optional setup

### AI companion (OmniRoute)

1. Install and start OmniRoute (`npm i -g omniroute && omniroute`). It
   listens on `http://localhost:20128/v1`.
2. In OmniRoute's dashboard, connect a provider and create an endpoint key.
3. In glimmer, open **File → Settings** and fill in the gateway URL, the
   model (`auto` lets OmniRoute choose) and the key. You can also set the
   key in the `GLIMMER_AI_API_KEY` environment variable so it's never stored
   on disk.

Nothing is sent until you press a button in the AI panel, and then only the
entries in the period you asked about.

## Where your diary lives

| OS | Default folder |
|---|---|
| Linux | `~/.local/share/glimmer/glimmer/` |
| macOS | `~/Library/Application Support/glimmer/glimmer/` |
| Windows | `%APPDATA%\glimmer\glimmer\` |

Uninstalling never touches this folder. Start with `--data-dir <folder>` to use a different one, e.g. a throwaway
diary for testing.

## Project layout

```
core/      Pure C++20 library: entries, SQLite, JSON, AI prompts, audio. No Qt.
app/       Qt 6 Widgets desktop app built on core/.
tests/     GoogleTest unit tests for core/.
docs/      Architecture, development guide, ADRs, release process.
scripts/   Formatting, git hooks, PR policy checker.
.github/   CI/CD workflows, PR and issue templates.
```

Read [docs/architecture.md](docs/architecture.md) for how the pieces fit together.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). In short: branch, make Conventional
Commits, open a PR, fill in the template, and keep CI green.

### AI-assisted development

This project has an explicit [AI policy](AI_POLICY.md): AI help is welcome,
but it must be disclosed with `Assisted-by:` commit trailers and the PR
template, a human must review every line, and an AI is never listed as an
author. CI enforces this. The initial codebase was generated with Claude and
is labelled that way in the git history.

## License

[MIT](LICENSE)
