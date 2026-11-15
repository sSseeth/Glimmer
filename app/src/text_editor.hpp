#pragma once

#include "entry_editor.hpp"

class QPlainTextEdit;

/// Plain typed journaling. Markdown is stored as written.
class TextEditor : public EntryEditor {
  Q_OBJECT

 public:
  explicit TextEditor(QWidget* parent = nullptr);

  void loadEntry(const glimmer::Entry& entry) override;
  void writeTo(glimmer::Entry& entry) const override;

  /// Used by "Use this prompt" in the AI panel.
  void insertPrompt(const QString& prompt);

 private:
  QPlainTextEdit* body_;
};
