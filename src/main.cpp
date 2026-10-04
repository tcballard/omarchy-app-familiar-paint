#include "window.h"
#include "theme.h"
#include "commands.h"
#include "agent.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextStream>
#include <QLabel>
#include <QToolButton>
#include <QStatusBar>
#include <QLockFile>
#include <stdexcept>

static void fail(QString s) { throw std::runtime_error(s.toStdString()); }
static void print(QJsonObject object) { QTextStream(stdout)<<QJsonDocument(object).toJson(QJsonDocument::Compact)<<'\n'; }
static QJsonObject loadRequest(const QString &path) {
    QFile file;
    if(path=="-") { if(!file.open(stdin,QIODevice::ReadOnly)) fail("Cannot read stdin."); }
    else { file.setFileName(path); if(!file.open(QIODevice::ReadOnly)) fail(file.errorString()); }
    const auto bytes=file.read(Paint::MaxRequestBytes+1);
    return Paint::parseRequest(bytes);
}
static void exportImage(const QImage &image, const QString &path, bool overwrite) {
    QLockFile lock(path+".familiar-paint.lock");
    if(!lock.tryLock(0)) fail("Output is locked by another Paint process.");
    if(QFileInfo::exists(path) && !overwrite) fail("Output already exists; use --force to replace it.");
    Paint::atomicSave(image,path);
}
int main(int argc,char **argv) {
    bool headless=false;
    for(int i=1;i<argc;++i) {
        const QString a=QString::fromLocal8Bit(argv[i]).section('=',0,0);
        if(a=="--render" || a=="--inspect" || a=="--send" || a=="--schema" || a=="--help" || a=="--version" || a=="-h" || a=="-v") headless=true;
    }
    if(headless) qputenv("QT_QPA_PLATFORM","offscreen");
    QApplication app(argc,argv);
    app.setApplicationName("Familiar Paint"); app.setApplicationVersion("0.0.1"); app.setOrganizationName("Familiar");
    app.setDesktopFileName("io.github.tcballard.FamiliarPaint"); app.setWindowIcon(QIcon::fromTheme("io.github.tcballard.FamiliarPaint")); app.setStyle("Fusion");
    QCommandLineParser parser;
    parser.setApplicationDescription("Native Paint with JSON drawing batches and opt-in local agent control. See --schema.");
    parser.addHelpOption(); parser.addVersionOption(); parser.addPositionalArgument("image","Image to open in the GUI.","[image]");
    parser.addOptions({
        {"render","Render a version-1 JSON command batch without opening a window. Use - for stdin.","file"},
        {"input","Initial image for --render (otherwise white 1000x650).","file"},
        {"output","Output PNG/JPEG/BMP for --render.","file"},
        {"inspect","Inspect an image and return JSON.","file"},
        {"schema","Print the versioned command/protocol reference as JSON."},
        {"listen","Open GUI with same-user local agent control enabled.","session"},
        {"send","Send a command to a named live GUI session.","session"},
        {"request","Raw version-1 live request JSON (or - for stdin).","file"},
        {"apply","Apply a JSON drawing batch to a live session in one undo step.","file"},
        {"state","Inspect the named live session."},
        {"undo","Undo one change in the named live session."},
        {"redo","Redo one change in the named live session."},
        {"export","Export a full-resolution snapshot of the named live session.","file"},
        {"expect","Expected canvas revision for --apply/--undo/--redo; otherwise inspect immediately before sending.","revision"},
        {"force","Allow replacing an existing CLI export/output file."}
    });
    parser.process(app);
    try {
        int modes=0; for(auto name:{"render","inspect","schema","send","listen"}) modes+=parser.isSet(name);
        if(modes>1) fail("Choose one of --render, --inspect, --schema, --send or --listen.");
        if(parser.positionalArguments().size()>1 || (headless && !parser.positionalArguments().isEmpty())) fail("Unexpected positional arguments.");
        for(auto name:{"request","apply","state","undo","redo","export","expect"}) if(parser.isSet(name) && !parser.isSet("send")) fail(QString("--")+name+" requires --send.");
        for(auto name:{"input","output"}) if(parser.isSet(name) && !parser.isSet("render")) fail(QString("--")+name+" requires --render.");
        if(parser.isSet("force") && !parser.isSet("render") && !parser.isSet("export")) fail("--force requires --render or --export.");
        if(parser.isSet("schema")) { print(Paint::protocolSchema()); return 0; }
        if(parser.isSet("inspect")) {
            const auto path=QFileInfo(parser.value("inspect")).absoluteFilePath();
            Paint::Document doc(Paint::readImage(path)); doc.markSaved(path,Paint::fingerprint(path)); print(Paint::state(doc)); return 0;
        }
        if(parser.isSet("render")) {
            if(!parser.isSet("output")) fail("--render requires --output.");
            const auto batch=loadRequest(parser.value("render"));
            for(auto i=batch.begin();i!=batch.end();++i) if(i.key()!="version" && i.key()!="commands") fail("Render batch accepts only version and commands.");
            if(!batch.value("commands").isArray()) fail("commands must be an array.");
            auto initial=parser.isSet("input") ? Paint::readImage(parser.value("input")) : Paint::blank();
            auto image=Paint::applyCommands(initial,batch.value("commands").toArray());
            auto path=QFileInfo(parser.value("output")).absoluteFilePath(); exportImage(image,path,parser.isSet("force"));
            Paint::Document doc(image); doc.markSaved(path,Paint::fingerprint(path)); print(Paint::state(doc)); return 0;
        }
        if(parser.isSet("send")) {
            int commands=0; for(auto name:{"request","apply","state","undo","redo","export"}) commands+=parser.isSet(name);
            if(commands!=1) fail("--send requires exactly one of --request, --apply, --state, --undo, --redo or --export.");
            if(parser.isSet("expect") && !parser.isSet("apply") && !parser.isSet("undo") && !parser.isSet("redo")) fail("--expect requires --apply, --undo or --redo.");
            const auto session=parser.value("send"); QJsonObject request{{"version",1}};
            if(parser.isSet("request")) request=loadRequest(parser.value("request"));
            else if(parser.isSet("state")) request["op"]="inspect";
            else if(parser.isSet("export")) request["op"]="snapshot";
            else {
                const QString op=parser.isSet("apply") ? "apply" : parser.isSet("undo") ? "undo" : "redo";
                request["op"]=op; QString revision=parser.value("expect");
                if(revision.isEmpty()) { auto state=sendAgentRequest(session,{{"version",1},{"op","inspect"}}); if(!state.value("ok").toBool()) { print(state); return 1; } revision=state.value("revision").toString(); }
                request["expected_revision"]=revision;
                if(op=="apply") {
                    auto batch=loadRequest(parser.value("apply"));
                    for(auto i=batch.begin();i!=batch.end();++i) if(i.key()!="version" && i.key()!="commands") fail("Batch accepts only version and commands.");
                    request["commands"]=batch.value("commands");
                }
            }
            auto result=sendAgentRequest(session,request);
            if(result.value("ok").toBool() && parser.isSet("export")) {
                const auto image=QImage::fromData(QByteArray::fromBase64(result.value("png_base64").toString().toLatin1()),"PNG");
                if(image.isNull()) fail("Invalid snapshot returned by session.");
                const auto path=QFileInfo(parser.value("export")).absoluteFilePath(); exportImage(image,path,parser.isSet("force"));
                result.remove("png_base64"); result["export_path"]=path;
            }
            print(result); return result.value("ok").toBool() ? 0 : 1;
        }
        applyTheme(&app); Window window; AgentServer agent(&window);
        if(parser.isSet("listen")) {
            const auto session=parser.value("listen"); agent.start(session);
            auto badge=new QLabel("Agent: "+session,&window); window.statusBar()->addPermanentWidget(badge);
            auto stop=new QToolButton(&window); stop->setText("Stop agent"); stop->setAccessibleName("Disable agent control");
            QObject::connect(stop,&QToolButton::clicked,&window,[&agent,badge,stop] { agent.stop(); badge->setText("Agent off"); stop->setEnabled(false); });
            window.statusBar()->addPermanentWidget(stop);
            print({{"version",1},{"ok",true},{"session",session},{"socket",agentSocketPath(session)}});
        }
        window.show();
        if(!parser.positionalArguments().isEmpty()) QTimer::singleShot(0,&window,[&window,&parser] { window.open(parser.positionalArguments().first()); });
        return app.exec();
    } catch(const std::exception &e) { print({{"version",1},{"ok",false},{"error",QString::fromUtf8(e.what())}}); return 1; }
}
