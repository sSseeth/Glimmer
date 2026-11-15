#!/usr/bin/env python3
"""Checks that a pull request follows AI_POLICY.md.

Used by .github/workflows/pr-policy.yml, and runnable locally:

    python3 scripts/check_ai_disclosure.py --body pr-body.md --range origin/main..HEAD

Rules (each maps to a section of AI_POLICY.md):
  1. The PR body ticks exactly one AI-assistance level: None, Assisted, Generated.
  2. The "I have read and understand every line" box is ticked.
  3. If any commit carries an `Assisted-by:` trailer, the level is not "None".
  4. If the level is "Generated", at least one commit carries `Assisted-by:`.
  5. `Assisted-by:` trailers name a tool (format `Assisted-by: <tool>[:<model>]`).
  6. No commit lists an AI tool as `Co-authored-by:` or `Signed-off-by:`;
     a human is the author and signs off, AI involvement is `Assisted-by:`.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass, field

LEVELS = ("None", "Assisted", "Generated")

# Names that indicate an AI tool in an authorship trailer.
AI_NAMES = re.compile(
    r"claude|anthropic|copilot|chatgpt|openai|\bgpt|gemini|cursor|codex|devin|"
    r"tabnine|codeium|windsurf|aider|grok|llama|mistral|deepseek",
    re.IGNORECASE,
)
ASSISTED_BY = re.compile(r"^Assisted-by:(.*)$", re.IGNORECASE | re.MULTILINE)
AUTHORSHIP_TRAILER = re.compile(r"^(Co-authored-by|Signed-off-by):(.*)$", re.IGNORECASE | re.MULTILINE)
CHECKBOX = re.compile(r"^\s*[-*]\s*\[( |x|X)\]\s*(.*)$")


@dataclass
class Result:
    errors: list[str] = field(default_factory=list)

    def fail(self, message: str) -> None:
        self.errors.append(message)


def section(body: str, heading: str) -> str:
    """Returns the text under `## heading` up to the next `## ` heading."""
    match = re.search(rf"^##\s+{re.escape(heading)}\s*$(.*?)(?=^##\s|\Z)", body, re.MULTILINE | re.DOTALL | re.IGNORECASE)
    return match.group(1) if match else ""


def checked_levels(body: str) -> list[str]:
    found = []
    for line in section(body, "AI assistance").splitlines():
        box = CHECKBOX.match(line)
        if not box or box.group(1) == " ":
            continue
        for level in LEVELS:
            if re.search(rf"\*\*{level}\*\*", box.group(2), re.IGNORECASE):
                found.append(level)
    return found


def review_box_ticked(body: str) -> bool:
    for line in section(body, "Human review").splitlines():
        box = CHECKBOX.match(line)
        if box and box.group(1) != " " and "understand every line" in box.group(2).lower():
            return True
    return False


def commit_messages(git_range: str) -> list[tuple[str, str]]:
    """Returns (short sha, full message) for each commit in the range."""
    out = subprocess.run(
        ["git", "log", "--format=%h%x00%B%x01", git_range],
        check=True, capture_output=True, text=True,
    ).stdout
    commits = []
    for chunk in out.split("\x01"):
        chunk = chunk.strip("\n")
        if not chunk:
            continue
        sha, _, message = chunk.partition("\x00")
        commits.append((sha, message))
    return commits


def check(body: str, commits: list[tuple[str, str]]) -> Result:
    result = Result()

    levels = checked_levels(body)
    if len(levels) != 1:
        result.fail(
            "Tick exactly one AI-assistance level (None / Assisted / Generated) in the "
            f"'## AI assistance' section of the PR description. Found: {levels or 'none'}."
        )
    level = levels[0] if len(levels) == 1 else None

    if not review_box_ticked(body):
        result.fail("Tick 'I have read and understand every line of this change' under '## Human review'.")

    assisted_commits = []
    for sha, message in commits:
        for match in ASSISTED_BY.finditer(message):
            value = match.group(1).strip()
            if not value:
                result.fail(f"{sha}: 'Assisted-by:' trailer is empty; name the tool, e.g. 'Assisted-by: Claude:claude-opus-5-5'.")
            assisted_commits.append(sha)
        for match in AUTHORSHIP_TRAILER.finditer(message):
            if AI_NAMES.search(match.group(2)):
                result.fail(
                    f"{sha}: '{match.group(1)}: {match.group(2).strip()}' credits an AI tool as an author. "
                    "Use 'Assisted-by:' instead; authorship and sign-off are for humans (AI_POLICY.md)."
                )

    if level == "None" and assisted_commits:
        result.fail(
            "The PR says 'None' but these commits have Assisted-by trailers: "
            + ", ".join(sorted(set(assisted_commits)))
        )
    if level == "Generated" and not assisted_commits:
        result.fail("The PR says 'Generated' but no commit has an 'Assisted-by:' trailer. Add one to the AI-written commits.")

    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--body", required=True, help="file containing the PR description")
    parser.add_argument("--range", default="", help="git revision range of the PR's commits, e.g. base..head")
    args = parser.parse_args()

    with open(args.body, encoding="utf-8") as handle:
        body = handle.read()
    commits = commit_messages(args.range) if args.range else []

    result = check(body, commits)
    for error in result.errors:
        print(f"::error::{error}")
    if result.errors:
        print(f"\n{len(result.errors)} AI-policy problem(s). See AI_POLICY.md.")
        return 1
    print(f"AI policy OK ({len(commits)} commit(s) checked).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
