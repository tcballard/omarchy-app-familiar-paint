#include "window.h"
#include "theme.h"
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLockFile>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QPainter>
#include <QPainterPath>
#include <QtConcurrent>
#include <stdexcept>

static QIcon toolIcon(Canvas::Tool tool) {
    QIcon icon;
    for (int scale : {1, 2, 3}) {
        QPixmap pix(32 * scale, 32 * scale); pix.setDevicePixelRatio(scale); pix.fill(Qt::transparent);
        QPainter p(&pix); p.setRenderHint(QPainter::Antialiasing);
        const QColor ink("#d9ddd9"), accent("#d8b866");
        p.setPen(QPen(ink, 1.7, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        switch (tool) {
        case Canvas::Tool::Select: p.setPen(QPen(ink, 1.7, Qt::DashLine)); p.drawRect(5, 6, 22, 20); break;
        case Canvas::Tool::Picker: p.drawLine(9, 24, 22, 11); p.drawLine(7, 22, 9, 24); p.drawLine(7, 22, 6, 27); p.drawLine(6, 27, 11, 26); p.drawLine(17, 10, 24, 17); p.setPen(QPen(accent, 5)); p.drawLine(22, 9, 26, 5); break;
        case Canvas::Tool::Eraser: { QPolygon poly; poly << QPoint(5,21) << QPoint(18,7) << QPoint(27,15) << QPoint(17,26) << QPoint(11,26); p.setBrush(QColor("#c58b92")); p.drawPolygon(poly); p.setPen(QPen(ink, 2)); p.drawLine(8,18,19,27); break; }
        case Canvas::Tool::Fill: { p.setBrush(QColor("#72a5b2")); QPolygon poly; poly << QPoint(5,16) << QPoint(16,5) << QPoint(27,16) << QPoint(16,27); p.drawPolygon(poly); p.setPen(QPen(ink,2)); p.drawLine(4,16,25,16); p.drawLine(12,3,12,10); break; }
        case Canvas::Tool::Brush: p.setPen(QPen(accent,5,Qt::SolidLine,Qt::RoundCap)); p.drawLine(15,17,26,5); p.setPen(QPen(ink,2)); p.setBrush(ink); { QPainterPath path; path.moveTo(15,16); path.cubicTo(5,17,12,25,4,27); path.cubicTo(17,29,20,23,18,20); path.closeSubpath(); p.drawPath(path); } break;
        case Canvas::Tool::Text: { QFont f = p.font(); f.setFamily("serif"); f.setPixelSize(29); f.setBold(true); p.setFont(f); p.drawText(QRect(0,0,32,32),Qt::AlignCenter,"A"); break; }
        case Canvas::Tool::Line: p.drawLine(6,26,26,6); break;
        case Canvas::Tool::Arrow: p.drawLine(5,27,26,6); p.drawLine(14,6,26,6); p.drawLine(26,6,26,18); break;
        case Canvas::Tool::Rectangle: p.drawRect(5,7,22,18); break;
        case Canvas::Tool::Ellipse: p.drawEllipse(QRect(4,7,24,18)); break;
        }
        p.end(); icon.addPixmap(pix);
    }
    return icon;
}

Window::Window(QWidget *parent) : QMainWindow(parent), canvas_(new Canvas(&document_, this)), scroll_(new QScrollArea(this)) {
    reportError = [this](QString message) { QMessageBox::warning(this, "Familiar Paint", message); };
    scroll_->setWidget(canvas_);
    scroll_->setAlignment(Qt::AlignCenter);
    resize(1280, 900);
    setMinimumSize(800, 580);
    connect(canvas_, &Canvas::changed, this, &Window::refresh);
    connect(canvas_, &Canvas::colourPicked, this, &Window::setColour);
    connect(canvas_, &Canvas::textRequested, this, [this](QPoint point) {
        bool ok = false;
        auto text = QInputDialog::getMultiLineText(this, "Add text", "Text:", {}, &ok);
        if (ok) canvas_->addText(point, text);
    });
    makeControls();
    refresh();
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [] { applyTheme(qApp); });
    timer->start(2000);
}
Window::~Window() { watcher_.waitForFinished(); }
QAction *Window::action(QWidget *container, const QString &name, const QKeySequence &shortcut, std::function<void()> callback) {
    auto result = new QAction(name, this);
    if (!shortcut.isEmpty()) result->setShortcut(shortcut);
    connect(result, &QAction::triggered, this, [this, callback] { if (!busy_) { canvas_->cancelDrag(); callback(); } });
    container->addAction(result);
    return result;
}
void Window::makeControls() {
    auto file = menuBar()->addMenu("&File");
    action(file, "New", QKeySequence::New, [this] { create(); });
    action(file, "Open…", QKeySequence::Open, [this] { open(); });
    action(file, "Save", QKeySequence::Save, [this] { save(); });
    action(file, "Save As…", QKeySequence::SaveAs, [this] { save(true); });
    file->addSeparator();
    action(file, "Quit", QKeySequence::Quit, [this] { close(); });
    auto edit = menuBar()->addMenu("&Edit");
    action(edit, "Undo", QKeySequence::Undo, [this] { history(false); });
    action(edit, "Redo", QKeySequence("Ctrl+Shift+Z"), [this] { history(true); });
    action(edit, "Copy", QKeySequence::Copy, [this] { copy(); });
    action(edit, "Paste as new image", QKeySequence::Paste, [this] { paste(); });
    auto image = menuBar()->addMenu("&Image");
    action(image, "Crop to selection", QKeySequence("Ctrl+Shift+X"), [this] { crop(); });
    action(image, "Resize…", QKeySequence("Ctrl+R"), [this] { resizeImage(); });
    action(image, "Rotate clockwise", {}, [this] {
        document_.commit(document_.image().transformed(QTransform().rotate(90))); canvas_->clearSelection(); refresh();
    });
    action(image, "Flip horizontally", {}, [this] { document_.commit(document_.image().mirrored(true, false)); refresh(); });
    auto help = menuBar()->addMenu("&Help");
    action(help, "About Familiar Paint", {}, [this] { QMessageBox::about(this, "Familiar Paint", "Familiar Paint 0.0.1 — development preview\n\nA small native painting app for Omarchy.\nC++ and Qt. No account or cloud.\n\nSave your work regularly: crash recovery is not yet implemented."); });
    // Classic Paint's two-column toolbox is the primary navigation.
    auto body = new QWidget(this);
    auto bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0); bodyLayout->setSpacing(0);
    auto toolbox = new QWidget(body); toolbox->setObjectName("toolbox"); toolbox->setFixedWidth(128);
    toolbox->setMinimumHeight(620);
    auto side = new QVBoxLayout(toolbox); side->setContentsMargins(12, 16, 12, 12); side->setSpacing(12);
    auto title = new QLabel("TOOLS", toolbox); title->setObjectName("sectionLabel"); side->addWidget(title);
    auto grid = new QGridLayout(); grid->setSpacing(4);
    auto group = new QActionGroup(this);
    const QList<QPair<QString, Canvas::Tool>> names{
        {"Select", Canvas::Tool::Select}, {"Picker", Canvas::Tool::Picker},
        {"Eraser", Canvas::Tool::Eraser}, {"Fill", Canvas::Tool::Fill},
        {"Brush", Canvas::Tool::Brush}, {"Text", Canvas::Tool::Text},
        {"Line", Canvas::Tool::Line}, {"Arrow", Canvas::Tool::Arrow},
        {"Rectangle", Canvas::Tool::Rectangle}, {"Ellipse", Canvas::Tool::Ellipse}};
    auto toolName = new QLabel("Brush", toolbox); toolName->setAlignment(Qt::AlignCenter);
    toolName->setObjectName("toolName");
    int index = 0;
    for (const auto &entry : names) {
        auto a = new QAction(toolIcon(entry.second), entry.first, this);
        a->setCheckable(true); a->setChecked(entry.second == Canvas::Tool::Brush);
        a->setToolTip(entry.first); group->addAction(a);
        auto button = new QToolButton(toolbox); button->setDefaultAction(a);
        button->setObjectName("drawingTool"); button->setAccessibleName(entry.first);
        button->setFixedSize(48, 44); button->setIconSize(QSize(28, 28));
        grid->addWidget(button, index / 2, index % 2); ++index;
        connect(a, &QAction::triggered, this, [this, entry, toolName] { canvas_->setTool(entry.second); toolName->setText(entry.first); refresh(); });
    }
    side->addLayout(grid); side->addWidget(toolName);
    auto divider = new QFrame(toolbox); divider->setFrameShape(QFrame::HLine); side->addWidget(divider);
    auto sampleLabel = new QLabel("STROKE", toolbox); sampleLabel->setObjectName("sectionLabel"); side->addWidget(sampleLabel);
    auto sample = new QLabel(toolbox); sample->setObjectName("strokePreview"); sample->setFixedSize(100, 72);
    sample->setAccessibleName("Current brush stroke preview"); side->addWidget(sample);
    auto size = new QSpinBox(toolbox); size->setRange(1, 80); size->setValue(5); size->setSuffix(" px");
    size->setAccessibleName("Stroke width"); side->addWidget(size);
    auto drawSample = [this, sample, size] {
        QPixmap pix(100, 72); pix.fill(QColor("#f4f1e9")); QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing); p.setPen(QPen(canvas_->colour(), qMin(size->value(), 36), Qt::SolidLine, Qt::RoundCap));
        QPainterPath path; path.moveTo(22, 48); path.cubicTo(30, 6, 70, 68, 78, 25); p.drawPath(path); p.end(); sample->setPixmap(pix);
    };
    connect(size, &QSpinBox::valueChanged, this, [this, drawSample](int width) { canvas_->setStroke(width); drawSample(); });
    connect(canvas_, &Canvas::colourPicked, this, [drawSample](QColor) { drawSample(); });
    connect(this, &Window::drawingColourChanged, sample, drawSample); drawSample();
    side->addStretch();
    auto shortcutHint = new QLabel("Esc to cancel\nCtrl+Z to undo", toolbox); shortcutHint->setObjectName("quietLabel"); side->addWidget(shortcutHint);
    auto toolboxScroll = new QScrollArea(body); toolboxScroll->setObjectName("toolboxScroll");
    toolboxScroll->setFrameShape(QFrame::NoFrame); toolboxScroll->setWidgetResizable(true);
    toolboxScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    toolboxScroll->setFixedWidth(144); toolboxScroll->setWidget(toolbox);
    bodyLayout->addWidget(toolboxScroll);
    scroll_->setObjectName("canvasWell"); scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    bodyLayout->addWidget(scroll_, 1); setCentralWidget(body);

    auto options = new QToolBar("Image and tool controls", this); options->setMovable(false); options->setObjectName("commandBar");
    addToolBar(options);
    auto brand = new QLabel("  Familiar Paint  ", this); brand->setObjectName("brandLabel"); options->addWidget(brand); options->addSeparator();
    action(options, "Open…", {}, [this] { open(); });
    action(options, "Save", {}, [this] { save(); }); options->addSeparator();
    action(options, "Undo", {}, [this] { history(false); });
    action(options, "Redo", {}, [this] { history(true); }); options->addSeparator();
    action(options, "Crop to selection", {}, [this] { crop(); });
    action(options, "Resize…", {}, [this] { resizeImage(); }); options->addSeparator();
    auto filled = new QCheckBox("Fill shapes", this); filled->setAccessibleName("Fill shapes");
    connect(filled, &QCheckBox::toggled, canvas_, &Canvas::setFilled); options->addWidget(filled);
    options->addSeparator(); options->addWidget(new QLabel(" Text ", this));
    auto textSize = new QSpinBox(this); textSize->setRange(8, 200); textSize->setValue(20); textSize->setSuffix(" px");
    textSize->setAccessibleName("Text size"); connect(textSize, &QSpinBox::valueChanged, canvas_, &Canvas::setTextSize); options->addWidget(textSize);

    auto palette = new QToolBar("Colour palette", this); palette->setObjectName("colourPalette"); palette->setMovable(false);
    addToolBar(Qt::BottomToolBarArea, palette);
    auto colourGroup = new QWidget(this); auto colourLayout = new QHBoxLayout(colourGroup); colourLayout->setContentsMargins(6, 0, 14, 0);
    colourButton_ = new QToolButton(this); colourButton_->setObjectName("activeColour"); colourButton_->setFixedSize(46, 46);
    colourButton_->setAccessibleName("Current drawing colour; choose a custom colour");
    connect(colourButton_, &QToolButton::clicked, this, [this] { auto c = QColorDialog::getColor(canvas_->colour(), this, "Drawing colour"); if (c.isValid()) setColour(c); });
    colourLayout->addWidget(colourButton_);
    auto colourInfo = new QVBoxLayout(); colourInfo->setSpacing(2);
    colourInfo->addWidget(new QLabel("COLOUR", this));
    auto colourValue = new QLabel(canvas_->colour().name().toUpper(), this); colourValue->setObjectName("colourValue"); colourInfo->addWidget(colourValue);
    colourLayout->addLayout(colourInfo); palette->addWidget(colourGroup);
    auto swatches = new QWidget(this); auto swatchGrid = new QGridLayout(swatches); swatchGrid->setContentsMargins(0, 0, 0, 0); swatchGrid->setSpacing(3);
    const QStringList colours{"#20242b", "#727880", "#8b3238", "#926332", "#858138", "#32674b", "#317877", "#315b91", "#514481", "#875481", "#4e3b30", "#a29783", "#cccccc", "#ffffff",
        "#ffffff", "#c3c9cf", "#e24e4b", "#ef9147", "#efc95b", "#65ac80", "#60bed0", "#548bd4", "#aa83ce", "#e888ad", "#bf9474", "#e5d8be", "#333333", "#000000"};
    index = 0;
    for (const auto &colour : colours) {
        auto button = new QToolButton(this); button->setObjectName("swatch"); button->setFixedSize(25, 25);
        button->setAccessibleName("Colour " + colour); button->setToolTip(colour);
        button->setStyleSheet(QString("QToolButton { background:%1; border:1px solid #8b9095; border-radius:0; padding:0; } QToolButton:hover, QToolButton:focus { border:2px solid #ffffff; }").arg(colour));
        connect(button, &QToolButton::clicked, this, [this, colour] { setColour(QColor(colour)); });
        swatchGrid->addWidget(button, index / 14, index % 14); ++index;
    }
    palette->addWidget(swatches); palette->addSeparator();
    action(palette, "Edit colours…", {}, [this] { auto c = QColorDialog::getColor(canvas_->colour(), this, "Drawing colour"); if (c.isValid()) setColour(c); });
    auto spacer = new QWidget(this); spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); palette->addWidget(spacer);
    auto paletteHint = new QLabel("Click a swatch to paint", this); paletteHint->setObjectName("quietLabel"); palette->addWidget(paletteHint);
    setColour(canvas_->colour());

    auto zoom = new QSlider(Qt::Horizontal, this); zoom->setRange(25, 400); zoom->setValue(100); zoom->setFixedWidth(136); zoom->setAccessibleName("Canvas zoom percent");
    auto zoomValue = new QLabel("100%", this); zoomValue->setMinimumWidth(44);
    connect(zoom, &QSlider::valueChanged, this, [this, zoomValue](int percent) { canvas_->setZoom(percent / 100.0); zoomValue->setText(QString::number(percent) + "%"); });
    auto fit = new QToolButton(this); fit->setText("Fit"); fit->setAccessibleName("Fit canvas in window");
    connect(fit, &QToolButton::clicked, this, [this, zoom] { auto area = scroll_->viewport()->size(); auto image = document_.image().size(); zoom->setValue(qBound(25, int(100 * qMin(double(area.width() - 48) / image.width(), double(area.height() - 48) / image.height())), 400)); });
    auto actual = new QToolButton(this); actual->setText("1:1"); actual->setAccessibleName("Actual size"); connect(actual, &QToolButton::clicked, zoom, [zoom] { zoom->setValue(100); });
    statusBar()->addPermanentWidget(fit); statusBar()->addPermanentWidget(actual); statusBar()->addPermanentWidget(zoom); statusBar()->addPermanentWidget(zoomValue);
}
void Window::setColour(QColor colour) {
    canvas_->setColour(colour);
    colourButton_->setStyleSheet("QToolButton { background:" + colour.name() + "; border:3px double #a7a9ac; border-radius:0; }");
    colourButton_->setToolTip("Drawing colour " + colour.name() + " — click to edit");
    if (auto label = findChild<QLabel *>("colourValue")) label->setText(colour.name().toUpper());
    emit drawingColourChanged();
}

void Window::refresh() {
    const QString name = document_.path().isEmpty() ? "Untitled" : QFileInfo(document_.path()).fileName();
    setWindowTitle((document_.dirty() ? "• " : "") + name + " — Familiar Paint");
    statusBar()->showMessage(QString("%1 × %2 px   ·   Drag to draw · Esc cancels").arg(document_.image().width()).arg(document_.image().height()));
    canvas_->syncSize();
    for (auto a : findChildren<QAction *>()) {
        if (a->text() == "Undo") a->setEnabled(document_.canUndo());
        if (a->text() == "Redo") a->setEnabled(document_.canRedo());
        if (a->text() == "Crop to selection") a->setEnabled(!canvas_->selection().isEmpty());
    }
}
void Window::work(std::function<IOResult()> task, std::function<void(IOResult)> callback) {
    if (busy_) return;
    canvas_->cancelDrag(); busy_ = true; setEnabled(false);
    disconnect(&watcher_, nullptr, this, nullptr);
    connect(&watcher_, &QFutureWatcher<IOResult>::finished, this, [this, callback] {
        const auto result = watcher_.result();
        busy_ = false; setEnabled(true);
        if (!result.error.isEmpty()) reportError(result.error); else callback(result);
        refresh();
    });
    watcher_.setFuture(QtConcurrent::run([task] {
        try { return task(); }
        catch (const std::exception &e) { IOResult r; r.error = QString::fromUtf8(e.what()); return r; }
    }));
}
void Window::guard(std::function<void()> continuation) {
    if (busy_) return;
    canvas_->cancelDrag();
    if (!document_.dirty()) { continuation(); return; }
    const auto answer = QMessageBox::question(this, "Save your work?", "Save changes before continuing?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) save(false, continuation);
    else if (answer == QMessageBox::Discard) continuation();
}
void Window::replace(QImage image, QString path, QByteArray hash) {
    document_ = Paint::Document(image);
    if (!path.isEmpty()) document_.markSaved(path, hash); else document_.markUnsaved();
    canvas_->setDocument(&document_); refresh();
}
std::optional<QSize> Window::dimensions(const QString &title) {
    QDialog dialog(this); dialog.setWindowTitle(title);
    QFormLayout layout(&dialog);
    QSpinBox width, height;
    width.setRange(1, 16384); height.setRange(1, 16384);
    width.setValue(document_.image().width()); height.setValue(document_.image().height());
    layout.addRow("Width (px)", &width); layout.addRow("Height (px)", &height);
    QLabel hint("Maximum 16 million pixels. Resize stretches to these dimensions.");
    layout.addRow(&hint);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout.addRow(&buttons);
    if (dialog.exec() != QDialog::Accepted) return {};
    Paint::validateSize(width.value(), height.value());
    return QSize(width.value(), height.value());
}
void Window::create() {
    guard([this] { try { auto size = dimensions("New canvas"); if (size) { replace(Paint::blank(size->width(), size->height())); document_.discardChanges(); refresh(); } }
        catch (const std::exception &e) { reportError(e.what()); } });
}
void Window::open(QString path) {
    guard([this, path] {
        auto selected = path.isEmpty() ? QFileDialog::getOpenFileName(this, "Open image", {}, "Images (*.png *.jpg *.jpeg *.bmp *.webp)") : path;
        if (selected.isEmpty()) return;
        selected = QFileInfo(selected).absoluteFilePath();
        work([selected] { IOResult r; const auto before = Paint::fingerprint(selected); r.image = Paint::readImage(selected); r.hash = Paint::fingerprint(selected);
              if (before != r.hash) throw std::runtime_error("This file changed while it was opening. Please try again."); return r; },
             [this, selected](IOResult r) { replace(r.image, selected, r.hash); });
    });
}
void Window::save(bool asNew, std::function<void()> after) {
    if (busy_) return;
    canvas_->cancelDrag();
    QString path = document_.path();
    if (asNew || path.isEmpty()) {
        QString filter;
        path = QFileDialog::getSaveFileName(this, "Save image", path.isEmpty() ? "Untitled.png" : path,
                    "PNG image (*.png);;JPEG image (*.jpg);;BMP image (*.bmp)", &filter);
        if (path.isEmpty()) return;
        if (QFileInfo(path).suffix().isEmpty()) {
            path += filter.startsWith("JPEG") ? ".jpg" : filter.startsWith("BMP") ? ".bmp" : ".png";
            if (QFileInfo::exists(path) && QMessageBox::question(this, "Replace image?", "Replace " + QFileInfo(path).fileName() + "?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
        }
    }
    path = QFileInfo(path).absoluteFilePath();
    auto snapshot = document_.image();
    const bool samePath = path == document_.path();
    const QByteArray expected = document_.diskHash();
    work([snapshot, path, samePath, expected] {
        QLockFile lock(path + ".familiar-paint.lock");
        if (!lock.tryLock(0)) throw std::runtime_error("Another Paint window is saving this file. Try again shortly.");
        if (samePath && Paint::fingerprint(path) != expected)
            throw std::runtime_error("This file changed outside Paint. Use Save As with a different filename to preserve both versions.");
        Paint::atomicSave(snapshot, path);
        IOResult r; r.hash = Paint::fingerprint(path); return r;
    }, [this, path, after](IOResult r) { document_.markSaved(path, r.hash); if (after) after(); });
}
void Window::history(bool redo) { canvas_->cancelDrag(); if (redo) document_.redo(); else document_.undo(); canvas_->clearSelection(); refresh(); }
void Window::crop() { try { document_.crop(canvas_->selection()); canvas_->clearSelection(); refresh(); } catch (const std::exception &e) { reportError(e.what()); } }
void Window::resizeImage() { try { auto size = dimensions("Resize image"); if (size) { document_.resize(size->width(), size->height()); canvas_->clearSelection(); refresh(); } } catch (const std::exception &e) { reportError(e.what()); } }
void Window::copy() { auto image = document_.image(); if (!canvas_->selection().isEmpty()) image = image.copy(canvas_->selection()); QApplication::clipboard()->setImage(image); }
void Window::paste() {
    const auto image = QApplication::clipboard()->image();
    if (image.isNull()) { reportError("The clipboard does not contain an image."); return; }
    try { Paint::validateSize(image.width(), image.height()); }
    catch (const std::exception &e) { reportError(e.what()); return; }
    guard([this, image] { replace(image); });
}
void Window::closeEvent(QCloseEvent *event) {
    if (busy_) { event->ignore(); return; }
    canvas_->cancelDrag();
    if (!document_.dirty()) { event->accept(); return; }
    event->ignore();
    guard([this] { document_.discardChanges(); close(); });
}
bool Window::event(QEvent *event) {
    if (event->type() == QEvent::WindowDeactivate && canvas_) canvas_->cancelDrag();
    return QMainWindow::event(event);
}
void Window::capture(const QString &path) { grab().save(path); }
