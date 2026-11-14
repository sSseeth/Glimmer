#pragma once

/// @file database.hpp
/// A small RAII wrapper around the SQLite C API.
///
/// Ownership rules:
///  - `Database` owns a `sqlite3*` and closes it in its destructor.
///  - `Statement` owns a `sqlite3_stmt*` and finalizes it in its destructor.
///  - Both are move-only: copying a handle would mean closing it twice.
///  - A `Statement` must not outlive the `Database` that prepared it.

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace glimmer {

/// Thrown for any SQLite failure. `what()` includes SQLite's own message.
class DatabaseError : public std::runtime_error {
 public:
  DatabaseError(const std::string& message, int sqliteCode);
  [[nodiscard]] int sqliteCode() const noexcept { return sqliteCode_; }

 private:
  int sqliteCode_;
};

class Statement {
 public:
  Statement(Statement&&) noexcept = default;
  Statement& operator=(Statement&&) noexcept = default;
  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;
  ~Statement() = default;

  // Parameters are 1-based, as in SQLite.
  Statement& bind(int index, std::int64_t value);
  Statement& bind(int index, int value);
  Statement& bind(int index, double value);
  Statement& bind(int index, std::string_view value);
  Statement& bindNull(int index);

  /// Advances to the next row. Returns true if a row is available, false when
  /// the statement has finished.
  bool step();

  /// Runs a statement that returns no rows (INSERT/UPDATE/DELETE).
  void run();

  /// Resets the statement so it can be stepped again with new bindings.
  void reset();

  // Columns are 0-based, as in SQLite.
  [[nodiscard]] std::int64_t columnInt64(int column) const;
  [[nodiscard]] int columnInt(int column) const;
  [[nodiscard]] double columnDouble(int column) const;
  [[nodiscard]] std::string columnText(int column) const;
  [[nodiscard]] bool columnIsNull(int column) const;

 private:
  friend class Database;
  struct Finalizer {
    void operator()(sqlite3_stmt* stmt) const noexcept;
  };
  Statement(sqlite3* db, sqlite3_stmt* stmt);
  void check(int resultCode, std::string_view what) const;

  sqlite3* db_;  // Non-owning; the Database outlives us.
  std::unique_ptr<sqlite3_stmt, Finalizer> stmt_;
};

class Database {
 public:
  /// Opens (creating if needed) the database file at `path`, with foreign keys
  /// enabled and write-ahead logging for crash safety.
  explicit Database(const std::filesystem::path& path);

  /// Opens a private in-memory database. Used by the tests.
  [[nodiscard]] static Database openInMemory();

  Database(Database&&) noexcept = default;
  Database& operator=(Database&&) noexcept = default;
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;
  ~Database() = default;

  /// Executes one or more SQL statements that return no rows.
  void execute(std::string_view sql);

  [[nodiscard]] Statement prepare(std::string_view sql);

  [[nodiscard]] std::int64_t lastInsertRowId() const noexcept;
  [[nodiscard]] int changes() const noexcept;

  /// The schema version stored in `PRAGMA user_version`.
  [[nodiscard]] int userVersion();
  void setUserVersion(int version);

 private:
  struct Closer {
    void operator()(sqlite3* db) const noexcept;
  };
  struct OpenTag {};  // Disambiguates from the public path constructor.
  Database(OpenTag tag, const std::string& sqliteFilename);

  std::unique_ptr<sqlite3, Closer> db_;
};

/// RAII transaction: rolls back automatically unless `commit()` is called.
/// This makes "all or nothing" writes exception-safe.
class Transaction {
 public:
  explicit Transaction(Database& db);
  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;
  Transaction(Transaction&&) = delete;
  Transaction& operator=(Transaction&&) = delete;
  ~Transaction();

  void commit();

 private:
  Database& db_;
  bool done_ = false;
};

}  // namespace glimmer
