#include "glimmer/entry_repository.hpp"

#include "strings.hpp"

#include <map>
#include <stdexcept>

namespace glimmer {

namespace {

constexpr std::string_view kSelectColumns =
    "SELECT id, entry_date, kind, title, body, audio_path, created_at, updated_at "
    "FROM entries ";

Entry readEntryRow(const Statement& stmt) {
  Entry entry;
  entry.id = stmt.columnInt64(0);
  const auto dateText = stmt.columnText(1);
  const auto date = parseIsoDate(dateText);
  if (!date) {
    throw DatabaseError("corrupt entry_date '" + dateText + "'", 0);
  }
  entry.date = *date;
  entry.kind = entryKindFromString(stmt.columnText(2)).value_or(EntryKind::Text);
  entry.title = stmt.columnText(3);
  entry.body = stmt.columnText(4);
  entry.audioPath = stmt.columnText(5);
  entry.createdAt = stmt.columnText(6);
  entry.updatedAt = stmt.columnText(7);
  return entry;
}

/// Escapes LIKE wildcards so user text is matched literally.
std::string likePattern(std::string_view text) {
  std::string pattern = "%";
  for (const char c : text) {
    if (c == '%' || c == '_' || c == '\\') {
      pattern += '\\';
    }
    pattern += c;
  }
  pattern += '%';
  return pattern;
}

}  // namespace

std::int64_t EntryRepository::create(Entry& entry) {
  Transaction tx{db_};
  auto stmt = db_.prepare(
      "INSERT INTO entries (entry_date, kind, title, body, audio_path) "
      "VALUES (?1, ?2, ?3, ?4, ?5);");
  stmt.bind(1, toIsoString(entry.date))
      .bind(2, toString(entry.kind))
      .bind(3, entry.title)
      .bind(4, entry.body)
      .bind(5, entry.audioPath);
  stmt.run();
  entry.id = db_.lastInsertRowId();
  saveCollage(entry);
  tx.commit();

  refreshTimestamps(entry);
  loadCollage(entry);  // Picks up the new collage item ids.
  return entry.id;
}

void EntryRepository::update(Entry& entry) {
  if (entry.id == 0) {
    throw std::invalid_argument("cannot update an entry that has not been created");
  }
  Transaction tx{db_};
  auto stmt = db_.prepare(
      "UPDATE entries SET entry_date = ?1, kind = ?2, title = ?3, body = ?4, "
      "audio_path = ?5, updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') "
      "WHERE id = ?6;");
  stmt.bind(1, toIsoString(entry.date))
      .bind(2, toString(entry.kind))
      .bind(3, entry.title)
      .bind(4, entry.body)
      .bind(5, entry.audioPath)
      .bind(6, entry.id);
  stmt.run();
  if (db_.changes() == 0) {
    throw std::invalid_argument(detail::concat("no entry with id ", std::to_string(entry.id)));
  }
  saveCollage(entry);
  tx.commit();

  refreshTimestamps(entry);
  loadCollage(entry);
}

bool EntryRepository::remove(std::int64_t id) {
  auto stmt = db_.prepare("DELETE FROM entries WHERE id = ?1;");
  stmt.bind(1, id).run();
  return db_.changes() > 0;
}

std::optional<Entry> EntryRepository::findById(std::int64_t id) {
  auto stmt = db_.prepare(std::string{kSelectColumns} + "WHERE id = ?1;");
  stmt.bind(1, id);
  auto results = query(stmt);
  if (results.empty()) {
    return std::nullopt;
  }
  return std::move(results.front());
}

std::vector<Entry> EntryRepository::findByDate(Date date) { return findInRange(date, date); }

std::vector<Entry> EntryRepository::findInRange(Date from, Date to) {
  auto stmt = db_.prepare(std::string{kSelectColumns} +
                          "WHERE entry_date BETWEEN ?1 AND ?2 ORDER BY entry_date, id;");
  stmt.bind(1, toIsoString(from)).bind(2, toIsoString(to));
  return query(stmt);
}

std::vector<Entry> EntryRepository::search(std::string_view text) {
  auto stmt = db_.prepare(std::string{kSelectColumns} +
                          "WHERE title LIKE ?1 ESCAPE '\\' OR body LIKE ?1 ESCAPE '\\' "
                          "ORDER BY entry_date DESC, id DESC;");
  stmt.bind(1, likePattern(text));
  return query(stmt);
}

std::vector<DaySummary> EntryRepository::daySummaries(Date from, Date to) {
  auto stmt = db_.prepare(
      "SELECT entry_date, kind, COUNT(*) FROM entries "
      "WHERE entry_date BETWEEN ?1 AND ?2 GROUP BY entry_date, kind ORDER BY entry_date;");
  stmt.bind(1, toIsoString(from)).bind(2, toIsoString(to));

  // std::map keeps days sorted; year_month_day has operator<=>.
  std::map<Date, DaySummary> byDay;
  while (stmt.step()) {
    const auto date = parseIsoDate(stmt.columnText(0));
    if (!date) {
      continue;
    }
    auto& summary = byDay[*date];
    summary.date = *date;
    summary.entryCount += stmt.columnInt(2);
    switch (entryKindFromString(stmt.columnText(1)).value_or(EntryKind::Text)) {
      case EntryKind::Text:
        summary.hasText = true;
        break;
      case EntryKind::Voice:
        summary.hasVoice = true;
        break;
      case EntryKind::Collage:
        summary.hasCollage = true;
        break;
    }
  }

  std::vector<DaySummary> result;
  result.reserve(byDay.size());
  for (const auto& day : byDay) {
    result.push_back(day.second);
  }
  return result;
}

// --- private -----------------------------------------------------------------

std::vector<Entry> EntryRepository::query(Statement& stmt) {
  std::vector<Entry> entries;
  while (stmt.step()) {
    entries.push_back(readEntryRow(stmt));
  }
  for (auto& entry : entries) {
    loadCollage(entry);
  }
  return entries;
}

void EntryRepository::loadCollage(Entry& entry) {
  entry.collage.clear();
  auto stmt = db_.prepare(
      "SELECT id, image_path, x, y, scale, rotation, z FROM collage_items "
      "WHERE entry_id = ?1 ORDER BY z, id;");
  stmt.bind(1, entry.id);
  while (stmt.step()) {
    CollageItem item;
    item.id = stmt.columnInt64(0);
    item.imagePath = stmt.columnText(1);
    item.x = stmt.columnDouble(2);
    item.y = stmt.columnDouble(3);
    item.scale = stmt.columnDouble(4);
    item.rotationDegrees = stmt.columnDouble(5);
    item.z = stmt.columnInt(6);
    entry.collage.push_back(std::move(item));
  }
}

void EntryRepository::saveCollage(const Entry& entry) {
  // Replace-all keeps the code simple; collages are small.
  db_.prepare("DELETE FROM collage_items WHERE entry_id = ?1;").bind(1, entry.id).run();
  auto insert = db_.prepare(
      "INSERT INTO collage_items (entry_id, image_path, x, y, scale, rotation, z) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);");
  for (const auto& item : entry.collage) {
    insert.bind(1, entry.id)
        .bind(2, item.imagePath)
        .bind(3, item.x)
        .bind(4, item.y)
        .bind(5, item.scale)
        .bind(6, item.rotationDegrees)
        .bind(7, item.z);
    insert.run();
    insert.reset();
  }
}

void EntryRepository::refreshTimestamps(Entry& entry) {
  auto stmt = db_.prepare("SELECT created_at, updated_at FROM entries WHERE id = ?1;");
  stmt.bind(1, entry.id);
  if (stmt.step()) {
    entry.createdAt = stmt.columnText(0);
    entry.updatedAt = stmt.columnText(1);
  }
}

}  // namespace glimmer
