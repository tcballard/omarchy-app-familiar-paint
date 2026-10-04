#pragma once
#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>
#include <QUuid>

namespace Paint {
constexpr qint64 MaxPixels = 16'000'000;
constexpr qint64 HistoryBudget = 192 * 1024 * 1024;
void validateSize(int width, int height);
QImage blank(int width = 1000, int height = 650);
QImage readImage(const QString &path);
void atomicSave(const QImage &image, const QString &path);
QByteArray fingerprint(const QString &path);

class Document {
public:
    explicit Document(QImage image = blank());
    const QImage &image() const { return image_; }
    bool commit(const QImage &image);
    void undo();
    void redo();
    void crop(QRect rect);
    void resize(int width, int height);
    bool dirty() const { return ids_[index_] != savedId_; }
    bool canUndo() const { return index_ > 0; }
    bool canRedo() const { return index_ + 1 < history_.size(); }
    void markSaved(QString path, QByteArray hash);
    void markUnsaved() { savedId_ = -1; }
    void discardChanges() { savedId_ = ids_[index_]; }
    const QString &path() const { return path_; }
    const QByteArray &diskHash() const { return diskHash_; }
    qint64 historyBytes() const;
    QString revision() const { return identity_ + ":" + QString::number(revision_); }
private:
    QString identity_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    quint64 revision_ = 0;
    QImage image_;
    QVector<QImage> history_;
    QVector<qint64> ids_;
    qsizetype index_ = 0;
    qint64 serial_ = 0, savedId_ = 0;
    QString path_;
    QByteArray diskHash_;
};
}
