# Instructions for AI coding agents

This file is for AI tools working in this repository (Claude Code, Copilot,
Cursor, Codex and others). Humans should read [CONTRIBUTING.md](CONTRIBUTING.md)
and [AI_POLICY.md](AI_POLICY.md).

## Build and test

```sh
cmake --preset core && cmake --build --preset core && ctest --preset core   # no Qt needed
cmake --preset dev  && cmake --build --preset dev  && ctest --preset dev    # full app
scripts/format.sh                                                          # clang-format
```

Run the tests and `scripts/format.sh` before saying a change is done. Report
honestly if something didn't build or a test failed. Never say it passed if
you didn't run it.

## Architecture rules

- `core/` is pure C++20 and **must not include Qt**. Anything that can be
  tested without a GUI belongs there.
- All SQL lives in the `*_repository.cpp` files. Schema changes are **new**
  migrations in `core/src/migrations.cpp`; never edit a shipped one.
- AI output is stored in `ai_notes` and shown in the AI panel. It is never
  written into a user's entry unless the user explicitly inserts it.
- Media paths in the database are relative to the media library root; always
  go through `MediaLibrary::resolve()`.
- Add a test for every behaviour change in `core/`.

## Commits and pull requests

- Use Conventional Commits: `type(scope): summary` (see `.gitmessage`).
- Add `Assisted-by: <tool>:<model>` to every commit containing content you
  generated.
- **Never** add `Co-authored-by:` or `Signed-off-by:` for yourself, and never
  set yourself as the commit author. The human you're working for is the
  author.
- Keep your generated changes in commits separate from the human's own
  edits.
- In PR descriptions, fill in the "AI assistance" section truthfully.
- Never push to `main`, force-push shared branches, merge PRs, or create
  tags or releases.

## Things to ask the human about first

- Adding a dependency (see `docs/adr/0003-dependencies.md`).
- Disabling, skipping or loosening any test, warning, sanitizer,
  clang-tidy check or CI job. Don't do this to make a build pass.
- Anything that sends data off the machine.

## Privacy

Never read, print or upload files from a real diary data directory
(`~/.local/share/glimmer`, `%APPDATA%/glimmer`,
`~/Library/Application Support/glimmer`). Use `--data-dir` with a temporary
folder for manual testing.
