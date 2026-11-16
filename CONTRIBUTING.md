# Contributing

Thanks for helping. This guide covers the workflow from first clone to merge.

## One-time setup

```sh
git clone https://github.com/sSseeth/Glimmer.git && cd Glimmer
scripts/install-hooks.sh     # commit-msg checks + commit template
cmake --preset dev           # also writes build/dev/compile_commands.json
ln -sf build/dev/compile_commands.json .   # lets clangd find it
```

Editor setup (including Neovim) is in [docs/development.md](docs/development.md).

## Workflow

1. **Open or pick an issue** so work isn't duplicated. Small fixes can skip this.
2. **Branch from `main`** with a descriptive name: `feat/collage-snap`,
   `fix/voice-empty-recording`, `docs/readme-build`.
3. **Make small commits** using [Conventional Commits](https://www.conventionalcommits.org/):
   ```
   feat(collage): snap images to a 20px grid while dragging
   fix(db): roll back collage writes when an insert fails
   ```
   Types: `feat fix docs style refactor perf test build ci chore revert`.
4. **Disclose AI help** as described in [AI_POLICY.md](AI_POLICY.md): add an
   `Assisted-by:` trailer to commits containing AI-generated content, and
   keep those changes in their own commits.
5. **Run the checks locally** before pushing:
   ```sh
   scripts/format.sh
   cmake --build --preset dev && ctest --preset dev
   ```
6. **Open a pull request** and fill in every section of the template,
   including **AI assistance**. CI checks it.
7. **Respond to review.** Push fixup commits while the review is going on,
   then tidy up with `git rebase -i` before merging if you like.
8. **Merge** with *Rebase and merge* once CI is green and the review is
   approved, so per-commit `Assisted-by:` trailers survive.

## What CI checks

| Job | What it does |
|---|---|
| `clang-format` | every `.cpp`/`.hpp` is formatted |
| `core` | builds and tests core with GCC, Clang and Apple Clang under ASan + UBSan, and with MSVC on Windows; warnings as errors |
| `clang-tidy` | static analysis of `core/` |
| `app` | builds the Qt app (with and without whisper.cpp), runs tests, smoke-tests the binary |
| `CodeQL` | security analysis, plus a weekly scheduled scan |
| `pr-policy` | AI-disclosure and commit-message rules |
| `Package & release` | builds the Windows, macOS and Linux packages and test-installs them (only on PRs touching app, core, build or installers) |

## Code style

- clang-format (Google-based, 100 columns) is the authority on layout.
- Naming: `CamelCase` types, `camelCase` functions and variables, `member_`
  for private members, `kConstant` for constants.
- Prefer RAII, `std::unique_ptr`, value types and `const`. Avoid raw
  `new`/`delete` outside Qt's parent-owned widgets.
- No Qt in `core/`. No SQL outside repositories.
- Every `core/` behaviour change comes with a test.

## Design decisions

Significant decisions are recorded as Architecture Decision Records in
[docs/adr/](docs/adr/). If your change alters one (a new dependency, a storage
change, a new network call), add a new ADR in the same PR.

## Reporting bugs and security issues

Use the issue templates for bugs and ideas. For security problems, follow
[SECURITY.md](SECURITY.md) instead of opening a public issue.

By contributing you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md)
and that your contribution is licensed under the [MIT License](LICENSE).
