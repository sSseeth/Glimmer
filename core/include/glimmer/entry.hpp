#pragma once

/// @file entry.hpp
/// The diary's domain model. Plain structs, no behaviour, no Qt.

#include "glimmer/date.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace glimmer {

/// How an entry was journaled. Stored in the database as lower-case text.
enum class EntryKind : std::uint8_t {
  Text,     ///< Typed with the keyboard.
  Voice,    ///< Recorded audio, optionally with a transcript in `body`.
  Collage,  ///< Images arranged freely on a canvas.
};

[[nodiscard]] std::string_view toString(EntryKind kind) noexcept;
[[nodiscard]] std::optional<EntryKind> entryKindFromString(std::string_view text) noexcept;

/// One image placed on a collage canvas.
struct CollageItem {
  std::int64_t id = 0;
  std::string imagePath;  ///< Relative to the MediaLibrary root.
  double x = 0.0;
  double y = 0.0;
  double scale = 1.0;
  double rotationDegrees = 0.0;
  int z = 0;  ///< Stacking order; higher draws on top.

  friend bool operator==(const CollageItem&, const CollageItem&) = default;
};

/// A single diary entry. A day can have any number of entries.
struct Entry {
  std::int64_t id = 0;  ///< 0 means "not saved yet".
  Date date{};
  EntryKind kind = EntryKind::Text;
  std::string title;
  std::string body;       ///< Text, or the transcript of a voice entry.
  std::string audioPath;  ///< Relative to the MediaLibrary root; voice only.
  std::vector<CollageItem> collage;
  std::string createdAt;  ///< UTC, ISO-8601; set by the database.
  std::string updatedAt;  ///< UTC, ISO-8601; set by the database.
};

/// Per-day roll-up used by the calendar to mark journaled days.
struct DaySummary {
  Date date{};
  int entryCount = 0;
  bool hasText = false;
  bool hasVoice = false;
  bool hasCollage = false;

  friend bool operator==(const DaySummary&, const DaySummary&) = default;
};

}  // namespace glimmer
