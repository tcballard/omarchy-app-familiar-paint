#include "drawing.h"
#include <QPainter>
#include <cmath>
QImage Paint::floodFill(QImage image, QPoint point, QColor colour) {
    if (!image.rect().contains(point)) return image;
    image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QRgb old = image.pixel(point), replacement = qPremultiply(colour.rgba());
    if (old == replacement) return image;
    // Scanline fill: bounded by image size; direct row access keeps large flat areas fast.
    QVector<QPoint> stack{point};
    while (!stack.isEmpty()) {
        auto seed = stack.takeLast();
        auto row = reinterpret_cast<QRgb *>(image.scanLine(seed.y()));
        if (row[seed.x()] != old) continue;
        int left = seed.x(), right = seed.x();
        while (left > 0 && row[left - 1] == old) --left;
        while (right + 1 < image.width() && row[right + 1] == old) ++right;
        for (int x = left; x <= right; ++x) row[x] = replacement;
        for (int y : {seed.y() - 1, seed.y() + 1}) {
            if (y < 0 || y >= image.height()) continue;
            const auto adjacent = reinterpret_cast<const QRgb *>(image.constScanLine(y));
            bool span = false;
            for (int x = left; x <= right; ++x) {
                if (adjacent[x] == old) { if (!span) stack.append({x, y}); span = true; }
                else span = false;
            }
        }
    }
    return image;
}

void Paint::drawPrimitive(QImage &image, const QString &kind, QPoint from, QPoint to, QColor colour, int width, bool filled) {
    QPainter p(&image); p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(colour, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(filled ? QBrush(colour) : QBrush(Qt::NoBrush));
    const auto rect = QRect(from, to).normalized();
    if (kind == "rect") p.drawRect(rect);
    else if (kind == "ellipse") p.drawEllipse(rect);
    else {
        if (from == to) p.drawPoint(to); else p.drawLine(from, to);
        if (kind == "arrow" && from != to) {
            const double angle = std::atan2(to.y() - from.y(), to.x() - from.x());
            const double length = qMax(14, width * 3);
            for (double offset : {-0.5, 0.5}) p.drawLine(QPointF(to), QPointF(to.x() - length * std::cos(angle + offset), to.y() - length * std::sin(angle + offset)));
        }
    }
}
void Paint::drawText(QImage &image, QPoint at, const QString &text, QColor colour, QFont font, int size) {
    QPainter p(&image); p.setPen(colour); font.setPixelSize(size); p.setFont(font);
    p.drawText(QRect(at, image.size()), Qt::AlignLeft | Qt::AlignTop, text);
}
