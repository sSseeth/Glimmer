#include "glimmer/entry.hpp"

namespace glimmer {

std::string_view toString(EntryKind kind) noexcept {
  switch (kind) {
    case EntryKind::Text:
      return "text";
    case EntryKind::Voice:
      return "voice";
    case EntryKind::Collage:
      return "collage";
  }
  return "text";  // Unreachable for valid enum values.
}

std::optional<EntryKind> entryKindFromString(std::string_view text) noexcept {
  if (text == "text") {
    return EntryKind::Text;
  }
  if (text == "voice") {
    return EntryKind::Voice;
  }
  if (text == "collage") {
    return EntryKind::Collage;
  }
  return std::nullopt;
}

}  // namespace glimmer
