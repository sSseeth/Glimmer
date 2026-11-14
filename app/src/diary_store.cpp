#include "diary_store.hpp"

#include "glimmer/migrations.hpp"

#include "qt_convert.hpp"

#include <QDir>

namespace {

glimmer::Database openAndMigrate(const QString& dataDir) {
  QDir().mkpath(dataDir);
  glimmer::Database db{
      glimmer::qt::toPath(QDir(dataDir).filePath(QStringLiteral("glimmer.sqlite")))};
  glimmer::migrate(db);
  return db;
}

}  // namespace

DiaryStore::DiaryStore(const QString& dataDir)
    : dataDir_(dataDir),
      db_(openAndMigrate(dataDir)),
      entries_(db_),
      aiNotes_(db_),
      media_(glimmer::qt::toPath(QDir(dataDir).filePath(QStringLiteral("media")))) {}
