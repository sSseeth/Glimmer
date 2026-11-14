#include "glimmer/ai_note_repository.hpp"

namespace glimmer {

std::string_view toString(AiNoteKind kind) noexcept {
  switch (kind) {
    case AiNoteKind::Summary:
      return "summary";
    case AiNoteKind::Prompt:
      return "prompt";
    case AiNoteKind::Answer:
      return "answer";
  }
  return "summary";
}

std::optional<AiNoteKind> aiNoteKindFromString(std::string_view text) noexcept {
  if (text == "summary") {
    return AiNoteKind::Summary;
  }
  if (text == "prompt") {
    return AiNoteKind::Prompt;
  }
  if (text == "answer") {
    return AiNoteKind::Answer;
  }
  return std::nullopt;
}

std::int64_t AiNoteRepository::add(AiNote& note) {
  auto stmt = db_.prepare(
      "INSERT INTO ai_notes (kind, range_start, range_end, request, content, model) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6);");
  stmt.bind(1, toString(note.kind))
      .bind(2, note.rangeStart)
      .bind(3, note.rangeEnd)
      .bind(4, note.request)
      .bind(5, note.content)
      .bind(6, note.model);
  stmt.run();
  note.id = db_.lastInsertRowId();

  auto ts = db_.prepare("SELECT created_at FROM ai_notes WHERE id = ?1;");
  ts.bind(1, note.id);
  if (ts.step()) {
    note.createdAt = ts.columnText(0);
  }
  return note.id;
}

bool AiNoteRepository::remove(std::int64_t id) {
  db_.prepare("DELETE FROM ai_notes WHERE id = ?1;").bind(1, id).run();
  return db_.changes() > 0;
}

std::vector<AiNote> AiNoteRepository::recent(int limit) {
  auto stmt = db_.prepare(
      "SELECT id, kind, range_start, range_end, request, content, model, created_at "
      "FROM ai_notes ORDER BY id DESC LIMIT ?1;");
  stmt.bind(1, limit);
  std::vector<AiNote> notes;
  while (stmt.step()) {
    AiNote note;
    note.id = stmt.columnInt64(0);
    note.kind = aiNoteKindFromString(stmt.columnText(1)).value_or(AiNoteKind::Summary);
    note.rangeStart = stmt.columnText(2);
    note.rangeEnd = stmt.columnText(3);
    note.request = stmt.columnText(4);
    note.content = stmt.columnText(5);
    note.model = stmt.columnText(6);
    note.createdAt = stmt.columnText(7);
    notes.push_back(std::move(note));
  }
  return notes;
}

}  // namespace glimmer
