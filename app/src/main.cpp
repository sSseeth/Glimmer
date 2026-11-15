#include "diary_store.hpp"
#include "main_window.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QMessageBox>
#include <QStandardPaths>

#include <exception>
#include <memory>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setOrganizationName(QStringLiteral("glimmer"));
  QApplication::setApplicationName(QStringLiteral("glimmer"));
  QApplication::setApplicationVersion(QStringLiteral(GLIMMER_VERSION));
  QApplication::setWindowIcon(QIcon(QStringLiteral(":/resources/glimmer.png")));
  QGuiApplication::setDesktopFileName(QStringLiteral("glimmer"));  // Wayland icon lookup

  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("A small, private desktop diary."));
  parser.addHelpOption();
  parser.addVersionOption();
  const QCommandLineOption dataDirOption(
      QStringLiteral("data-dir"),
      QStringLiteral("Store the diary in <dir> instead of the default location."),
      QStringLiteral("dir"));
  parser.addOption(dataDirOption);
  parser.process(app);

  const QString dataDir = parser.isSet(dataDirOption)
                              ? parser.value(dataDirOption)
                              : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

  std::unique_ptr<DiaryStore> store;
  try {
    store = std::make_unique<DiaryStore>(dataDir);
  } catch (const std::exception& e) {
    QMessageBox::critical(nullptr, QStringLiteral("glimmer"),
                          QApplication::tr("Couldn't open your diary in %1:\n\n%2")
                              .arg(dataDir, QString::fromUtf8(e.what())));
    return 1;
  }

  MainWindow window(*store);
  window.show();
  return QApplication::exec();
}
