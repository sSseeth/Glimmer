#include "glimmer/entry_repository.hpp"

#include "glimmer/migrations.hpp"

#include <gtest/gtest.h>

using namespace std::chrono;
using glimmer::Date;
using glimmer::Entry;
using glimmer::EntryKind;

namespace {

class EntryRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override { glimmer::migrate(db_); }

  Entry make(Date date, EntryKind kind, std::string title, std::string body = {}) {
    Entry e;
    e.date = date;
    e.kind = kind;
    e.title = std::move(title);
    e.body = std::move(body);
    repo_.create(e);
    return e;
  }

  glimmer::Database db_ = glimmer::Database::openInMemory();
  glimmer::EntryRepository repo_{db_};
};

constexpr Date kNov14{year{2026}, November, day{14}};
constexpr Date kNov15{year{2026}, November, day{15}};
constexpr Date kDec01{year{2026}, December, day{1}};

}  // namespace

TEST_F(EntryRepositoryTest, CreateAssignsIdAndTimestamps) {
  const auto e = make(kNov14, EntryKind::Text, "Hello", "First entry");
  EXPECT_GT(e.id, 0);
  EXPECT_FALSE(e.createdAt.empty());
  EXPECT_EQ(e.createdAt, e.updatedAt);
}

TEST_F(EntryRepositoryTest, FindByIdRoundTripsAllFields) {
  Entry e;
  e.date = kNov14;
  e.kind = EntryKind::Voice;
  e.title = "Walk";
  e.body = "transcript here";
  e.audioPath = "audio/1.wav";
  repo_.create(e);

  const auto found = repo_.findById(e.id);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->date, kNov14);
  EXPECT_EQ(found->kind, EntryKind::Voice);
  EXPECT_EQ(found->title, "Walk");
  EXPECT_EQ(found->body, "transcript here");
  EXPECT_EQ(found->audioPath, "audio/1.wav");
}

TEST_F(EntryRepositoryTest, FindByIdReturnsNulloptForMissing) {
  EXPECT_FALSE(repo_.findById(999).has_value());
}

TEST_F(EntryRepositoryTest, CollageItemsArePersistedInZOrder) {
  Entry e;
  e.date = kNov14;
  e.kind = EntryKind::Collage;
  e.collage.push_back({0, "images/b.png", 10, 20, 0.5, 15, 2});
  e.collage.push_back({0, "images/a.png", 1, 2, 1.0, 0, 1});
  repo_.create(e);

  ASSERT_EQ(e.collage.size(), 2U);
  EXPECT_EQ(e.collage[0].imagePath, "images/a.png");  // z=1 first
  EXPECT_GT(e.collage[0].id, 0);

  const auto found = repo_.findById(e.id);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->collage, e.collage);
}

TEST_F(EntryRepositoryTest, UpdateChangesFieldsAndReplacesCollage) {
  Entry e;
  e.date = kNov14;
  e.kind = EntryKind::Collage;
  e.collage.push_back({0, "images/a.png", 0, 0, 1, 0, 0});
  repo_.create(e);

  e.title = "Renamed";
  e.date = kNov15;
  e.collage = {{0, "images/c.png", 5, 5, 2, 90, 0}};
  repo_.update(e);

  const auto found = repo_.findById(e.id);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->title, "Renamed");
  EXPECT_EQ(found->date, kNov15);
  ASSERT_EQ(found->collage.size(), 1U);
  EXPECT_EQ(found->collage[0].imagePath, "images/c.png");
}

TEST_F(EntryRepositoryTest, UpdateRejectsUnsavedOrMissingEntries) {
  Entry unsaved;
  EXPECT_THROW(repo_.update(unsaved), std::invalid_argument);

  Entry ghost;
  ghost.id = 12345;
  ghost.date = kNov14;
  EXPECT_THROW(repo_.update(ghost), std::invalid_argument);
}

TEST_F(EntryRepositoryTest, RemoveDeletesEntryAndCascadesCollage) {
  Entry e;
  e.date = kNov14;
  e.kind = EntryKind::Collage;
  e.collage.push_back({0, "images/a.png", 0, 0, 1, 0, 0});
  repo_.create(e);

  EXPECT_TRUE(repo_.remove(e.id));
  EXPECT_FALSE(repo_.findById(e.id).has_value());
  EXPECT_FALSE(repo_.remove(e.id));

  auto stmt = db_.prepare("SELECT COUNT(*) FROM collage_items;");
  ASSERT_TRUE(stmt.step());
  EXPECT_EQ(stmt.columnInt(0), 0);
}

TEST_F(EntryRepositoryTest, FindByDateAndRange) {
  make(kNov14, EntryKind::Text, "a");
  make(kNov14, EntryKind::Voice, "b");
  make(kNov15, EntryKind::Text, "c");
  make(kDec01, EntryKind::Text, "d");

  EXPECT_EQ(repo_.findByDate(kNov14).size(), 2U);

  const auto november =
      repo_.findInRange(Date{year{2026}, November, day{1}}, Date{year{2026}, November, day{30}});
  ASSERT_EQ(november.size(), 3U);
  EXPECT_EQ(november[0].title, "a");
  EXPECT_EQ(november[2].title, "c");
}

TEST_F(EntryRepositoryTest, SearchIsCaseInsensitiveAndTreatsWildcardsLiterally) {
  make(kNov14, EntryKind::Text, "Beach day", "Sunny and warm");
  make(kNov15, EntryKind::Text, "Budget", "Saved 100% of it");

  EXPECT_EQ(repo_.search("sunny").size(), 1U);
  EXPECT_EQ(repo_.search("BEACH").size(), 1U);
  EXPECT_EQ(repo_.search("100%").size(), 1U);
  EXPECT_EQ(repo_.search("%").size(), 1U);  // only the entry containing '%'
  EXPECT_EQ(repo_.search("nothing like this").size(), 0U);
}

TEST_F(EntryRepositoryTest, DaySummariesRollUpKinds) {
  make(kNov14, EntryKind::Text, "a");
  make(kNov14, EntryKind::Voice, "b");
  make(kNov14, EntryKind::Voice, "c");
  make(kNov15, EntryKind::Collage, "d");

  const auto summaries =
      repo_.daySummaries(Date{year{2026}, November, day{1}}, Date{year{2026}, November, day{30}});
  ASSERT_EQ(summaries.size(), 2U);

  EXPECT_EQ(summaries[0].date, kNov14);
  EXPECT_EQ(summaries[0].entryCount, 3);
  EXPECT_TRUE(summaries[0].hasText);
  EXPECT_TRUE(summaries[0].hasVoice);
  EXPECT_FALSE(summaries[0].hasCollage);

  EXPECT_EQ(summaries[1].date, kNov15);
  EXPECT_EQ(summaries[1].entryCount, 1);
  EXPECT_TRUE(summaries[1].hasCollage);
}
