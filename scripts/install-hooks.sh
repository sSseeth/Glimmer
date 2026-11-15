#!/usr/bin/env bash
# Points git at the repo's hooks and commit template. Run once per clone.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
git config core.hooksPath scripts/hooks
git config commit.template .gitmessage
chmod +x scripts/hooks/*
echo "Hooks installed (core.hooksPath=scripts/hooks) and commit template set."
