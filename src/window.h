#pragma once
#include "canvas.h"
#include <QMainWindow>
#include <QFutureWatcher>
#include <functional>
#include <optional>

class QToolButton;
class QScrollArea;
struct IOResult { QImage image; QByteArray hash; QString error; };

class Window : public QMainWindow {
    Q_OBJECT
public:
    explicit Window(QWidget *parent = nullptr);
    ~Window() override;
    Paint::Document &document() { return document_; }
    Canvas *canvas() { return canvas_; }
    bool busy() const { return busy_; }
    void replace(QImage image, QString path = {}, QByteArray hash = {});
    void open(QString path = {});
    void save(bool asNew = false, std::function<void()> after = {});
    void history(bool redo);
    void refresh();
    void copy();
    void crop();
    void capture(const QString &path);
    std::function<void(QString)> reportError;
signals:
    void drawingColourChanged();
protected:
    void closeEvent(QCloseEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    bool event(QEvent *) override;
private:
    QAction *action(QWidget *container, const QString &name, const QKeySequence &shortcut, std::function<void()> callback);
    void makeControls();
    void setColour(QColor colour);
    void guard(std::function<void()> continuation);
    void work(std::function<IOResult()> task, std::function<void(IOResult)> callback);
    void create();
    void paste();
    void resizeImage();
    std::optional<QSize> dimensions(const QString &title);
    Paint::Document document_;
    Canvas *canvas_;
    QScrollArea *scroll_;
    QToolButton *colourButton_;
    QFutureWatcher<IOResult> watcher_;
    bool busy_ = false;
};
