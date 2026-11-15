#!/usr/bin/env bash
# Formats every tracked C++ file in place (CI runs the same check read-only).
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"
git ls-files '*.cpp' '*.hpp' | xargs "$CLANG_FORMAT" -i
echo "Formatted with $("$CLANG_FORMAT" --version)"
