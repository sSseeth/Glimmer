#pragma once

/// @file entry_repository.hpp
/// Reads and writes diary entries. All SQL for entries lives here, so the rest
/// of the program never sees a query string.

#include "glimmer/database.hpp"
#include "glimmer/entry.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace glimmer {

class EntryRepository {
 public:
  /// `db` must outlive the repository and must already be migrated.
  explicit EntryRepository(Database& db) : db_(db) {}

  /// Inserts `entry` (and its collage items). Fills in `entry.id`,
  /// `createdAt` and `updatedAt`. Returns the new id.
  std::int64_t create(Entry& entry);

  /// Saves changes to an existing entry, replacing its collage items.
  /// Throws std::invalid_argument if the entry has no id or doesn't exist.
  void update(Entry& entry);

  /// Deletes the entry (collage items are removed by ON DELETE CASCADE).
  /// Returns false if no such entry existed.
  bool remove(std::int64_t id);

  [[nodiscard]] std::optional<Entry> findById(std::int64_t id);

  /// Entries on one day, oldest first.
  [[nodiscard]] std::vector<Entry> findByDate(Date date);

  /// Entries between `from` and `to` inclusive, ordered by date then id.
  [[nodiscard]] std::vector<Entry> findInRange(Date from, Date to);

  /// Case-insensitive substring search over titles and bodies.
  [[nodiscard]] std::vector<Entry> search(std::string_view text);

  /// One summary per day that has at least one entry in [from, to].
  [[nodiscard]] std::vector<DaySummary> daySummaries(Date from, Date to);

 private:
  std::vector<Entry> query(Statement& stmt);
  void loadCollage(Entry& entry);
  void saveCollage(const Entry& entry);
  void refreshTimestamps(Entry& entry);

  Database& db_;
};

}  // namespace glimmer
