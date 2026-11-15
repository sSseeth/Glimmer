#include "glimmer/migrations.hpp"

#include <array>
#include <string_view>

namespace glimmer {

namespace {

// Index 0 upgrades version 0 -> 1, index 1 upgrades 1 -> 2, and so on.
constexpr std::array<std::string_view, 1> kMigrations{
    // v1: initial schema
    R"sql(
CREATE TABLE entries (
  id          INTEGER PRIMARY KEY,
  entry_date  TEXT    NOT NULL,              -- YYYY-MM-DD
  kind        TEXT    NOT NULL CHECK (kind IN ('text', 'voice', 'collage')),
  title       TEXT    NOT NULL DEFAULT '',
  body        TEXT    NOT NULL DEFAULT '',
  audio_path  TEXT    NOT NULL DEFAULT '',
  created_at  TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),
  updated_at  TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))
);
CREATE INDEX idx_entries_date ON entries (entry_date);

CREATE TABLE collage_items (
  id          INTEGER PRIMARY KEY,
  entry_id    INTEGER NOT NULL REFERENCES entries (id) ON DELETE CASCADE,
  image_path  TEXT    NOT NULL,
  x           REAL    NOT NULL DEFAULT 0,
  y           REAL    NOT NULL DEFAULT 0,
  scale       REAL    NOT NULL DEFAULT 1,
  rotation    REAL    NOT NULL DEFAULT 0,
  z           INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX idx_collage_entry ON collage_items (entry_id);

CREATE TABLE ai_notes (
  id           INTEGER PRIMARY KEY,
  kind         TEXT    NOT NULL CHECK (kind IN ('summary', 'prompt', 'answer')),
  range_start  TEXT    NOT NULL DEFAULT '',
  range_end    TEXT    NOT NULL DEFAULT '',
  request      TEXT    NOT NULL DEFAULT '',
  content      TEXT    NOT NULL,
  model        TEXT    NOT NULL DEFAULT '',
  created_at   TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))
);
)sql",
};

}  // namespace

int latestSchemaVersion() noexcept { return static_cast<int>(kMigrations.size()); }

void migrate(Database& db) {
  const int current = db.userVersion();
  const int latest = latestSchemaVersion();
  if (current > latest) {
    throw DatabaseError("database schema v" + std::to_string(current) +
                            " is newer than this app supports (v" + std::to_string(latest) +
                            "); please upgrade glimmer",
                        0);
  }
  for (int version = current; version < latest; ++version) {
    Transaction tx{db};
    db.execute(kMigrations.at(static_cast<std::size_t>(version)));
    db.setUserVersion(version + 1);
    tx.commit();
  }
}

}  // namespace glimmer
