#include "canvas.h"
#include "drawing.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <cmath>

Canvas::Canvas(Paint::Document *document, QWidget *parent) : QWidget(parent), document_(document) {
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName("Drawing canvas");
    setCursor(Qt::CrossCursor);
    syncSize();
}
void Canvas::setDocument(Paint::Document *document) { cancelDrag(); document_ = document; clearSelection(); syncSize(); }
void Canvas::setTool(Tool tool) { cancelDrag(); tool_ = tool; }
void Canvas::setZoom(double zoom) { cancelDrag(); zoom_ = zoom; syncSize(); }
void Canvas::syncSize() {
    setFixedSize(qRound(document_->image().width() * zoom_), qRound(document_->image().height() * zoom_));
    update();
}
void Canvas::clearSelection() { selection_ = {}; update(); }
void Canvas::cancelDrag() { dragging_ = false; preview_ = {}; update(); }
QPoint Canvas::imagePoint(const QMouseEvent *event) const {
    auto point = event->position() / zoom_;
    return {qBound(0, qRound(point.x()), document_->image().width() - 1),
            qBound(0, qRound(point.y()), document_->image().height() - 1)};
}
void Canvas::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.scale(zoom_, zoom_);
    QPixmap tile(32, 32);
    tile.fill(Qt::white);
    { QPainter checker(&tile); checker.fillRect(0, 0, 16, 16, QColor("#e3e5e8")); checker.fillRect(16, 16, 16, 16, QColor("#e3e5e8")); }
    p.fillRect(document_->image().rect(), QBrush(tile));
    p.drawImage(0, 0, preview_.isNull() ? document_->image() : preview_);
    if (!selection_.isEmpty()) {
        p.setPen(QPen(QColor("#147ad6"), 1 / zoom_, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(selection_);
    }
}
QImage Canvas::floodFill(QImage image, QPoint point, QColor colour) { return Paint::floodFill(image, point, colour); }
void Canvas::addText(const QPoint &point, const QString &text) {
    if (text.isEmpty()) return;
    auto image = document_->image();
    Paint::drawText(image, point, text, colour_, font(), textSize_);
    document_->commit(image); emit changed(); update();
}
void Canvas::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) return;
    setFocus();
    auto point = imagePoint(event);
    if (tool_ == Tool::Picker) { emit colourPicked(document_->image().pixelColor(point)); return; }
    if (tool_ == Tool::Text) { emit textRequested(point); return; }
    if (tool_ == Tool::Fill) {
        document_->commit(floodFill(document_->image(), point, colour_)); emit changed(); update(); return;
    }
    clearSelection();
    start_ = last_ = point;
    dragging_ = true;
    preview_ = document_->image();
    if (tool_ == Tool::Brush || tool_ == Tool::Eraser) drawTo(point);
    update();
}
void Canvas::drawTo(QPoint point) {
    if (tool_ == Tool::Select) { selection_ = QRect(start_, point).normalized(); update(); return; }
    const bool freehand = tool_ == Tool::Brush || tool_ == Tool::Eraser;
    if (!freehand) preview_ = document_->image();
    QString kind = "line";
    if (tool_ == Tool::Rectangle) kind = "rect";
    else if (tool_ == Tool::Ellipse) kind = "ellipse";
    else if (tool_ == Tool::Arrow) kind = "arrow";
    Paint::drawPrimitive(preview_, kind, freehand ? last_ : start_, point,
                         tool_ == Tool::Eraser ? QColor(Qt::white) : colour_, stroke_, filled_);
    last_ = point;
}
void Canvas::mouseMoveEvent(QMouseEvent *event) { if (dragging_) { drawTo(imagePoint(event)); update(); } }
void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || !dragging_) return;
    drawTo(imagePoint(event));
    if (tool_ != Tool::Select) document_->commit(preview_);
    cancelDrag(); emit changed();
}
void Canvas::focusOutEvent(QFocusEvent *event) { cancelDrag(); QWidget::focusOutEvent(event); }
bool Canvas::event(QEvent *event) {
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide) cancelDrag();
    return QWidget::event(event);
}
void Canvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) { cancelDrag(); clearSelection(); emit changed(); }
    else QWidget::keyPressEvent(event);
}
