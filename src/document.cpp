#include "document.h"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QSaveFile>
#include <stdexcept>

namespace Paint {
static void fail(const QString &message) { throw std::runtime_error(message.toStdString()); }
void validateSize(int width, int height) {
    if (width < 1 || height < 1 || width > 16384 || height > 16384 || qint64(width) * height > MaxPixels)
        fail("Use dimensions from 1 to 16384, with at most 16 million pixels.");
}
QImage blank(int width, int height) {
    validateSize(width, height);
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    if (image.isNull()) fail("Not enough memory for this canvas.");
    image.fill(Qt::white);
    return image;
}
QImage readImage(const QString &path) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const auto size = reader.size();
    if (!size.isValid()) fail("Cannot read this image: " + reader.errorString());
    validateSize(size.width(), size.height());
    auto image = reader.read();
    if (image.isNull()) fail(reader.errorString());
    validateSize(image.width(), image.height());
    return image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}
void atomicSave(const QImage &image, const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    QByteArray format;
    if (suffix == "png") format = "PNG";
    else if (suffix == "jpg" || suffix == "jpeg") format = "JPEG";
    else if (suffix == "bmp") format = "BMP";
    else fail("Save as PNG, JPEG or BMP.");
    QImage output = image;
    if (format != "PNG") {
        output = blank(image.width(), image.height());
        QPainter painter(&output);
        painter.drawImage(0, 0, image);
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) fail(file.errorString());
    if (!output.save(&file, format.constData(), 95)) {
        file.cancelWriting();
        fail("The image encoder could not save this file.");
    }
    if (!file.commit()) fail(file.errorString());
}
QByteArray fingerprint(const QString &path) {
    QFile file(path);
    if (!file.exists()) return {};
    if (!file.open(QIODevice::ReadOnly)) fail(file.errorString());
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) fail("Could not read the existing file.");
    return hash.result();
}
Document::Document(QImage image) : image_(image.convertToFormat(QImage::Format_ARGB32_Premultiplied)) {
    validateSize(image.width(), image.height());
    history_.append(image_);
    ids_.append(0);
}
qint64 Document::historyBytes() const {
    qint64 bytes = 0;
    for (const auto &image : history_) bytes += image.sizeInBytes();
    return bytes;
}
bool Document::commit(const QImage &image) {
    validateSize(image.width(), image.height());
    if (image == image_) return false;
    history_.resize(index_ + 1);
    ids_.resize(index_ + 1);
    image_ = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    history_.append(image_);
    ids_.append(++serial_);
    ++revision_;
    ++index_;
    while (history_.size() > 2 && historyBytes() > HistoryBudget) {
        history_.removeFirst(); ids_.removeFirst(); --index_;
    }
    return true;
}
void Document::undo() { if (canUndo()) { image_ = history_[--index_]; ++revision_; } }
void Document::redo() { if (canRedo()) { image_ = history_[++index_]; ++revision_; } }
void Document::crop(QRect rect) {
    rect = rect.normalized().intersected(image_.rect());
    if (rect.isEmpty()) fail("Drag a selection on the canvas first.");
    commit(image_.copy(rect));
}
void Document::resize(int width, int height) {
    validateSize(width, height);
    commit(image_.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}
void Document::markSaved(QString path, QByteArray hash) {
    path_ = std::move(path); diskHash_ = std::move(hash); savedId_ = ids_[index_];
}
}
