# Setting up the GitHub repository

Do these once, after pushing for the first time. Together they're what turn
the policies in this repo into rules GitHub enforces.

## 1. Check the owner name

The repository URLs and `.github/CODEOWNERS` point at `sSseeth/Glimmer`.
If you fork or rename the repo, update them:

```sh
git grep -l 'sSseeth' | xargs sed -i 's#sSseeth/Glimmer#new-owner/new-name#g; s#@sSseeth#@new-owner#g'
```

## 2. Merge settings (Settings → General → Pull Requests)

- ✅ Allow rebase merging
- ✅ Allow merge commits (optional)
- ❌ Allow squash merging. Squashing can drop per-commit `Assisted-by:`
  trailers. If you keep it, set the default message to "Pull request title
  and commit details".
- ✅ Automatically delete head branches

## 3. Protect `main` (Settings → Rules → Rulesets → New branch ruleset)

Target: the default branch. Enable:

- **Restrict deletions** and **Block force pushes**
- **Require a pull request before merging.** You can't approve your own PR
  on a solo repo, so leave required approvals at 0 and rely on the checks
  below and the template's self-review boxes. Raise it to 1 when others join.
- **Require conversation resolution before merging**
- **Require status checks to pass**, adding:
  - `clang-format`
  - `core (ubuntu-24.04, gcc)`, `core (ubuntu-24.04, clang)`, `core (macos-14, apple-clang)`
  - `clang-tidy`
  - `app (Linux, whisper=OFF)`, `app (Linux, whisper=ON)`
  - `AI disclosure`, `Conventional commits`
  - `Analyze (C/C++)`
- **Require linear history** (fits rebase merging)

(Status checks only appear in the picker after they've run once, so open
your first PR before doing this step.)

## 4. Security (Settings → Code security)

- ✅ Dependabot alerts and security updates
- ✅ Secret scanning and push protection
- ✅ Private vulnerability reporting (used by `SECURITY.md`)
- CodeQL runs from `.github/workflows/codeql.yml`; no default setup needed.

## 5. Actions (Settings → Actions → General)

- Workflow permissions: **Read repository contents** (the workflows ask for
  more only where they need it).
- ✅ Require approval for workflows from outside contributors.
