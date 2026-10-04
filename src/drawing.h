#pragma once
#include <QImage>
#include <QColor>
#include <QFont>
#include <QPoint>
#include <QString>
namespace Paint {
void drawPrimitive(QImage &image, const QString &kind, QPoint from, QPoint to, QColor colour, int width, bool filled = false);
void drawText(QImage &image, QPoint at, const QString &text, QColor colour, QFont font, int size);
QImage floodFill(QImage image, QPoint point, QColor colour);
}
