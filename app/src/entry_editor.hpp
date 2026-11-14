#pragma once

#include "glimmer/entry.hpp"

#include <QWidget>

class QLabel;
class QLineEdit;
class QVBoxLayout;

/// Base class for the three ways of journaling. It provides the shared
/// chrome (date label, title field, Save/Cancel) and leaves the body to
/// subclasses.
class EntryEditor : public QWidget {
  Q_OBJECT

 public:
  explicit EntryEditor(const QString& heading, QWidget* parent = nullptr);

  /// Shows `entry` for editing. Subclasses call the base first.
  virtual void loadEntry(const glimmer::Entry& entry);

  /// Copies what's on screen into `entry` (title, body, media...).
  virtual void writeTo(glimmer::Entry& entry) const;

 signals:
  void saveRequested();
  void cancelRequested();

 protected:
  /// Subclasses add their widgets here.
  [[nodiscard]] QVBoxLayout* contentLayout() const { return content_; }

 private:
  QLabel* heading_;
  QLabel* date_;
  QLineEdit* title_;
  QVBoxLayout* content_;
  QString headingText_;
};
