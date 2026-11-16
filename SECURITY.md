# Security policy

## Reporting a vulnerability

Please **don't open a public issue**. Use GitHub's private reporting:
**Security → Report a vulnerability** on this repository. You should get a
reply within a week.

## Supported versions

Only the latest release gets security fixes before 1.0.

## How glimmer handles your data

- **Everything is local.** Entries are in `glimmer.sqlite` and media in
  `media/` inside the data folder (see README). There's no telemetry.
- **Voice** recordings and whisper.cpp transcription never leave your
  machine.
- **AI requests** go only to the gateway URL you configure, only when you
  press a button in the AI panel, and contain only the entries in the
  period you chose. If that gateway forwards them to a cloud provider, that
  provider's terms apply.
- **API key storage:** a key typed into Settings is stored in **plain text**
  in Qt's settings file (`~/.config/glimmer/glimmer.conf` on Linux). To
  avoid that, set `GLIMMER_AI_API_KEY` in your environment instead; it is
  never written to disk. Moving to the OS keychain is on the roadmap.
- The diary database is **not encrypted**. Use full-disk encryption if
  your device could be lost or shared.

## Verifying downloads

Every release includes `SHA256SUMS`, and both install scripts refuse to
install a package whose checksum doesn't match. Packages are built only by
the public GitHub Actions workflow in this repository. They aren't signed
with a paid certificate yet (see `docs/releasing.md`).

## Hardening in the codebase

- All SQL uses bound parameters; user text is never put into a query
  string.
- Stored media paths are relative and checked against path traversal
  (`MediaLibrary::resolve`).
- The JSON parser limits nesting depth and validates input strictly.
- CI runs AddressSanitizer, UBSan, clang-tidy and CodeQL on every change.
