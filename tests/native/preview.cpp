#include "window.h"
#include "theme.h"
#include <QApplication>
#include <QTest>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setStyle("Fusion"); applyTheme(&app);
    Window w; w.show(); app.processEvents();
    auto canvas = w.canvas();
    auto stroke = [canvas](Canvas::Tool tool, QPoint a, QPoint b, QColor colour, int width = 5, bool fill = false) {
        canvas->setTool(tool); canvas->setColour(colour); canvas->setStroke(width); canvas->setFilled(fill);
        QTest::mousePress(canvas, Qt::LeftButton, {}, a);
        QTest::mouseRelease(canvas, Qt::LeftButton, {}, b);
    };
    // A reproducible sample drawn through the application's own tools.
    stroke(Canvas::Tool::Fill, {1, 1}, {1, 1}, QColor("#f6f3ec"), 1, true);
    canvas->setColour(QColor("#20242b")); canvas->setTextSize(44);
    canvas->addText({64, 55}, "Make yourself at home.");
    canvas->setColour(QColor("#66716b")); canvas->setTextSize(20);
    canvas->addText({67, 119}, "A little colour. A familiar canvas.");
    stroke(Canvas::Tool::Ellipse, {736, 185}, {823, 272}, QColor("#efc95b"), 4, true);
    stroke(Canvas::Tool::Line, {174, 499}, {821, 499}, QColor("#65ac80"), 5);
    stroke(Canvas::Tool::Rectangle, {295, 330}, {539, 497}, QColor("#cddfd2"), 5, true);
    stroke(Canvas::Tool::Line, {266, 339}, {417, 225}, QColor("#20242b"), 7);
    stroke(Canvas::Tool::Line, {417, 225}, {566, 339}, QColor("#20242b"), 7);
    stroke(Canvas::Tool::Rectangle, {327, 364}, {381, 419}, QColor("#548bd4"), 5, true);
    stroke(Canvas::Tool::Rectangle, {453, 364}, {506, 419}, QColor("#548bd4"), 5, true);
    stroke(Canvas::Tool::Rectangle, {394, 419}, {437, 495}, QColor("#ef9147"), 4, true);
    stroke(Canvas::Tool::Arrow, {718, 370}, {565, 403}, QColor("#e24e4b"), 4);
    canvas->setColour(QColor("#20242b")); canvas->setTextSize(20);
    canvas->addText({725, 324}, "You are here.");
    canvas->setColour(QColor("#66716b")); canvas->setTextSize(16);
    canvas->addText({66, 567}, "Draw · annotate · save · carry on");
    canvas->setTool(Canvas::Tool::Brush);
    canvas->setColour(QColor("#20242b")); canvas->setStroke(5); canvas->setFilled(false);
    w.refresh();
    if (argc > 3) w.resize(QString::fromLocal8Bit(argv[2]).toInt(), QString::fromLocal8Bit(argv[3]).toInt());
    app.processEvents();
    w.capture(argc > 1 ? QString::fromLocal8Bit(argv[1]) : "preview.png");
    return 0;
}
