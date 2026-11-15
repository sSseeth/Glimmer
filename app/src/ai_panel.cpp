#include "ai_panel.hpp"

#include "glimmer/date.hpp"

#include "diary_store.hpp"
#include "omniroute_client.hpp"
#include "qt_convert.hpp"
#include "settings.hpp"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace {

constexpr int kNoteIdRole = Qt::UserRole;
constexpr int kRecentDays = 14;

QString kindLabel(glimmer::AiNoteKind kind) {
  switch (kind) {
    case glimmer::AiNoteKind::Summary:
      return QObject::tr("Summary");
    case glimmer::AiNoteKind::Prompt:
      return QObject::tr("Prompt");
    case glimmer::AiNoteKind::Answer:
      return QObject::tr("Answer");
  }
  return {};
}

}  // namespace

AiPanel::AiPanel(DiaryStore& store, OmniRouteClient& client, QWidget* parent)
    : QWidget(parent),
      store_(store),
      client_(client),
      selectedDate_(QDate::currentDate()),
      gatewayLabel_(new QLabel(this)),
      weekButton_(new QPushButton(tr("Summarise week"), this)),
      monthButton_(new QPushButton(tr("Summarise month"), this)),
      promptButton_(new QPushButton(tr("Prompt me"), this)),
      usePromptButton_(new QPushButton(tr("Write about this"), this)),
      question_(new QLineEdit(this)),
      askButton_(new QPushButton(tr("Ask"), this)),
      output_(new QTextBrowser(this)),
      history_(new QListWidget(this)),
      status_(new QLabel(this)) {
  gatewayLabel_->setWordWrap(true);
  gatewayLabel_->setStyleSheet(QStringLiteral("color: palette(mid); font-size: small;"));
  question_->setPlaceholderText(tr("Ask about your entries…"));
  output_->setOpenExternalLinks(false);
  output_->setPlaceholderText(
      tr("AI summaries and prompts appear here. Nothing is sent until "
         "you press a button."));
  usePromptButton_->setVisible(false);
  status_->setWordWrap(true);
  history_->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(weekButton_, &QPushButton::clicked, this, &AiPanel::summariseWeek);
  connect(monthButton_, &QPushButton::clicked, this, &AiPanel::summariseMonth);
  connect(promptButton_, &QPushButton::clicked, this, &AiPanel::promptMe);
  connect(askButton_, &QPushButton::clicked, this, &AiPanel::ask);
  connect(question_, &QLineEdit::returnPressed, this, &AiPanel::ask);
  connect(usePromptButton_, &QPushButton::clicked, this,
          [this] { emit usePromptRequested(lastPrompt_); });

  connect(history_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* item) {
    if (item == nullptr) {
      return;
    }
    const auto id = item->data(kNoteIdRole).toLongLong();
    for (const auto& note : store_.aiNotes().recent()) {
      if (note.id == id) {
        showNote(note);
        break;
      }
    }
  });
  connect(history_, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
    auto* item = history_->itemAt(pos);
    if (item == nullptr) {
      return;
    }
    QMenu menu(this);
    auto* remove = menu.addAction(tr("Delete"));
    if (menu.exec(history_->viewport()->mapToGlobal(pos)) == remove) {
      store_.aiNotes().remove(item->data(kNoteIdRole).toLongLong());
      reloadHistory();
    }
  });

  auto* buttons = new QGridLayout;
  buttons->addWidget(weekButton_, 0, 0);
  buttons->addWidget(monthButton_, 0, 1);
  buttons->addWidget(promptButton_, 1, 0, 1, 2);

  auto* askRow = new QHBoxLayout;
  askRow->addWidget(question_);
  askRow->addWidget(askButton_);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(gatewayLabel_);
  layout->addLayout(buttons);
  layout->addLayout(askRow);
  layout->addWidget(output_, 3);
  layout->addWidget(usePromptButton_);
  layout->addWidget(status_);
  layout->addWidget(new QLabel(tr("History"), this));
  layout->addWidget(history_, 1);

  reloadHistory();
  setBusy(false);
}

void AiPanel::setSelectedDate(QDate date) { selectedDate_ = date; }

void AiPanel::summariseWeek() {
  const auto day = glimmer::qt::fromQDate(selectedDate_);
  const auto from = glimmer::startOfWeek(day);
  const auto to = glimmer::addDays(from, 6);
  const AppSettings settings;
  const glimmer::ai::PromptBuilder builder{glimmer::qt::toStd(settings.aiModel())};

  glimmer::AiNote note;
  note.kind = glimmer::AiNoteKind::Summary;
  note.rangeStart = glimmer::toIsoString(from);
  note.rangeEnd = glimmer::toIsoString(to);
  note.request = glimmer::qt::toStd(
      tr("Summary of the week starting %1").arg(glimmer::qt::toQString(note.rangeStart)));
  run(builder.summarise(store_.entries().findInRange(from, to), from, to), std::move(note));
}

void AiPanel::summariseMonth() {
  const auto day = glimmer::qt::fromQDate(selectedDate_);
  const auto from = glimmer::firstDayOfMonth(day);
  const auto to = glimmer::lastDayOfMonth(day);
  const AppSettings settings;
  const glimmer::ai::PromptBuilder builder{glimmer::qt::toStd(settings.aiModel())};

  glimmer::AiNote note;
  note.kind = glimmer::AiNoteKind::Summary;
  note.rangeStart = glimmer::toIsoString(from);
  note.rangeEnd = glimmer::toIsoString(to);
  note.request = glimmer::qt::toStd(
      tr("Summary of %1").arg(selectedDate_.toString(QStringLiteral("MMMM yyyy"))));
  run(builder.summarise(store_.entries().findInRange(from, to), from, to), std::move(note));
}

void AiPanel::promptMe() {
  const auto today = glimmer::qt::fromQDate(QDate::currentDate());
  const auto recent = store_.entries().findInRange(glimmer::addDays(today, -kRecentDays), today);
  const AppSettings settings;
  const glimmer::ai::PromptBuilder builder{glimmer::qt::toStd(settings.aiModel())};

  glimmer::AiNote note;
  note.kind = glimmer::AiNoteKind::Prompt;
  note.request = glimmer::qt::toStd(tr("Journaling prompt"));
  run(builder.journalingPrompt(recent, today), std::move(note));
}

void AiPanel::ask() {
  const auto question = question_->text().trimmed();
  if (question.isEmpty()) {
    return;
  }
  // Context: the 90 days up to the selected date. Enough for most questions
  // and keeps prompts a sensible size.
  const auto to = glimmer::qt::fromQDate(selectedDate_);
  const auto from = glimmer::addDays(to, -90);
  const AppSettings settings;
  const glimmer::ai::PromptBuilder builder{glimmer::qt::toStd(settings.aiModel())};

  glimmer::AiNote note;
  note.kind = glimmer::AiNoteKind::Answer;
  note.rangeStart = glimmer::toIsoString(from);
  note.rangeEnd = glimmer::toIsoString(to);
  note.request = glimmer::qt::toStd(question);
  run(builder.ask(note.request, store_.entries().findInRange(from, to)), std::move(note));
}

void AiPanel::run(const glimmer::ai::ChatRequest& request, glimmer::AiNote note) {
  const AppSettings settings;
  setBusy(true);
  status_->setText(tr("Asking %1…").arg(settings.aiBaseUrl()));

  // The reply can arrive after this panel is gone (e.g. on quit); QPointer
  // turns into nullptr when the widget is destroyed.
  const QPointer<AiPanel> self(this);
  client_.send(
      settings.aiBaseUrl(), settings.aiApiKey(), request,
      [self, this, note](const glimmer::ai::ChatResult& result) mutable {
        if (self.isNull()) {
          return;
        }
        note.content = result.content;
        note.model = result.model;
        try {
          store_.aiNotes().add(note);
        } catch (const std::exception& e) {
          status_->setText(
              tr("Couldn't save the result: %1").arg(glimmer::qt::toQString(e.what())));
        }
        if (note.kind == glimmer::AiNoteKind::Answer) {
          question_->clear();
        }
        showNote(note);
        reloadHistory();
        setBusy(false);
        status_->clear();
      },
      [self, this](const QString& error) {
        if (self.isNull()) {
          return;
        }
        setBusy(false);
        status_->setText(error);
      });
}

void AiPanel::showNote(const glimmer::AiNote& note) {
  QString markdown =
      QStringLiteral("**%1** · %2\n\n")
          .arg(glimmer::qt::toQString(note.request).toHtmlEscaped(),
               glimmer::qt::toQString(note.model.empty() ? std::string{"AI"} : note.model));
  markdown += glimmer::qt::toQString(note.content);
  output_->setMarkdown(markdown);

  const bool isPrompt = note.kind == glimmer::AiNoteKind::Prompt;
  lastPrompt_ = isPrompt ? glimmer::qt::toQString(note.content) : QString();
  usePromptButton_->setVisible(isPrompt);
}

void AiPanel::reloadHistory() {
  const QSignalBlocker blocker(history_);
  history_->clear();
  for (const auto& note : store_.aiNotes().recent()) {
    auto* item = new QListWidgetItem(
        QStringLiteral("%1 · %2").arg(kindLabel(note.kind), glimmer::qt::toQString(note.request)),
        history_);
    item->setData(kNoteIdRole, QVariant::fromValue<qlonglong>(note.id));
    item->setToolTip(glimmer::qt::toQString(note.createdAt));
  }
}

void AiPanel::setBusy(bool busy) {
  const AppSettings settings;
  gatewayLabel_->setText(
      tr("Gateway: %1 · model: %2").arg(settings.aiBaseUrl().toHtmlEscaped(), settings.aiModel()));
  for (auto* button : {weekButton_, monthButton_, promptButton_, askButton_}) {
    button->setEnabled(!busy);
  }
  question_->setEnabled(!busy);
}
