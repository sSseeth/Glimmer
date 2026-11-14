# 5. AI-assisted development policy

- Status: accepted
- Date: 2026-11-14

## Context

Some of this project, including its initial version, is written with AI
coding tools. Git can't tell AI-written code from human code, so a PR made
by an AI looks identical to one made by a person unless we label it.

## Decision

Adopt [AI_POLICY.md](../../AI_POLICY.md), following the conventions used by
LLVM, Fedora, OpenSSL and others:

- a human author is accountable for every line;
- `Assisted-by: <tool>:<model>` commit trailers mark AI-generated content;
- AI is never credited as `Co-authored-by:` or `Signed-off-by:`;
- the PR template declares one of None / Assisted / Generated;
- the same CI and review bar applies to all code;
- rebase merging keeps per-commit trailers in `main`.

These rules are enforced by `scripts/hooks/commit-msg` and
`.github/workflows/pr-policy.yml`. Instructions for AI agents are in
`AGENTS.md`.

## Consequences

`git log --grep=Assisted-by` gives an honest provenance trail. Contributors
spend a few seconds per PR on disclosure.
