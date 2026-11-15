#include "main_window.hpp"

#include "ai_panel.hpp"
#include "collage_editor.hpp"
#include "diary_store.hpp"
#include "omniroute_client.hpp"
#include "qt_convert.hpp"
#include "settings.hpp"
#include "text_editor.hpp"
#include "voice_editor.hpp"

#include <QAction>
#include <QApplication>
#include <QCalendarWidget>
#include <QDockWidget>
#include <QFont>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextCharFormat>
#include <QVBoxLayout>

namespace {

constexpr int kEntryIdRole = Qt::UserRole;

QString kindSymbol(glimmer::EntryKind kind) {
  switch (kind) {
    case glimmer::EntryKind::Text:
      return QStringLiteral("✎");
    case glimmer::EntryKind::Voice:
      return QStringLiteral("♪");
    case glimmer::EntryKind::Collage:
      return QStringLiteral("▦");
  }
  return {};
}

QString entryLabel(const glimmer::Entry& entry, bool withDate) {
  QString text = glimmer::qt::toQString(entry.title);
  if (text.isEmpty()) {
    text = glimmer::qt::toQString(entry.body).section(QLatin1Char('\n'), 0, 0).left(60);
  }
  if (text.isEmpty()) {
    text = QObject::tr("(untitled)");
  }
  QString label = kindSymbol(entry.kind) + QLatin1Char(' ') + text;
  if (withDate) {
    label = glimmer::qt::toQString(glimmer::toIsoString(entry.date)) + QStringLiteral("  ") + label;
  }
  return label;
}

}  // namespace

MainWindow::MainWindow(DiaryStore& store, QWidget* parent)
    : QMainWindow(parent), store_(store), client_(new OmniRouteClient(this)) {
  setWindowTitle(QStringLiteral("glimmer"));
  buildUi();
  buildMenus();
  onDateSelected();
  refreshCalendarMarks();
  statusBar()->showMessage(tr("Diary folder: %1").arg(store_.dataDir()), 8000);
  resize(1280, 800);
}

// --- UI construction ---------------------------------------------------------

void MainWindow::buildUi() {
  // Left column: calendar, day's entries, new-entry buttons, search.
  calendar_ = new QCalendarWidget(this);
  calendar_->setGridVisible(false);
  calendar_->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
  calendar_->setFirstDayOfWeek(Qt::Monday);
  connect(calendar_, &QCalendarWidget::selectionChanged, this, &MainWindow::onDateSelected);
  connect(calendar_, &QCalendarWidget::currentPageChanged, this,
          [this](int, int) { refreshCalendarMarks(); });

  dayLabel_ = new QLabel(this);
  QFont labelFont = dayLabel_->font();
  labelFont.setBold(true);
  dayLabel_->setFont(labelFont);

  dayList_ = new QListWidget(this);
  connect(dayList_, &QListWidget::itemActivated, this,
          [this](QListWidgetItem* item) { openEntry(item->data(kEntryIdRole).toLongLong()); });
  connect(dayList_, &QListWidget::itemClicked, this,
          [this](QListWidgetItem* item) { openEntry(item->data(kEntryIdRole).toLongLong()); });

  auto* newText = new QPushButton(tr("✎ Write"), this);
  auto* newVoice = new QPushButton(tr("♪ Voice"), this);
  auto* newCollage = new QPushButton(tr("▦ Collage"), this);
  newText->setToolTip(tr("New written entry (Ctrl+N)"));
  connect(newText, &QPushButton::clicked, this, [this] { newEntry(glimmer::EntryKind::Text); });
  connect(newVoice, &QPushButton::clicked, this, [this] { newEntry(glimmer::EntryKind::Voice); });
  connect(newCollage, &QPushButton::clicked, this,
          [this] { newEntry(glimmer::EntryKind::Collage); });
  auto* newRow = new QHBoxLayout;
  newRow->addWidget(newText);
  newRow->addWidget(newVoice);
  newRow->addWidget(newCollage);

  search_ = new QLineEdit(this);
  search_->setPlaceholderText(tr("Search all entries…"));
  search_->setClearButtonEnabled(true);
  connect(search_, &QLineEdit::returnPressed, this, &MainWindow::runSearch);
  connect(search_, &QLineEdit::textChanged, this, [this](const QString& text) {
    if (text.isEmpty() && showingSearch_) {
      showingSearch_ = false;
      refreshDayList();
    }
  });

  auto* left = new QWidget(this);
  auto* leftLayout = new QVBoxLayout(left);
  leftLayout->addWidget(calendar_);
  leftLayout->addLayout(newRow);
  leftLayout->addWidget(dayLabel_);
  leftLayout->addWidget(dayList_, 1);
  leftLayout->addWidget(search_);

  // Centre: one editor per entry kind, plus an empty-state placeholder.
  auto* emptyState = new QLabel(tr("Pick a day, then write, record or make a collage."), this);
  emptyState->setAlignment(Qt::AlignCenter);
  emptyState->setStyleSheet(QStringLiteral("color: palette(mid);"));
  placeholder_ = emptyState;

  textEditor_ = new TextEditor(this);
  voiceEditor_ =
      new VoiceEditor(store_.media(), [this](QString* error) { return transcriber(error); }, this);
  collageEditor_ = new CollageEditor(store_.media(), this);

  stack_ = new QStackedWidget(this);
  stack_->addWidget(placeholder_);
  for (EntryEditor* editor :
       {static_cast<EntryEditor*>(textEditor_), static_cast<EntryEditor*>(voiceEditor_),
        static_cast<EntryEditor*>(collageEditor_)}) {
    stack_->addWidget(editor);
    connect(editor, &EntryEditor::saveRequested, this, &MainWindow::saveCurrent);
    connect(editor, &EntryEditor::cancelRequested, this, &MainWindow::closeEditor);
  }

  auto* splitter = new QSplitter(this);
  splitter->addWidget(left);
  splitter->addWidget(stack_);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  splitter->setSizes({360, 900});
  setCentralWidget(splitter);

  // Right: the AI panel lives in its own dock so it's visibly separate from
  // the diary itself and can be hidden entirely.
  aiPanel_ = new AiPanel(store_, *client_, this);
  connect(aiPanel_, &AiPanel::usePromptRequested, this, [this](const QString& prompt) {
    newEntry(glimmer::EntryKind::Text);
    textEditor_->insertPrompt(prompt);
  });
  auto* dock = new QDockWidget(tr("AI companion"), this);
  dock->setObjectName(QStringLiteral("aiDock"));
  dock->setWidget(aiPanel_);
  dock->setMinimumWidth(320);
  addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::buildMenus() {
  auto* file = menuBar()->addMenu(tr("&File"));
  auto* newText = file->addAction(tr("New &written entry"));
  newText->setShortcut(QKeySequence::New);
  connect(newText, &QAction::triggered, this, [this] { newEntry(glimmer::EntryKind::Text); });
  auto* newVoice = file->addAction(tr("New &voice entry"));
  connect(newVoice, &QAction::triggered, this, [this] { newEntry(glimmer::EntryKind::Voice); });
  auto* newCollage = file->addAction(tr("New &collage"));
  connect(newCollage, &QAction::triggered, this, [this] { newEntry(glimmer::EntryKind::Collage); });
  file->addSeparator();
  auto* remove = file->addAction(tr("&Delete selected entry"));
  remove->setShortcut(QKeySequence::Delete);
  remove->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  dayList_->addAction(remove);
  connect(remove, &QAction::triggered, this, &MainWindow::deleteSelected);
  file->addSeparator();
  auto* settings = file->addAction(tr("&Settings…"));
  settings->setShortcut(QKeySequence::Preferences);
  connect(settings, &QAction::triggered, this, &MainWindow::showSettings);
  auto* quit = file->addAction(tr("&Quit"));
  quit->setShortcut(QKeySequence::Quit);
  connect(quit, &QAction::triggered, qApp, &QApplication::quit);

  auto* edit = menuBar()->addMenu(tr("&Edit"));
  auto* find = edit->addAction(tr("&Search"));
  find->setShortcut(QKeySequence::Find);
  connect(find, &QAction::triggered, this, [this] {
    search_->setFocus();
    search_->selectAll();
  });
  auto* today = edit->addAction(tr("Go to &today"));
  today->setShortcut(QKeySequence(tr("Ctrl+T")));
  connect(today, &QAction::triggered, this,
          [this] { calendar_->setSelectedDate(QDate::currentDate()); });

  auto* help = menuBar()->addMenu(tr("&Help"));
  connect(help->addAction(tr("&About glimmer")), &QAction::triggered, this, &MainWindow::showAbout);
}

// --- Calendar and lists ------------------------------------------------------

void MainWindow::onDateSelected() {
  aiPanel_->setSelectedDate(calendar_->selectedDate());
  showingSearch_ = false;
  refreshDayList();
}

void MainWindow::refreshDayList() {
  if (showingSearch_) {
    runSearch();
    return;
  }
  const QDate date = calendar_->selectedDate();
  dayLabel_->setText(QLocale().toString(date, QLocale::LongFormat));
  dayList_->clear();
  try {
    for (const auto& entry : store_.entries().findByDate(glimmer::qt::fromQDate(date))) {
      auto* item = new QListWidgetItem(entryLabel(entry, false), dayList_);
      item->setData(kEntryIdRole, QVariant::fromValue<qlonglong>(entry.id));
    }
  } catch (const std::exception& e) {
    statusBar()->showMessage(glimmer::qt::toQString(e.what()));
  }
  if (dayList_->count() == 0) {
    auto* item = new QListWidgetItem(tr("Nothing yet."), dayList_);
    item->setFlags(Qt::NoItemFlags);
  }
}

void MainWindow::refreshCalendarMarks() {
  // Cover the visible grid, which spills into neighbouring months.
  const QDate first(calendar_->yearShown(), calendar_->monthShown(), 1);
  const auto from = glimmer::addDays(glimmer::qt::fromQDate(first), -7);
  const auto to = glimmer::addDays(glimmer::lastDayOfMonth(glimmer::qt::fromQDate(first)), 14);

  calendar_->setDateTextFormat(QDate(), QTextCharFormat());  // clear all
  try {
    for (const auto& day : store_.entries().daySummaries(from, to)) {
      QTextCharFormat format;
      format.setFontWeight(QFont::Bold);
      // One tint per "strongest" kind, translucent so it works in dark mode.
      QColor tint = day.hasCollage ? QColor(76, 175, 80)
                    : day.hasVoice ? QColor(255, 152, 0)
                                   : QColor(66, 133, 244);
      tint.setAlpha(90);
      format.setBackground(tint);

      QStringList kinds;
      if (day.hasText) {
        kinds << tr("written");
      }
      if (day.hasVoice) {
        kinds << tr("voice");
      }
      if (day.hasCollage) {
        kinds << tr("collage");
      }
      format.setToolTip(
          tr("%1 entries: %2").arg(day.entryCount).arg(kinds.join(QStringLiteral(", "))));
      calendar_->setDateTextFormat(glimmer::qt::toQDate(day.date), format);
    }
  } catch (const std::exception& e) {
    statusBar()->showMessage(glimmer::qt::toQString(e.what()));
  }
}

void MainWindow::runSearch() {
  const QString query = search_->text().trimmed();
  if (query.isEmpty()) {
    return;
  }
  showingSearch_ = true;
  dayList_->clear();
  try {
    const auto results = store_.entries().search(glimmer::qt::toStd(query));
    dayLabel_->setText(
        tr("%1 results for \"%2\"").arg(static_cast<qulonglong>(results.size())).arg(query));
    for (const auto& entry : results) {
      auto* item = new QListWidgetItem(entryLabel(entry, true), dayList_);
      item->setData(kEntryIdRole, QVariant::fromValue<qlonglong>(entry.id));
    }
  } catch (const std::exception& e) {
    statusBar()->showMessage(glimmer::qt::toQString(e.what()));
  }
}

// --- Editing -----------------------------------------------------------------

EntryEditor* MainWindow::editorFor(glimmer::EntryKind kind) {
  switch (kind) {
    case glimmer::EntryKind::Text:
      return textEditor_;
    case glimmer::EntryKind::Voice:
      return voiceEditor_;
    case glimmer::EntryKind::Collage:
      return collageEditor_;
  }
  return textEditor_;
}

void MainWindow::newEntry(glimmer::EntryKind kind) {
  current_ = glimmer::Entry{};
  current_.date = glimmer::qt::fromQDate(calendar_->selectedDate());
  current_.kind = kind;
  activeEditor_ = editorFor(kind);
  activeEditor_->loadEntry(current_);
  stack_->setCurrentWidget(activeEditor_);
}

void MainWindow::openEntry(std::int64_t id) {
  if (activeEditor_ != nullptr && current_.id == id) {
    return;  // Already open; don't clobber unsaved edits.
  }
  try {
    const auto entry = store_.entries().findById(id);
    if (!entry) {
      return;
    }
    current_ = *entry;
    activeEditor_ = editorFor(current_.kind);
    activeEditor_->loadEntry(current_);
    stack_->setCurrentWidget(activeEditor_);
  } catch (const std::exception& e) {
    QMessageBox::warning(this, tr("Couldn't open entry"), glimmer::qt::toQString(e.what()));
  }
}

void MainWindow::saveCurrent() {
  if (activeEditor_ == nullptr) {
    return;
  }
  activeEditor_->writeTo(current_);
  try {
    if (current_.id == 0) {
      store_.entries().create(current_);
    } else {
      store_.entries().update(current_);
    }
  } catch (const std::exception& e) {
    QMessageBox::critical(this, tr("Couldn't save"), glimmer::qt::toQString(e.what()));
    return;
  }
  activeEditor_->loadEntry(current_);  // Picks up ids/timestamps.
  statusBar()->showMessage(tr("Saved"), 2000);
  refreshDayList();
  refreshCalendarMarks();
}

void MainWindow::closeEditor() {
  activeEditor_ = nullptr;
  current_ = glimmer::Entry{};
  stack_->setCurrentWidget(placeholder_);
  dayList_->clearSelection();
}

void MainWindow::deleteSelected() {
  auto* item = dayList_->currentItem();
  if (item == nullptr || !item->flags().testFlag(Qt::ItemIsEnabled)) {
    return;
  }
  const auto id = item->data(kEntryIdRole).toLongLong();
  const auto answer = QMessageBox::question(
      this, tr("Delete entry?"), tr("Delete \"%1\"? This can't be undone.").arg(item->text()));
  if (answer != QMessageBox::Yes) {
    return;
  }
  try {
    if (const auto entry = store_.entries().findById(id)) {
      store_.entries().remove(id);
      // Remove the entry's media after the row is gone.
      auto removeQuietly = [this](const std::string& path) {
        if (path.empty()) {
          return;
        }
        try {
          store_.media().remove(path);
        } catch (const std::exception&) {
          // A missing file is fine; we wanted it gone anyway.
        }
      };
      removeQuietly(entry->audioPath);
      for (const auto& image : entry->collage) {
        removeQuietly(image.imagePath);
      }
    }
  } catch (const std::exception& e) {
    QMessageBox::critical(this, tr("Couldn't delete"), glimmer::qt::toQString(e.what()));
    return;
  }
  if (current_.id == id) {
    closeEditor();
  }
  refreshDayList();
  refreshCalendarMarks();
}

// --- Services ----------------------------------------------------------------

std::shared_ptr<glimmer::Transcriber> MainWindow::transcriber(QString* error) {
  const AppSettings settings;
  const QString modelPath = settings.whisperModelPath();
  if (!glimmer::whisperAvailable()) {
    *error = tr("This build has no transcription support.");
    return nullptr;
  }
  if (modelPath.isEmpty()) {
    *error = tr("Choose a whisper model file in Settings first.");
    return nullptr;
  }
  if (transcriber_ && transcriberModel_ == modelPath) {
    return transcriber_;
  }
  try {
    statusBar()->showMessage(tr("Loading speech model…"));
    transcriber_ = glimmer::makeWhisperTranscriber(glimmer::qt::toPath(modelPath));
    transcriberModel_ = modelPath;
    statusBar()->clearMessage();
    return transcriber_;
  } catch (const std::exception& e) {
    statusBar()->clearMessage();
    *error = glimmer::qt::toQString(e.what());
    return nullptr;
  }
}

void MainWindow::showSettings() {
  SettingsDialog dialog(this);
  dialog.exec();
  aiPanel_->setSelectedDate(calendar_->selectedDate());
}

void MainWindow::showAbout() {
  QMessageBox::about(
      this, tr("About glimmer"),
      tr("<h3>glimmer %1</h3>"
         "<p>A small, private desktop diary.</p>"
         "<p>Your entries live in <code>%2</code>. AI features are optional and only "
         "contact the gateway you configure.</p>"
         "<p>Transcription: %3</p>")
          .arg(QStringLiteral(GLIMMER_VERSION), store_.dataDir().toHtmlEscaped(),
               glimmer::whisperAvailable() ? tr("whisper.cpp (local)") : tr("not included")));
}
