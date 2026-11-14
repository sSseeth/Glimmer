#include "glimmer/database.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using glimmer::Database;
using glimmer::DatabaseError;
using glimmer::Transaction;

namespace {

Database makeDbWithTable() {
  auto db = Database::openInMemory();
  db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, score REAL);");
  return db;
}

int countRows(Database& db) {
  auto stmt = db.prepare("SELECT COUNT(*) FROM t;");
  EXPECT_TRUE(stmt.step());
  return stmt.columnInt(0);
}

}  // namespace

TEST(Database, BindsAndReadsBackValues) {
  auto db = makeDbWithTable();
  db.prepare("INSERT INTO t (name, score) VALUES (?1, ?2);").bind(1, "Ada").bind(2, 9.5).run();
  EXPECT_EQ(db.lastInsertRowId(), 1);

  auto stmt = db.prepare("SELECT id, name, score FROM t;");
  ASSERT_TRUE(stmt.step());
  EXPECT_EQ(stmt.columnInt64(0), 1);
  EXPECT_EQ(stmt.columnText(1), "Ada");
  EXPECT_DOUBLE_EQ(stmt.columnDouble(2), 9.5);
  EXPECT_FALSE(stmt.step());
}

TEST(Database, TextWithEmbeddedNulAndUnicodeSurvives) {
  auto db = makeDbWithTable();
  const std::string tricky = std::string{"a\0b", 3} + " 日記 ✨";
  db.prepare("INSERT INTO t (name) VALUES (?1);").bind(1, tricky).run();
  auto stmt = db.prepare("SELECT name FROM t;");
  ASSERT_TRUE(stmt.step());
  EXPECT_EQ(stmt.columnText(0), tricky);
}

TEST(Database, NullColumnsReadAsEmpty) {
  auto db = makeDbWithTable();
  db.prepare("INSERT INTO t (name) VALUES (?1);").bindNull(1).run();
  auto stmt = db.prepare("SELECT name FROM t;");
  ASSERT_TRUE(stmt.step());
  EXPECT_TRUE(stmt.columnIsNull(0));
  EXPECT_EQ(stmt.columnText(0), "");
}

TEST(Database, BadSqlThrowsWithMessage) {
  auto db = Database::openInMemory();
  try {
    (void)db.prepare("SELEKT nonsense;");
    FAIL() << "expected DatabaseError";
  } catch (const DatabaseError& e) {
    EXPECT_NE(std::string{e.what()}.find("syntax error"), std::string::npos);
  }
}

TEST(Database, StatementCanBeResetAndReused) {
  auto db = makeDbWithTable();
  auto insert = db.prepare("INSERT INTO t (name) VALUES (?1);");
  for (const char* name : {"a", "b", "c"}) {
    insert.bind(1, name).run();
    insert.reset();
  }
  EXPECT_EQ(countRows(db), 3);
}

TEST(Database, TransactionRollsBackWhenNotCommitted) {
  auto db = makeDbWithTable();
  {
    Transaction tx{db};
    db.execute("INSERT INTO t (name) VALUES ('temp');");
    // No commit: destructor rolls back.
  }
  EXPECT_EQ(countRows(db), 0);
}

TEST(Database, TransactionRollsBackOnException) {
  auto db = makeDbWithTable();
  EXPECT_THROW(
      {
        Transaction tx{db};
        db.execute("INSERT INTO t (name) VALUES ('temp');");
        db.execute("INSERT INTO missing_table VALUES (1);");  // throws
        tx.commit();
      },
      DatabaseError);
  EXPECT_EQ(countRows(db), 0);
}

TEST(Database, TransactionCommitPersists) {
  auto db = makeDbWithTable();
  {
    Transaction tx{db};
    db.execute("INSERT INTO t (name) VALUES ('kept');");
    tx.commit();
  }
  EXPECT_EQ(countRows(db), 1);
}

TEST(Database, ForeignKeysAreEnforced) {
  auto db = Database::openInMemory();
  db.execute(
      "CREATE TABLE parent (id INTEGER PRIMARY KEY);"
      "CREATE TABLE child (id INTEGER PRIMARY KEY, parent_id INTEGER NOT NULL "
      "REFERENCES parent (id));");
  EXPECT_THROW(db.execute("INSERT INTO child (parent_id) VALUES (42);"), DatabaseError);
}

TEST(Database, UserVersionRoundTrips) {
  auto db = Database::openInMemory();
  EXPECT_EQ(db.userVersion(), 0);
  db.setUserVersion(7);
  EXPECT_EQ(db.userVersion(), 7);
}

TEST(Database, IsMovable) {
  auto a = makeDbWithTable();
  Database b = std::move(a);
  b.execute("INSERT INTO t (name) VALUES ('moved');");
  EXPECT_EQ(countRows(b), 1);
}

TEST(Database, FileDatabasePersistsAcrossConnections) {
  const auto path = std::filesystem::temp_directory_path() / "glimmer-db-persist-test.sqlite";
  std::filesystem::remove(path);
  {
    Database db{path};
    db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, score REAL);");
    db.execute("INSERT INTO t (name) VALUES ('saved');");
  }
  {
    Database db{path};
    EXPECT_EQ(countRows(db), 1);
  }
  for (const char* suffix : {"", "-wal", "-shm"}) {
    std::filesystem::remove(path.string() + suffix);
  }
}

TEST(Database, OpeningAnUnwritablePathThrows) {
  EXPECT_THROW(Database{std::filesystem::path{"/nonexistent-dir/glimmer.sqlite"}}, DatabaseError);
}
