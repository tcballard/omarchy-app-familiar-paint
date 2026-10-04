#include "theme.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPalette>
#include <QRegularExpression>
#include <QStandardPaths>

QString themePath() {
    QString base = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (!QDir::isAbsolutePath(base)) base = QDir::homePath() + "/.config";
    return base + "/omarchy/current/theme/colors.toml";
}
void applyTheme(QApplication *app) {
    QMap<QString, QColor> colours{{"background", QColor("#20242b")}, {"foreground", QColor("#e6e8eb")},
        {"lighter_background", QColor("#2c323b")}, {"selection", QColor("#435a50")}};
    QFile file(themePath());
    if (file.open(QIODevice::ReadOnly)) {
        // Read only the top-level quoted colour assignments used by Omarchy.
        const auto text = QString::fromUtf8(file.read(64 * 1024));
        const QRegularExpression expression(R"re(^\s*([a-z_]+)\s*=\s*"(#[0-9a-fA-F]{6})"\s*(?:#.*)?$)re", QRegularExpression::MultilineOption);
        auto matches = expression.globalMatch(text);
        while (matches.hasNext()) { auto match = matches.next(); if (colours.contains(match.captured(1))) colours[match.captured(1)] = QColor(match.captured(2)); }
    }
    QPalette palette;
    for (auto role : {QPalette::Window, QPalette::Base}) palette.setColor(role, colours["background"]);
    palette.setColor(QPalette::Button, colours["lighter_background"]);
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::HighlightedText}) palette.setColor(role, colours["foreground"]);
    palette.setColor(QPalette::Highlight, colours["selection"]);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#80858c"));
    if (app->palette() != palette) app->setPalette(palette);
    // Raised/sunken edges echo classic Paint without rasterising the interface.
    const QString panel = colours["lighter_background"].name();
    const QString base = colours["background"].name();
    const QString edge = colours["lighter_background"].lighter(155).name();
    const QString shadow = colours["background"].darker(160).name();
    const QString selection = colours["selection"].name();
    const QString foreground = colours["foreground"].name();
    const QString css = QString(R"css(
        QMainWindow { background: %1; }
        QMenuBar { background:%1; padding:5px 8px; border-bottom:1px solid %4; }
        QMenuBar::item { padding:5px 12px; }
        QMenuBar::item:selected { background:%5; }
        QToolBar { background:%2; spacing:6px; padding:10px 8px; border-top:1px solid %3; border-bottom:1px solid %4; }
        QToolBar::separator { background:%4; width:1px; margin:5px 8px; }
        QToolButton { padding:7px 10px; border-radius:2px; border:1px solid transparent; }
        QToolButton:hover { background:%5; border:1px solid %3; }
        QToolButton:focus { border:1px dashed %6; }
        QToolButton:pressed { border-top:1px solid %4; border-left:1px solid %4; background:%1; }
        QToolButton#qt_toolbar_ext_button { padding:0; min-width:20px; border:1px solid %3; }
        QWidget#toolbox { background:%2; border-right:1px solid %4; }
        QToolButton#drawingTool { background:#30373d; padding:4px; border-radius:0; border-top:1px solid #697278; border-left:1px solid #697278; border-bottom:1px solid #11161b; border-right:1px solid #11161b; }
        QToolButton#drawingTool:hover { background:#465058; }
        QToolButton#drawingTool:checked { background:#4a5b55; border-top:2px solid #11161b; border-left:2px solid #11161b; border-bottom:1px solid #a0b1a9; border-right:1px solid #a0b1a9; }
        QToolButton#drawingTool:focus { border:2px solid #e8cf87; }
        QLabel#sectionLabel { color:%6; font-size:10px; font-weight:600; letter-spacing:2px; }
        QLabel#brandLabel { font-size:14px; font-weight:600; }
        QLabel#toolName { font-size:12px; font-weight:600; padding:2px; }
        QLabel#strokePreview { border:2px inset %3; }
        QLabel#quietLabel { font-size:11px; padding:4px; }
        QScrollArea#canvasWell { background:%4; border:2px inset %3; padding:16px; }
        QSpinBox { background:%1; padding:5px 7px; border:1px solid %3; border-radius:2px; min-width:58px; }
        QSpinBox:focus { border:1px solid %6; }
        QCheckBox { padding:5px; }
        QStatusBar { background:%2; border-top:1px solid %3; padding:3px 8px; }
        QStatusBar::item { border:none; }
        QSlider::groove:horizontal { height:3px; background:%4; }
        QSlider::handle:horizontal { background:%6; width:9px; margin:-5px 0; border:1px solid %3; border-radius:1px; }
    )css").arg(base, panel, edge, shadow, selection, foreground);
    if (app->styleSheet() != css) app->setStyleSheet(css);
}
