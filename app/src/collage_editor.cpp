#include "collage_editor.hpp"

#include "qt_convert.hpp"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kPathRole = 0;  // QGraphicsItem::data key
constexpr double kCanvasWidth = 1600.0;
constexpr double kCanvasHeight = 1200.0;
constexpr double kNewImageMaxSide = 360.0;  // fit new images to this size

QPixmap missingPixmap() {
  QPixmap pixmap(240, 160);
  pixmap.fill(Qt::lightGray);
  QPainter painter(&pixmap);
  painter.drawText(pixmap.rect(), Qt::AlignCenter, QObject::tr("image missing"));
  return pixmap;
}

QString imageFileFilter() {
  QStringList patterns;
  for (const auto& format : QImageReader::supportedImageFormats()) {
    patterns << QStringLiteral("*.") + QString::fromLatin1(format);
  }
  return QObject::tr("Images (%1)").arg(patterns.join(QLatin1Char(' ')));
}

}  // namespace

CollageEditor::CollageEditor(glimmer::MediaLibrary& media, QWidget* parent)
    : EntryEditor(tr("Collage"), parent),
      media_(media),
      scene_(new QGraphicsScene(0, 0, kCanvasWidth, kCanvasHeight, this)),
      view_(new QGraphicsView(scene_, this)),
      caption_(new QLineEdit(this)) {
  scene_->setBackgroundBrush(QColor(250, 247, 240));
  view_->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  view_->setDragMode(QGraphicsView::RubberBandDrag);
  // Let file drops bubble up to this widget instead of the scene.
  view_->setAcceptDrops(false);
  view_->viewport()->setAcceptDrops(false);
  setAcceptDrops(true);

  caption_->setPlaceholderText(tr("Caption (optional)"));

  auto* add = new QPushButton(tr("Add images…"), this);
  auto* bigger = new QPushButton(tr("Bigger"), this);
  auto* smaller = new QPushButton(tr("Smaller"), this);
  auto* rotate = new QPushButton(tr("Rotate"), this);
  auto* front = new QPushButton(tr("To front"), this);
  auto* remove = new QPushButton(tr("Remove"), this);
  connect(add, &QPushButton::clicked, this, &CollageEditor::addImages);
  connect(bigger, &QPushButton::clicked, this, [this] { scaleSelected(1.15); });
  connect(smaller, &QPushButton::clicked, this, [this] { scaleSelected(1.0 / 1.15); });
  connect(rotate, &QPushButton::clicked, this, [this] { rotateSelected(15.0); });
  connect(front, &QPushButton::clicked, this, &CollageEditor::raiseSelected);
  connect(remove, &QPushButton::clicked, this, &CollageEditor::removeSelected);

  auto* tools = new QHBoxLayout;
  tools->addWidget(add);
  tools->addSpacing(12);
  tools->addWidget(bigger);
  tools->addWidget(smaller);
  tools->addWidget(rotate);
  tools->addWidget(front);
  tools->addWidget(remove);
  tools->addStretch();

  contentLayout()->addLayout(tools);
  contentLayout()->addWidget(view_, 1);
  contentLayout()->addWidget(caption_);
}

void CollageEditor::loadEntry(const glimmer::Entry& entry) {
  EntryEditor::loadEntry(entry);
  scene_->clear();
  caption_->setText(glimmer::qt::toQString(entry.body));
  for (const auto& item : entry.collage) {
    addItem(item);
  }
}

void CollageEditor::writeTo(glimmer::Entry& entry) const {
  EntryEditor::writeTo(entry);
  entry.body = glimmer::qt::toStd(caption_->text().trimmed());
  entry.collage.clear();
  for (auto* graphicsItem : scene_->items(Qt::AscendingOrder)) {
    auto* pixmapItem = qgraphicsitem_cast<QGraphicsPixmapItem*>(graphicsItem);
    if (pixmapItem == nullptr) {
      continue;
    }
    glimmer::CollageItem item;
    item.imagePath = glimmer::qt::toStd(pixmapItem->data(kPathRole).toString());
    item.x = pixmapItem->pos().x();
    item.y = pixmapItem->pos().y();
    item.scale = pixmapItem->scale();
    item.rotationDegrees = pixmapItem->rotation();
    item.z = static_cast<int>(pixmapItem->zValue());
    entry.collage.push_back(item);
  }
}

void CollageEditor::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void CollageEditor::dropEvent(QDropEvent* event) {
  QStringList paths;
  for (const auto& url : event->mimeData()->urls()) {
    if (url.isLocalFile()) {
      paths << url.toLocalFile();
    }
  }
  importFiles(paths);
  event->acceptProposedAction();
}

void CollageEditor::addImages() {
  const auto start = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
  importFiles(QFileDialog::getOpenFileNames(this, tr("Add images"), start, imageFileFilter()));
}

void CollageEditor::importFiles(const QStringList& paths) {
  QStringList failures;
  double offset = 0.0;
  for (const auto& path : paths) {
    if (QImageReader(path).format().isEmpty()) {
      failures << path;
      continue;
    }
    try {
      glimmer::CollageItem item;
      item.imagePath = media_.importImage(glimmer::qt::toPath(path));
      item.x = 40.0 + offset;
      item.y = 40.0 + offset;
      item.z = static_cast<int>(topZ()) + 1;
      item.scale = 0.0;  // 0 = "fit automatically" for new images
      addItem(item);
      offset += 30.0;
    } catch (const std::exception& e) {
      failures << QStringLiteral("%1 (%2)").arg(path, glimmer::qt::toQString(e.what()));
    }
  }
  if (!failures.isEmpty()) {
    QMessageBox::warning(this, tr("Some images were not added"), failures.join(QLatin1Char('\n')));
  }
}

void CollageEditor::addItem(const glimmer::CollageItem& item) {
  QPixmap pixmap;
  try {
    pixmap.load(glimmer::qt::fromPath(media_.resolve(item.imagePath)));
  } catch (const std::exception&) {
    // Invalid stored path; fall through to the placeholder.
  }
  if (pixmap.isNull()) {
    pixmap = missingPixmap();
  }

  auto* graphicsItem = scene_->addPixmap(pixmap);
  graphicsItem->setData(kPathRole, glimmer::qt::toQString(item.imagePath));
  graphicsItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
  graphicsItem->setTransformationMode(Qt::SmoothTransformation);
  graphicsItem->setTransformOriginPoint(pixmap.rect().center());
  graphicsItem->setPos(item.x, item.y);
  graphicsItem->setRotation(item.rotationDegrees);
  graphicsItem->setZValue(item.z);

  double scale = item.scale;
  if (scale <= 0.0) {
    const double longest = std::max(pixmap.width(), pixmap.height());
    scale = longest > kNewImageMaxSide ? kNewImageMaxSide / longest : 1.0;
  }
  graphicsItem->setScale(scale);
}

void CollageEditor::scaleSelected(double factor) {
  for (auto* item : scene_->selectedItems()) {
    item->setScale(std::clamp(item->scale() * factor, 0.05, 10.0));
  }
}

void CollageEditor::rotateSelected(double degrees) {
  for (auto* item : scene_->selectedItems()) {
    item->setRotation(item->rotation() + degrees);
  }
}

void CollageEditor::raiseSelected() {
  double z = topZ();
  for (auto* item : scene_->selectedItems()) {
    item->setZValue(++z);
  }
}

void CollageEditor::removeSelected() {
  // Files stay in the media library until the entry itself is deleted, so a
  // removal can still be undone by closing without saving.
  for (auto* item : scene_->selectedItems()) {
    scene_->removeItem(item);
    delete item;
  }
}

double CollageEditor::topZ() const {
  double z = 0.0;
  for (const auto* item : scene_->items()) {
    z = std::max(z, item->zValue());
  }
  return z;
}
