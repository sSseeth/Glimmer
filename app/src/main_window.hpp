#pragma once

#include "glimmer/entry.hpp"
#include "glimmer/transcriber.hpp"

#include <QMainWindow>

#include <cstdint>
#include <memory>

class AiPanel;
class CollageEditor;
class DiaryStore;
class EntryEditor;
class OmniRouteClient;
class QCalendarWidget;
class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class TextEditor;
class VoiceEditor;

/// Layout:  [calendar + day list] | [editor] | [AI panel (dock)]
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(DiaryStore& store, QWidget* parent = nullptr);

 private:
  void buildUi();
  void buildMenus();

  void onDateSelected();
  void refreshDayList();
  void refreshCalendarMarks();
  void runSearch();

  void newEntry(glimmer::EntryKind kind);
  void openEntry(std::int64_t id);
  void saveCurrent();
  void closeEditor();
  void deleteSelected();

  EntryEditor* editorFor(glimmer::EntryKind kind);
  std::shared_ptr<glimmer::Transcriber> transcriber(QString* error);
  void showSettings();
  void showAbout();

  DiaryStore& store_;
  OmniRouteClient* client_ = nullptr;

  QCalendarWidget* calendar_ = nullptr;
  QLabel* dayLabel_ = nullptr;
  QListWidget* dayList_ = nullptr;
  QLineEdit* search_ = nullptr;
  QStackedWidget* stack_ = nullptr;
  QWidget* placeholder_ = nullptr;
  TextEditor* textEditor_ = nullptr;
  VoiceEditor* voiceEditor_ = nullptr;
  CollageEditor* collageEditor_ = nullptr;
  AiPanel* aiPanel_ = nullptr;

  EntryEditor* activeEditor_ = nullptr;
  glimmer::Entry current_;  ///< The entry open in activeEditor_.
  bool showingSearch_ = false;

  std::shared_ptr<glimmer::Transcriber> transcriber_;
  QString transcriberModel_;  ///< Model path transcriber_ was loaded from.
};
