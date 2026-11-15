#include "glimmer/migrations.hpp"

#include <gtest/gtest.h>

using glimmer::Database;

TEST(Migrations, FreshDatabaseReachesLatestVersion) {
  auto db = Database::openInMemory();
  glimmer::migrate(db);
  EXPECT_EQ(db.userVersion(), glimmer::latestSchemaVersion());
}

TEST(Migrations, AreIdempotent) {
  auto db = Database::openInMemory();
  glimmer::migrate(db);
  EXPECT_NO_THROW(glimmer::migrate(db));
  EXPECT_EQ(db.userVersion(), glimmer::latestSchemaVersion());
}

TEST(Migrations, RefuseDatabasesFromTheFuture) {
  auto db = Database::openInMemory();
  db.setUserVersion(glimmer::latestSchemaVersion() + 1);
  EXPECT_THROW(glimmer::migrate(db), glimmer::DatabaseError);
}

TEST(Migrations, CreateExpectedTables) {
  auto db = Database::openInMemory();
  glimmer::migrate(db);
  for (const char* table : {"entries", "collage_items", "ai_notes"}) {
    auto stmt = db.prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?1;");
    stmt.bind(1, table);
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.columnInt(0), 1) << table;
  }
}
