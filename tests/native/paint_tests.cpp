#include "window.h"
#include "commands.h"
#include "agent.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include "theme.h"
#include <QtTest>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QPainter>

static void edit(Paint::Document &doc, QColor colour = Qt::red) {
    auto image = doc.image(); image.setPixelColor(4, 4, colour); doc.commit(image);
}
class PaintTests : public QObject {
    Q_OBJECT
private slots:
    void historyBranch() {
        Paint::Document doc(Paint::blank(32, 32));
        edit(doc); doc.markSaved("a.png", "hash");
        doc.undo(); QVERIFY(doc.dirty());
        doc.redo(); QVERIFY(!doc.dirty());
        doc.undo(); edit(doc, Qt::blue); QVERIFY(doc.dirty()); QVERIFY(!doc.canRedo());
    }
    void cropResizeUndo() {
        Paint::Document doc(Paint::blank(32, 32)); edit(doc);
        doc.crop(QRect(2, 2, 10, 10)); QCOMPARE(doc.image().pixelColor(2, 2), QColor(Qt::red));
        doc.resize(20, 20); doc.undo(); QCOMPARE(doc.image().size(), QSize(10, 10));
        doc.undo(); QCOMPARE(doc.image().size(), QSize(32, 32));
    }
    void sizeAndCorruptRejection() {
        QVERIFY_EXCEPTION_THROWN(Paint::blank(16384, 16384), std::runtime_error);
        QTemporaryDir dir; QFile bad(dir.filePath("bad.png")); QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("invalid image"); bad.close();
        QVERIFY_EXCEPTION_THROWN(Paint::readImage(bad.fileName()), std::runtime_error);
    }
    void saveReopen() {
        QTemporaryDir dir; auto image = Paint::blank(32, 32);
        image.setPixelColor(0, 0, QColor(20, 40, 80, 128));
        auto path = dir.filePath("image with spaces.png");
        Paint::atomicSave(image, path); QCOMPARE(Paint::readImage(path), image);
        auto digest = Paint::fingerprint(path);
        QVERIFY_EXCEPTION_THROWN(Paint::atomicSave(image, dir.filePath("missing/a.png")), std::runtime_error);
        QCOMPARE(Paint::fingerprint(path), digest);
    }
    void invalidFormatPreservesFile() {
        QTemporaryDir dir; QFile file(dir.filePath("original.xyz")); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("keep this"); file.close();
        QVERIFY_EXCEPTION_THROWN(Paint::atomicSave(Paint::blank(), file.fileName()), std::runtime_error);
        QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), QByteArray("keep this"));
    }
    void jpegFlattensWhite() {
        QTemporaryDir dir; auto image = Paint::blank(32, 32); image.fill(Qt::transparent);
        auto path = dir.filePath("flat.jpg"); Paint::atomicSave(image, path);
        QCOMPARE(Paint::readImage(path).pixelColor(10, 10), QColor(Qt::white));
    }
    void historyBudget() {
        Paint::Document doc(Paint::blank(2500, 2000));
        for (int n = 0; n < 15; ++n) edit(doc, QColor(n, 20, 30));
        QVERIFY(doc.historyBytes() <= Paint::HistoryBudget);
    }
    void floodContained() {
        auto image = Paint::blank(30, 30);
        { QPainter p(&image); p.setPen(Qt::black); p.drawRect(5, 5, 20, 20); }
        auto filled = Canvas::floodFill(image, {10, 10}, Qt::red);
        QCOMPARE(filled.pixelColor(10, 10), QColor(Qt::red));
        QCOMPARE(filled.pixelColor(0, 0), QColor(Qt::white));
        QCOMPARE(filled.pixelColor(5, 5), QColor(Qt::black));
        QCOMPARE(Canvas::floodFill(filled, {10, 10}, Qt::red), filled);
    }
    void brushUndoCancel() {
        Window w; w.replace(Paint::blank(100, 100)); w.document().discardChanges(); w.show();
        QTest::qWait(10); auto c = w.canvas();
        QTest::mousePress(c, Qt::LeftButton, {}, {10, 10});
        QTest::mouseMove(c, {60, 60}); QTest::mouseRelease(c, Qt::LeftButton, {}, {60, 60});
        QVERIFY(w.document().dirty()); QVERIFY(w.document().image().pixelColor(30, 30) != QColor(Qt::white));
        w.history(false); QVERIFY(!w.document().dirty());
        QTest::mousePress(c, Qt::LeftButton, {}, {10, 10}); QTest::mouseMove(c, {60, 60});
        QTest::keyClick(c, Qt::Key_Escape); QTest::mouseRelease(c, Qt::LeftButton, {}, {60, 60});
        QVERIFY(!w.document().dirty());
    }
    void shapes_data() {
        QTest::addColumn<int>("tool");
        QTest::newRow("line") << int(Canvas::Tool::Line);
        QTest::newRow("arrow") << int(Canvas::Tool::Arrow);
        QTest::newRow("rectangle") << int(Canvas::Tool::Rectangle);
        QTest::newRow("ellipse") << int(Canvas::Tool::Ellipse);
    }
    void shapes() {
        QFETCH(int, tool); Window w; w.replace(Paint::blank(100, 100)); w.document().discardChanges(); w.show();
        QTest::qWait(10); auto c = w.canvas(); c->setTool(Canvas::Tool(tool));
        QTest::mousePress(c, Qt::LeftButton, {}, {10, 10}); QTest::mouseRelease(c, Qt::LeftButton, {}, {80, 80});
        QVERIFY(w.document().dirty()); w.history(false); QCOMPARE(w.document().image(), Paint::blank(100, 100));
    }
    void focusLossCancelsDrag() {
        Window w; w.replace(Paint::blank(100, 100)); w.document().discardChanges(); w.show(); QTest::qWait(10);
        auto c = w.canvas(); QTest::mousePress(c, Qt::LeftButton, {}, {10, 10});
        QTest::mouseMove(c, {60, 60});
        QEvent deactivate(QEvent::WindowDeactivate); QApplication::sendEvent(&w, &deactivate);
        QTest::mouseRelease(c, Qt::LeftButton, {}, {60, 60}); QVERIFY(!w.document().dirty());
    }
    void selectCropClipboard() {
        Window w; w.replace(Paint::blank(100, 100)); w.show(); QTest::qWait(10);
        auto c = w.canvas(); c->setTool(Canvas::Tool::Select);
        QTest::mousePress(c, Qt::LeftButton, {}, {10, 10}); QTest::mouseRelease(c, Qt::LeftButton, {}, {29, 29});
        w.copy(); QCOMPARE(QApplication::clipboard()->image().size(), QSize(20, 20));
        w.crop(); QCOMPARE(w.document().image().size(), QSize(20, 20));
    }
    void textUndo() {
        Window w; w.replace(Paint::blank(200, 100)); w.document().discardChanges();
        w.canvas()->addText({10, 10}, "Hello, Familiar"); QVERIFY(w.document().dirty());
        w.history(false); QCOMPARE(w.document().image(), Paint::blank(200, 100));
    }
    void asyncSaveConflictAndClose() {
        QTemporaryDir dir; auto path = dir.filePath("output.png"); Window w;
        w.replace(Paint::blank(100, 100)); Paint::atomicSave(w.document().image(), path);
        w.document().markSaved(path, Paint::fingerprint(path)); edit(w.document());
        QString error; w.reportError = [&error](QString e) { error = e; };
        w.show(); w.save(); QVERIFY(w.busy()); w.close(); QVERIFY(w.isVisible());
        QTRY_VERIFY_WITH_TIMEOUT(!w.busy(), 3000);
        QVERIFY(error.isEmpty()); QVERIFY(!w.document().dirty());
        QCOMPARE(Paint::readImage(path).pixelColor(4, 4), QColor(Qt::red));
        QFile external(path); QVERIFY(external.open(QIODevice::WriteOnly)); external.write("external edit"); external.close();
        edit(w.document(), Qt::blue); w.save(); QTRY_VERIFY_WITH_TIMEOUT(!w.busy(), 3000);
        QVERIFY(!error.isEmpty()); QVERIFY(w.document().dirty());
        QVERIFY(external.open(QIODevice::ReadOnly)); QCOMPARE(external.readAll(), QByteArray("external edit"));
    }
    void cancelClose() {
        Window w; edit(w.document()); w.show();
        QTimer::singleShot(0, [] {
            if (auto box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) box->done(QMessageBox::Cancel);
        });
        w.close(); QVERIFY(w.isVisible()); QVERIFY(w.document().dirty());
    }
    void batchAtomicityAndRevision() {
        Window w; auto before=w.document().image(); auto revision=w.document().revision();
        QJsonArray bad{QJsonObject{{"op","line"},{"from",QJsonArray{1,1}},{"to",QJsonArray{30,30}},{"color","red"}},QJsonObject{{"op","typo"}}};
        QVERIFY_EXCEPTION_THROWN(AgentServer::handle(&w,{{"version",1},{"op","apply"},{"expected_revision",revision},{"commands",bad}}),std::runtime_error);
        QCOMPARE(w.document().image(),before); QCOMPARE(w.document().revision(),revision);
        bad.removeLast();
        auto reply=AgentServer::handle(&w,{{"version",1},{"op","apply"},{"expected_revision",revision},{"commands",bad}});
        QVERIFY(reply.value("ok").toBool()); QVERIFY(w.document().revision()!=revision);
        QVERIFY_EXCEPTION_THROWN(AgentServer::handle(&w,{{"version",1},{"op","undo"},{"expected_revision",revision}}),std::runtime_error);
        AgentServer::handle(&w,{{"version",1},{"op","undo"},{"expected_revision",w.document().revision()}});
        QCOMPARE(w.document().image(),before); QVERIFY(!w.document().canUndo());
        AgentServer::handle(&w,{{"version",1},{"op","redo"},{"expected_revision",w.document().revision()}});
        const auto snapshot=AgentServer::handle(&w,{{"version",1},{"op","snapshot"}});
        const auto exported=QImage::fromData(QByteArray::fromBase64(snapshot.value("png_base64").toString().toLatin1()),"PNG");
        QCOMPARE(exported.convertToFormat(QImage::Format_ARGB32_Premultiplied),w.document().image());
        QVERIFY(w.document().dirty());
        QCOMPARE(AgentServer::handle(&w,{{"version",1},{"op","inspect"}}).value("revision").toString(),w.document().revision());
    }
    void commandsRejectBadInputs() {
        auto image=Paint::blank(100,100);
        for(const QByteArray json : {QByteArray(R"({"op":"resize","width":1.5,"height":20})"), QByteArray(R"({"op":"crop","x":99,"y":0,"width":50,"height":50})"), QByteArray(R"({"op":"line","from":[0,0],"to":[10,10],"color":"bad-colour"})"), QByteArray(R"({"op":"new","width":16384,"height":16384,"color":"white"})")}) {
            QVERIFY_EXCEPTION_THROWN(Paint::applyCommands(image,{QJsonDocument::fromJson(json).object()}),std::runtime_error);
        }
        QVERIFY_EXCEPTION_THROWN(Paint::parseRequest(R"({"version":2})"),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(Paint::parseRequest(QByteArray(Paint::MaxRequestBytes+1,'x')),std::runtime_error);
        const QJsonObject fresh{{"op","new"},{"width",10},{"height",10},{"color","white"}};
        QVERIFY_EXCEPTION_THROWN(Paint::applyCommands(image,{fresh,fresh}),std::runtime_error);
    }
    void sharedDrawingEngine() {
        Window w; w.replace(Paint::blank(100,100)); w.show(); QTest::qWait(10);
        auto c=w.canvas(); c->setTool(Canvas::Tool::Arrow); c->setColour(Qt::red); c->setStroke(5);
        QTest::mousePress(c,Qt::LeftButton,{},QPoint(10,10)); QTest::mouseRelease(c,Qt::LeftButton,{},QPoint(80,70));
        const QJsonArray commands{QJsonObject{{"op","arrow"},{"from",QJsonArray{10,10}},{"to",QJsonArray{80,70}},{"color","red"},{"width",5}}};
        QCOMPARE(Paint::applyCommands(Paint::blank(100,100),commands),w.document().image());
    }
    void liveRejectsActiveDrag() {
        Window w; w.show(); QTest::qWait(10); const auto revision=w.document().revision();
        QTest::mousePress(w.canvas(),Qt::LeftButton,{},QPoint(10,10));
        QVERIFY_EXCEPTION_THROWN(AgentServer::handle(&w,{{"version",1},{"op","undo"},{"expected_revision",revision}}),std::runtime_error);
        QTest::keyClick(w.canvas(),Qt::Key_Escape);
    }
    void cliAndLiveRoundTrip() {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const auto program=QCoreApplication::applicationDirPath()+"/familiar-paint";
        auto env=QProcessEnvironment::systemEnvironment(); env.insert("QT_QPA_PLATFORM","offscreen"); env.insert("XDG_RUNTIME_DIR",dir.path());
        auto run=[&](QStringList args) { QProcess p; p.setProcessEnvironment(env); p.start(program,args); if(!p.waitForFinished(15000)) { p.kill(); p.waitForFinished(); return QPair<int,QByteArray>{-1,"timeout"}; } return QPair<int,QByteArray>{p.exitCode(),p.readAllStandardOutput()}; };
        const QByteArray batch=R"({"version":1,"commands":[{"op":"new","width":256,"height":200,"color":"white"},{"op":"rect","from":[20,20],"to":[100,100],"color":"red","filled":true},{"op":"text","at":[20,130],"text":"Agent made this","color":"black","size":18}]})";
        QFile f(dir.filePath("batch.json")); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(batch); f.close();
        auto rendered=dir.filePath("render.png");
        auto r=run({"--render",f.fileName(),"--output",rendered}); QCOMPARE(r.first,0);
        r=run({"--render",f.fileName(),"--output",rendered}); QCOMPARE(r.first,1); // no accidental overwrite
        QCOMPARE(Paint::readImage(rendered).size(),QSize(256,200));
        r=run({"--inspect",rendered}); QCOMPARE(r.first,0);
        QCOMPARE(QJsonDocument::fromJson(r.second).object().value("width").toInt(),256);
        if(qEnvironmentVariableIsSet("PAINT_TEST_NO_LOCAL_SOCKETS"))
            QSKIP("CLI render/inspect checked; local socket transport disabled by this test environment.");
        QProcess live; live.setProcessEnvironment(env); live.start(program,{"--listen","test"}); QVERIFY(live.waitForStarted()); QVERIFY(live.waitForReadyRead(3000));
        const auto readyBytes=live.readAllStandardOutput();
        auto ready=QJsonDocument::fromJson(readyBytes).object(); QVERIFY2(ready.value("ok").toBool(),qPrintable(QString::fromUtf8(readyBytes+live.readAllStandardError())));
        r=run({"--send","test","--state"}); QCOMPARE(r.first,0); const auto initial=QJsonDocument::fromJson(r.second).object().value("revision").toString();
        r=run({"--send","test","--apply",f.fileName(),"--expect",initial}); QCOMPARE(r.first,0);
        r=run({"--send","test","--export",dir.filePath("live.png")}); QCOMPARE(r.first,0);
        QCOMPARE(Paint::readImage(rendered),Paint::readImage(dir.filePath("live.png")));
        r=run({"--send","test","--undo","--expect",initial}); QCOMPARE(r.first,1);
        r=run({"--send","test","--undo"}); QCOMPARE(r.first,0);
        auto restored=QJsonDocument::fromJson(r.second).object(); QCOMPARE(restored.value("width").toInt(),1000); QVERIFY(!restored.value("can_undo").toBool());
        r=run({"--send","test","--redo"}); QCOMPARE(r.first,0);
        r=run({"--send","test","--export",dir.filePath("redo.png")}); QCOMPARE(r.first,0);
        QCOMPARE(Paint::readImage(rendered),Paint::readImage(dir.filePath("redo.png")));
        r=run({"--listen","test"}); QCOMPARE(r.first,1); // duplicate endpoint cannot displace the running app
        live.terminate(); QVERIFY(live.waitForFinished(3000));
    }
    void themeReplacement() {
        QTemporaryDir dir; const auto old = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", dir.path().toUtf8());
        QDir().mkpath(dir.filePath("omarchy/current/theme"));
        QFile file(themePath()); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("background = \"#123456\"\nforeground = \"#ffffff\"\n"); file.close();
        applyTheme(qApp); QCOMPARE(qApp->palette().color(QPalette::Window), QColor("#123456"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("garbage"); file.close(); applyTheme(qApp);
        QCOMPARE(qApp->palette().color(QPalette::Window), QColor("#20242b"));
        qputenv("XDG_CONFIG_HOME", old); applyTheme(qApp);
    }
};
QTEST_MAIN(PaintTests)
#include "paint_tests.moc"
