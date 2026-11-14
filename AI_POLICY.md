# AI-assisted contributions policy

**In short:** use whatever tools you like, but a human owns every line. Say
when AI wrote something, hold AI code to exactly the same bar as human code,
and never list an AI as an author.

This policy applies to everyone who contributes, including the maintainer. It
follows conventions that large open-source projects have settled on (LLVM,
Fedora, OpenSSL, MariaDB, OpenTelemetry; see [Sources](#sources)).

## 1. Principles

1. **A human is accountable.** Whoever opens the PR is its author and must be
   able to explain every line, including AI-written ones, as if they'd typed
   it. "The AI wrote that" is never an answer in review.
2. **Disclose, don't hide.** Reviewers can't reliably tell AI code apart, so
   disclosure works on good faith. AI use that wasn't disclosed and is found
   later is treated as a mistake to fix, and the change may be reverted.
3. **No lower bar.** AI code goes through the same CI, tests, sanitizers,
   static analysis and review as everything else. Nothing in this repo checks
   *who* wrote code in order to relax a rule.
4. **Don't hand reviewers unreviewed output.** Read and run what a tool gives
   you before asking someone else to review it. LLVM calls skipping this an
   "extractive contribution".
5. **Protect privacy.** This is a diary app. Never paste real diary entries,
   API keys or other secrets into an AI tool while developing it.

## 2. Disclosure levels

Every PR ticks **exactly one** level in its description (the PR template has
the boxes, and CI checks them):

| Level | Meaning | Examples | Commit trailer |
|---|---|---|---|
| **None** | No AI beyond trivial completion | Autocompleting a variable name or closing a bracket | none |
| **Assisted** | AI explained, reviewed or suggested; **you wrote the code** | Asked how `std::unique_ptr` deleters work, then wrote the wrapper; had AI review your diff | optional |
| **Generated** | AI produced code, tests or docs that you kept, edited or not | AI wrote a CI workflow, a test file, a function body or a README section | **required** on those commits |

Grammar, spelling and wording fixes to prose don't need disclosure.

When in doubt, pick the higher level. Over-disclosing costs nothing.

## 3. How to disclose

### Commit trailer

Add an `Assisted-by:` trailer to every commit that contains AI-generated
content, in the form `Assisted-by: <tool>:<model>`:

```
feat(core): add RAII wrapper for sqlite3 statements

Wraps sqlite3_stmt in a unique_ptr with a custom deleter so statements are
finalized even when an exception unwinds the stack.

Assisted-by: Claude:claude-opus-5-5
```

Use one line per tool if you used several. The commit template
(`.gitmessage`, installed by `scripts/install-hooks.sh`) reminds you.

### Keep AI work in its own commits

Where you can, put AI-generated changes and your own changes in **separate
commits**. Then `git log --grep='Assisted-by'` shows exactly where AI was
involved, and `git blame` stays meaningful.

### Attribution: Assisted-by, never Co-authored-by

- The **commit author** is always the human who's responsible for it.
- **Never** credit an AI with `Co-authored-by:` or `Signed-off-by:`. Those
  trailers mean a person who can answer for the work and certify they have
  the right to submit it. A tool can't do either.
- AI coding agents must not commit with their own identity, add those
  trailers, or sign off. See [AGENTS.md](AGENTS.md).

The `commit-msg` hook and the PR-policy workflow reject AI names in
`Co-authored-by:` and `Signed-off-by:` lines.

### Merging

The repository uses **rebase merging** (or merge commits), not squash
merging, so each commit's trailers stay in `main`. If you do squash, keep the
`Assisted-by:` lines in the squashed message.

## 4. Reviewing AI-generated code

Ticking "I have read and understand every line" means you checked at least
this:

- [ ] **It builds and the tests pass locally**, under the `dev` preset
      (sanitizers on).
- [ ] **No invented APIs.** Every Qt, SQLite and standard-library call exists
      with that signature in the versions we support (Qt ≥ 6.4, C++20).
- [ ] **Error handling is real.** No swallowed exceptions or ignored return
      codes without a comment explaining why.
- [ ] **Tests test the right thing.** AI-written tests can assert whatever
      the code happens to do. Check the expected values yourself.
- [ ] **No secrets, no personal data, no telemetry.**
- [ ] **No large verbatim copies** of code from elsewhere with an unknown
      licence. If a tool says it reproduced public code, find the source and
      check the licence is compatible with MIT.
- [ ] **It fits the architecture** ([docs/architecture.md](docs/architecture.md)):
      no Qt in `core/`, SQL only in repositories, AI output kept out of
      entries.

## 5. Enforcement

| Check | Where | What it catches |
|---|---|---|
| `commit-msg` hook | your machine and CI | malformed `Assisted-by:`, AI listed as co-author or sign-off, non-conventional subjects |
| `pr-policy / ai-disclosure` | CI, every PR | no level or several levels ticked; "None" contradicted by `Assisted-by:` commits; "Generated" with no trailers; review box unticked |
| Required review | branch protection | AI code merged without a human reading it |

If a check fails because you forgot a trailer, amend or rebase the commit
(`git commit --amend`, or `git rebase -i` and `reword`) rather than
disabling the check.

## 6. This repository's own history

The initial implementation (build system, core library, tests, Qt app, CI and
these docs) was **generated by Claude (Anthropic)** at the maintainer's
request, and is marked with `Assisted-by: Claude:claude-opus-5-5` trailers.
The maintainer reviewed it before it was merged. Later contributions say
which level applies in their own PRs.

## Sources

- [LLVM AI Tool Use Policy](https://llvm.org/docs/AIToolPolicy.html): human in the loop; "extractive contributions"
- [Fedora AI-Assisted Contributions Policy](https://docs.fedoraproject.org/en-US/council/policy/ai-contribution-policy/): `Assisted-by:` trailer
- [OpenSSL AI Contribution Policy](https://openssl-library.org/policies/general/ai-policy/): `Assisted-by: {agent}:{model}`; no lower bar
- [MariaDB AI contribution policy](https://mariadb.org/?p=40507): responsibility rests with the human submitter
- [OpenTelemetry GenAI policy](https://github.blesdmm.dns-dynamic.net/open-telemetry/community/blob/main/policies/genai.md): `AGENTS.md` for agent instructions
- [PX4 AI coding assistants](https://docs.px4.io/main/en/contribute/ai_assistants): disclosure on good faith
