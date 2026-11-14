#pragma once

#include "glimmer/media_library.hpp"

#include "entry_editor.hpp"

class QDragEnterEvent;
class QDropEvent;
class QGraphicsScene;
class QGraphicsView;
class QLineEdit;

/// A free-form canvas: drop images on it, drag them around, resize, rotate
/// and stack them. Positions are saved per image (see glimmer::CollageItem).
class CollageEditor : public EntryEditor {
  Q_OBJECT

 public:
  explicit CollageEditor(glimmer::MediaLibrary& media, QWidget* parent = nullptr);

  void loadEntry(const glimmer::Entry& entry) override;
  void writeTo(glimmer::Entry& entry) const override;

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;

 private:
  void addImages();
  void importFiles(const QStringList& paths);
  void addItem(const glimmer::CollageItem& item);
  void scaleSelected(double factor);
  void rotateSelected(double degrees);
  void raiseSelected();
  void removeSelected();
  [[nodiscard]] double topZ() const;

  glimmer::MediaLibrary& media_;
  QGraphicsScene* scene_;
  QGraphicsView* view_;
  QLineEdit* caption_;
};
