#pragma once

/// @file ai_note_repository.hpp
/// Storage for AI output (summaries, prompts, answers).
///
/// AI text is kept in its own table, never mixed into the user's entries, so
/// the diary always shows exactly what the user wrote.

#include "glimmer/database.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace glimmer {

enum class AiNoteKind : std::uint8_t { Summary, Prompt, Answer };

[[nodiscard]] std::string_view toString(AiNoteKind kind) noexcept;
[[nodiscard]] std::optional<AiNoteKind> aiNoteKindFromString(std::string_view text) noexcept;

struct AiNote {
  std::int64_t id = 0;
  AiNoteKind kind = AiNoteKind::Summary;
  std::string rangeStart;  ///< YYYY-MM-DD, or empty.
  std::string rangeEnd;    ///< YYYY-MM-DD, or empty.
  std::string request;     ///< What the user asked for, in their words.
  std::string content;     ///< What the model replied.
  std::string model;       ///< Model name reported by the gateway.
  std::string createdAt;
};

class AiNoteRepository {
 public:
  explicit AiNoteRepository(Database& db) : db_(db) {}

  std::int64_t add(AiNote& note);
  bool remove(std::int64_t id);

  /// Most recent first.
  [[nodiscard]] std::vector<AiNote> recent(int limit = 50);

 private:
  Database& db_;
};

}  // namespace glimmer
