#pragma once

#include "glimmer/ai_note_repository.hpp"
#include "glimmer/database.hpp"
#include "glimmer/entry_repository.hpp"
#include "glimmer/media_library.hpp"

#include <QString>

/// Owns everything persistent: the database, its repositories, and the media
/// folder. Constructed once in main() and shared by reference.
///
/// Layout on disk:
///   <dataDir>/glimmer.sqlite
///   <dataDir>/media/images/...
///   <dataDir>/media/audio/...
class DiaryStore {
 public:
  /// Throws glimmer::DatabaseError / std::filesystem::filesystem_error.
  explicit DiaryStore(const QString& dataDir);

  [[nodiscard]] const QString& dataDir() const { return dataDir_; }
  glimmer::EntryRepository& entries() { return entries_; }
  glimmer::AiNoteRepository& aiNotes() { return aiNotes_; }
  glimmer::MediaLibrary& media() { return media_; }

 private:
  QString dataDir_;
  glimmer::Database db_;  // Declared before the repositories that reference it.
  glimmer::EntryRepository entries_;
  glimmer::AiNoteRepository aiNotes_;
  glimmer::MediaLibrary media_;
};
