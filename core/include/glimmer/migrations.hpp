#pragma once

/// @file migrations.hpp
/// Versioned schema migrations.
///
/// Each migration is applied once, in order, inside a transaction. The current
/// version is stored in SQLite's `PRAGMA user_version`. Never edit a migration
/// that has shipped: add a new one instead (see docs/adr/0002-storage.md).

#include "glimmer/database.hpp"

namespace glimmer {

/// The schema version this build of the code expects.
[[nodiscard]] int latestSchemaVersion() noexcept;

/// Brings `db` up to latestSchemaVersion(). Safe to call on every start-up.
/// Throws DatabaseError if the file was written by a *newer* version.
void migrate(Database& db);

}  // namespace glimmer
