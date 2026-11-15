# 1. Record architecture decisions

- Status: accepted
- Date: 2026-11-14

## Context

Decisions like "why SQLite?" or "why no JSON library?" get forgotten, and
then they get relitigated or quietly undone.

## Decision

We record significant decisions as short Architecture Decision Records
(ADRs) in `docs/adr/`, numbered in order, using this template: Context,
Decision, Consequences. An ADR is never edited after it's accepted; a later
ADR *supersedes* it.

A PR that adds a dependency, changes storage, adds a network call or changes
a project-wide convention must include an ADR.

## Consequences

Reviewers and future contributors (human or AI) can see why things are the
way they are. It costs about ten minutes per decision.
