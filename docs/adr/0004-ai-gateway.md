# 4. AI through an OpenAI-compatible gateway

- Status: accepted
- Date: 2026-11-14

## Context

The AI panel summarises entries, suggests prompts and answers questions. We
don't want to hard-code one AI provider, and diary contents are private.

## Decision

- glimmer talks to **one OpenAI-compatible `/chat/completions` endpoint**,
  by default a local [OmniRoute](https://github.com/diegosouzapw/OmniRoute)
  gateway at `http://127.0.0.1:20128/v1`. The gateway decides which provider
  answers. Any compatible server (OpenAI, a local llama.cpp server, etc.)
  also works.
- **Nothing is sent automatically.** Requests happen only when the user
  presses a button, and contain only the entries in the chosen period.
- Prompt building and request/response encoding live in `core/` (tested);
  only the HTTP call is in `app/`.
- AI output is stored in its own `ai_notes` table and never mixed into
  entries.
- **JSON:** we wrote a small RFC 8259 parser and serializer
  (`core/json.hpp`, about 300 lines, fully tested) instead of adding a
  library, because we only need JSON for this one API. If our needs grow,
  a later ADR can replace it with nlohmann/json.

## Consequences

Switching providers is a settings change, not a code change. Users who don't
want AI never configure it, and nothing leaves the machine.
