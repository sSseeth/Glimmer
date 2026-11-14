#pragma once

#include "glimmer/ai.hpp"
#include "glimmer/ai_note_repository.hpp"

#include <QDate>
#include <QWidget>

class DiaryStore;
class OmniRouteClient;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTextBrowser;

/// The AI side panel. Everything the model writes appears here and is stored
/// in the ai_notes table, never mixed into the user's own entries.
class AiPanel : public QWidget {
  Q_OBJECT

 public:
  AiPanel(DiaryStore& store, OmniRouteClient& client, QWidget* parent = nullptr);

  /// The date the calendar is on; "this week/month" are relative to it.
  void setSelectedDate(QDate date);

 signals:
  /// Emitted when the user wants to start a written entry from a prompt.
  void usePromptRequested(const QString& prompt);

 private:
  void summariseWeek();
  void summariseMonth();
  void promptMe();
  void ask();
  void run(const glimmer::ai::ChatRequest& request, glimmer::AiNote note);
  void showNote(const glimmer::AiNote& note);
  void reloadHistory();
  void setBusy(bool busy);

  DiaryStore& store_;
  OmniRouteClient& client_;
  QDate selectedDate_;
  QString lastPrompt_;

  QLabel* gatewayLabel_;
  QPushButton* weekButton_;
  QPushButton* monthButton_;
  QPushButton* promptButton_;
  QPushButton* usePromptButton_;
  QLineEdit* question_;
  QPushButton* askButton_;
  QTextBrowser* output_;
  QListWidget* history_;
  QLabel* status_;
};
