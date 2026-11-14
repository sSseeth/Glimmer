#include "text_editor.hpp"

#include "qt_convert.hpp"

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QVBoxLayout>

TextEditor::TextEditor(QWidget* parent)
    : EntryEditor(tr("Written entry"), parent), body_(new QPlainTextEdit(this)) {
  body_->setPlaceholderText(tr("What happened today?"));
  body_->setTabChangesFocus(false);
  contentLayout()->addWidget(body_);
}

void TextEditor::loadEntry(const glimmer::Entry& entry) {
  EntryEditor::loadEntry(entry);
  body_->setPlainText(glimmer::qt::toQString(entry.body));
  body_->setFocus();
}

void TextEditor::writeTo(glimmer::Entry& entry) const {
  EntryEditor::writeTo(entry);
  entry.body = glimmer::qt::toStd(body_->toPlainText());
}

void TextEditor::insertPrompt(const QString& prompt) {
  QString quoted;
  for (const auto& line : prompt.split(QLatin1Char('\n'))) {
    quoted += QStringLiteral("> ") + line + QLatin1Char('\n');
  }
  body_->moveCursor(QTextCursor::Start);
  body_->insertPlainText(quoted + QLatin1Char('\n'));
  body_->moveCursor(QTextCursor::End);
  body_->setFocus();
}
