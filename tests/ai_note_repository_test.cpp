#include "glimmer/ai_note_repository.hpp"

#include "glimmer/migrations.hpp"

#include <gtest/gtest.h>

using glimmer::AiNote;
using glimmer::AiNoteKind;

TEST(AiNoteRepository, AddListAndRemove) {
  auto db = glimmer::Database::openInMemory();
  glimmer::migrate(db);
  glimmer::AiNoteRepository repo{db};

  AiNote first;
  first.kind = AiNoteKind::Summary;
  first.rangeStart = "2026-11-09";
  first.rangeEnd = "2026-11-15";
  first.request = "Summarise this week";
  first.content = "A calm week.";
  first.model = "test-model";
  repo.add(first);

  AiNote second;
  second.kind = AiNoteKind::Prompt;
  second.content = "What made you smile today?";
  repo.add(second);

  EXPECT_GT(first.id, 0);
  EXPECT_FALSE(first.createdAt.empty());

  const auto notes = repo.recent();
  ASSERT_EQ(notes.size(), 2U);
  EXPECT_EQ(notes[0].kind, AiNoteKind::Prompt);  // newest first
  EXPECT_EQ(notes[1].content, "A calm week.");
  EXPECT_EQ(notes[1].rangeEnd, "2026-11-15");

  EXPECT_TRUE(repo.remove(first.id));
  EXPECT_EQ(repo.recent().size(), 1U);
}

TEST(AiNoteRepository, RecentRespectsLimit) {
  auto db = glimmer::Database::openInMemory();
  glimmer::migrate(db);
  glimmer::AiNoteRepository repo{db};
  for (int i = 0; i < 5; ++i) {
    AiNote note;
    note.content = std::to_string(i);
    repo.add(note);
  }
  EXPECT_EQ(repo.recent(3).size(), 3U);
}

TEST(AiNoteKind, StringRoundTrip) {
  for (const auto kind : {AiNoteKind::Summary, AiNoteKind::Prompt, AiNoteKind::Answer}) {
    EXPECT_EQ(glimmer::aiNoteKindFromString(glimmer::toString(kind)), kind);
  }
  EXPECT_FALSE(glimmer::aiNoteKindFromString("bogus").has_value());
}
