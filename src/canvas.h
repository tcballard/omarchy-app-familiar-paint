#pragma once
#include "document.h"
#include <QWidget>
#include <QColor>

class Canvas : public QWidget {
    Q_OBJECT
public:
    enum class Tool { Brush, Eraser, Fill, Line, Arrow, Rectangle, Ellipse, Text, Select, Picker };
    explicit Canvas(Paint::Document *document, QWidget *parent = nullptr);
    void setDocument(Paint::Document *document);
    void setTool(Tool tool);
    Tool tool() const { return tool_; }
    void setColour(QColor colour) { colour_ = colour; }
    QColor colour() const { return colour_; }
    void setStroke(int width) { stroke_ = width; }
    int strokeWidth() const { return stroke_; }
    int textSize() const { return textSize_; }
    void setTextSize(int size) { textSize_ = size; }
    void setFilled(bool filled) { filled_ = filled; }
    void setZoom(double zoom);
    double zoom() const { return zoom_; }
    void syncSize();
    void cancelDrag();
    bool isDrawing() const { return dragging_; }
    QRect selection() const { return selection_; }
    void clearSelection();
    void addText(const QPoint &position, const QString &text);
    static QImage floodFill(QImage image, QPoint point, QColor colour);
signals:
    void changed();
    void colourPicked(QColor colour);
    void textRequested(QPoint point);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    bool event(QEvent *) override;
private:
    QPoint imagePoint(const QMouseEvent *event) const;
    void drawTo(QPoint point);
    Paint::Document *document_;
    Tool tool_ = Tool::Brush;
    QColor colour_ = QColor("#20242b");
    int stroke_ = 5, textSize_ = 20;
    double zoom_ = 1;
    bool filled_ = false, dragging_ = false;
    QPoint start_, last_;
    QImage preview_;
    QRect selection_;
};
