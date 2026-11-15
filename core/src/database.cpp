#include "glimmer/database.hpp"

#include <sqlite3.h>

#include <limits>

namespace glimmer {

namespace {

std::string describe(sqlite3* db, std::string_view what, int code) {
  std::string message{what};
  message += ": ";
  message += (db != nullptr) ? sqlite3_errmsg(db) : sqlite3_errstr(code);
  return message;
}

/// SQLite expects UTF-8 file names. path::string() would use the ANSI code
/// page on Windows and break non-ASCII user folders.
std::string utf8(const std::filesystem::path& path) {
  const auto u8 = path.u8string();
  return {u8.begin(), u8.end()};
}

int checkedLength(std::string_view text) {
  if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    throw DatabaseError("string too large for SQLite", SQLITE_TOOBIG);
  }
  return static_cast<int>(text.size());
}

}  // namespace

// --- DatabaseError -----------------------------------------------------------

DatabaseError::DatabaseError(const std::string& message, int sqliteCode)
    : std::runtime_error(message), sqliteCode_(sqliteCode) {}

// --- Statement ---------------------------------------------------------------

void Statement::Finalizer::operator()(sqlite3_stmt* stmt) const noexcept { sqlite3_finalize(stmt); }

Statement::Statement(sqlite3* db, sqlite3_stmt* stmt) : db_(db), stmt_(stmt) {}

void Statement::check(int resultCode, std::string_view what) const {
  if (resultCode != SQLITE_OK) {
    throw DatabaseError(describe(db_, what, resultCode), resultCode);
  }
}

Statement& Statement::bind(int index, std::int64_t value) {
  check(sqlite3_bind_int64(stmt_.get(), index, value), "bind int64");
  return *this;
}

Statement& Statement::bind(int index, int value) {
  check(sqlite3_bind_int(stmt_.get(), index, value), "bind int");
  return *this;
}

Statement& Statement::bind(int index, double value) {
  check(sqlite3_bind_double(stmt_.get(), index, value), "bind double");
  return *this;
}

Statement& Statement::bind(int index, std::string_view value) {
  // SQLITE_TRANSIENT tells SQLite to copy the bytes, because `value` may not
  // live until the statement runs.
  check(sqlite3_bind_text(stmt_.get(), index, value.data(), checkedLength(value), SQLITE_TRANSIENT),
        "bind text");
  return *this;
}

Statement& Statement::bindNull(int index) {
  check(sqlite3_bind_null(stmt_.get(), index), "bind null");
  return *this;
}

bool Statement::step() {
  const int rc = sqlite3_step(stmt_.get());
  if (rc == SQLITE_ROW) {
    return true;
  }
  if (rc == SQLITE_DONE) {
    return false;
  }
  throw DatabaseError(describe(db_, "step", rc), rc);
}

void Statement::run() {
  while (step()) {
    // Discard any rows; callers use run() for statements without results.
  }
}

void Statement::reset() {
  sqlite3_reset(stmt_.get());
  sqlite3_clear_bindings(stmt_.get());
}

std::int64_t Statement::columnInt64(int column) const {
  return sqlite3_column_int64(stmt_.get(), column);
}

int Statement::columnInt(int column) const { return sqlite3_column_int(stmt_.get(), column); }

double Statement::columnDouble(int column) const {
  return sqlite3_column_double(stmt_.get(), column);
}

std::string Statement::columnText(int column) const {
  const auto* text = sqlite3_column_text(stmt_.get(), column);
  if (text == nullptr) {
    return {};
  }
  const int bytes = sqlite3_column_bytes(stmt_.get(), column);
  // sqlite3_column_text returns `const unsigned char*`; reinterpret is the
  // documented way to view it as chars.
  return {reinterpret_cast<const char*>(text), static_cast<std::size_t>(bytes)};
}

bool Statement::columnIsNull(int column) const {
  return sqlite3_column_type(stmt_.get(), column) == SQLITE_NULL;
}

// --- Database ----------------------------------------------------------------

void Database::Closer::operator()(sqlite3* db) const noexcept {
  // sqlite3_close_v2 defers closing until outstanding statements finalize,
  // so a stray Statement can never cause a crash here.
  sqlite3_close_v2(db);
}

Database::Database(OpenTag /*tag*/, const std::string& sqliteFilename) {
  sqlite3* raw = nullptr;
  const int rc = sqlite3_open_v2(sqliteFilename.c_str(), &raw,
                                 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  db_.reset(raw);  // Take ownership even on failure: SQLite may allocate.
  if (rc != SQLITE_OK) {
    throw DatabaseError(describe(raw, "open " + sqliteFilename, rc), rc);
  }
  execute("PRAGMA foreign_keys = ON;");
}

Database::Database(const std::filesystem::path& path) : Database(OpenTag{}, utf8(path)) {
  execute("PRAGMA journal_mode = WAL;");
}

Database Database::openInMemory() { return Database(OpenTag{}, ":memory:"); }

void Database::execute(std::string_view sql) {
  const std::string copy{sql};  // sqlite3_exec needs a NUL-terminated string.
  char* error = nullptr;
  const int rc = sqlite3_exec(db_.get(), copy.c_str(), nullptr, nullptr, &error);
  if (rc != SQLITE_OK) {
    std::string message = "execute: ";
    message += (error != nullptr) ? error : sqlite3_errstr(rc);
    sqlite3_free(error);
    throw DatabaseError(message, rc);
  }
}

Statement Database::prepare(std::string_view sql) {
  sqlite3_stmt* stmt = nullptr;
  const int rc = sqlite3_prepare_v2(db_.get(), sql.data(), checkedLength(sql), &stmt, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_finalize(stmt);
    throw DatabaseError(describe(db_.get(), "prepare", rc), rc);
  }
  return Statement{db_.get(), stmt};
}

std::int64_t Database::lastInsertRowId() const noexcept {
  return sqlite3_last_insert_rowid(db_.get());
}

int Database::changes() const noexcept { return sqlite3_changes(db_.get()); }

int Database::userVersion() {
  auto stmt = prepare("PRAGMA user_version;");
  stmt.step();
  return stmt.columnInt(0);
}

void Database::setUserVersion(int version) {
  // PRAGMA arguments cannot be bound as parameters, so format the integer.
  execute("PRAGMA user_version = " + std::to_string(version) + ";");
}

// --- Transaction -------------------------------------------------------------

Transaction::Transaction(Database& db) : db_(db) { db_.execute("BEGIN IMMEDIATE;"); }

Transaction::~Transaction() {
  if (!done_) {
    try {
      db_.execute("ROLLBACK;");
    } catch (...) {  // NOLINT(bugprone-empty-catch): destructors must not throw
      // Destructors must not throw. If rollback fails the connection is
      // already in a bad state and the next call will report it.
    }
  }
}

void Transaction::commit() {
  db_.execute("COMMIT;");
  done_ = true;
}

}  // namespace glimmer
