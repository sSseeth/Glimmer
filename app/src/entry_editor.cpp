#include "entry_editor.hpp"

#include "qt_convert.hpp"

#include <QFont>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

EntryEditor::EntryEditor(const QString& heading, QWidget* parent)
    : QWidget(parent),
      heading_(new QLabel(this)),
      date_(new QLabel(this)),
      title_(new QLineEdit(this)),
      content_(new QVBoxLayout),
      headingText_(heading) {
  QFont headingFont = heading_->font();
  headingFont.setPointSizeF(headingFont.pointSizeF() * 1.3);
  headingFont.setBold(true);
  heading_->setFont(headingFont);
  heading_->setText(heading);
  date_->setStyleSheet(QStringLiteral("color: palette(mid);"));
  title_->setPlaceholderText(tr("Title (optional)"));

  auto* save = new QPushButton(tr("Save"), this);
  save->setDefault(true);
  save->setShortcut(QKeySequence::Save);
  save->setToolTip(tr("Save (%1)").arg(QKeySequence(QKeySequence::Save).toString()));
  auto* cancel = new QPushButton(tr("Close"), this);
  connect(save, &QPushButton::clicked, this, &EntryEditor::saveRequested);
  connect(cancel, &QPushButton::clicked, this, &EntryEditor::cancelRequested);

  auto* header = new QHBoxLayout;
  header->addWidget(heading_);
  header->addStretch();
  header->addWidget(date_);

  auto* buttons = new QHBoxLayout;
  buttons->addStretch();
  buttons->addWidget(cancel);
  buttons->addWidget(save);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(header);
  layout->addWidget(title_);
  layout->addLayout(content_, 1);
  layout->addLayout(buttons);
}

void EntryEditor::loadEntry(const glimmer::Entry& entry) {
  heading_->setText(entry.id == 0 ? tr("New %1").arg(headingText_.toLower()) : headingText_);
  date_->setText(QLocale().toString(glimmer::qt::toQDate(entry.date), QLocale::LongFormat));
  title_->setText(glimmer::qt::toQString(entry.title));
}

void EntryEditor::writeTo(glimmer::Entry& entry) const {
  entry.title = glimmer::qt::toStd(title_->text().trimmed());
}
