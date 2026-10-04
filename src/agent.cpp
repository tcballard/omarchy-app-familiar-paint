#include "agent.h"
#include "window.h"
#include "commands.h"
#include <QApplication>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QJsonParseError>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLocalSocket>
#include <QRegularExpression>
#include <QTimer>
#include <QSet>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <stdexcept>

static void fail(QString text) { throw std::runtime_error(text.toStdString()); }
static void privateDirectory(const QString &path, bool create) {
    const auto bytes=QFile::encodeName(path);
    if(create && ::mkdir(bytes.constData(),0700)!=0 && errno!=EEXIST) fail("Cannot create private agent directory.");
    struct stat st{};
    if(::lstat(bytes.constData(),&st)!=0 || !S_ISDIR(st.st_mode) || st.st_uid!=getuid() || (st.st_mode & 0077))
        fail("Agent directory must be a real directory owned by this user with mode 0700.");
}
QString agentSocketPath(const QString &name) {
    if(!QRegularExpression("^[A-Za-z0-9_-]{1,32}$").match(name).hasMatch()) fail("Session name must contain 1–32 letters, numbers, _ or -.");
    QString base=qEnvironmentVariable("XDG_RUNTIME_DIR");
    if(!base.isEmpty()) {
        if(!QDir::isAbsolutePath(base)) fail("XDG_RUNTIME_DIR must be absolute.");
        privateDirectory(base,false); base += "/familiar-paint";
    } else base = "/tmp/familiar-paint-" + QString::number(getuid());
    privateDirectory(base,true);
    const auto path=base+"/"+name+".sock";
    if(QFile::encodeName(path).size()>100) fail("Agent socket path is too long.");
    return path;
}
QJsonObject sendAgentRequest(const QString &name, const QJsonObject &request) {
    QByteArray bytes=QJsonDocument(request).toJson(QJsonDocument::Compact);
    if(bytes.size()>Paint::MaxRequestBytes) fail("Request exceeds 1 MiB.");
    QLocalSocket socket; socket.connectToServer(agentSocketPath(name));
    if(!socket.waitForConnected(2000)) fail("Cannot connect to Paint session " + name + ": " + socket.errorString());
    socket.write(bytes+'\n');
    QElapsedTimer timer; timer.start();
    while(socket.bytesToWrite()) if(!socket.waitForBytesWritten(2000)) fail("Could not send request.");
    QByteArray response;
    while(!response.contains('\n')) {
        response += socket.readAll();
        if(response.size()>Paint::MaxResponseBytes) fail("Agent response exceeds limit.");
        if(response.contains('\n')) break;
        if(timer.elapsed()>30000) fail("Response timed out; inspect the canvas before retrying a mutation.");
        if(socket.state()==QLocalSocket::UnconnectedState) fail("Agent disconnected before responding; inspect before retrying.");
        socket.waitForReadyRead(100);
    }
    QJsonParseError error; const auto parsed=QJsonDocument::fromJson(response.left(response.indexOf('\n')), &error);
    if(error.error!=QJsonParseError::NoError || !parsed.isObject() || parsed.object().value("version")!=QJsonValue(1)) fail("Invalid agent response.");
    return parsed.object();
}
AgentServer::AgentServer(Window *window) : QObject(window), window_(window) {
    server_.setSocketOptions(QLocalServer::UserAccessOption); server_.setMaxPendingConnections(4);
    connect(&server_,&QLocalServer::newConnection,this,[this] {
        while(server_.hasPendingConnections()) {
            auto socket=server_.nextPendingConnection(); socket->setParent(this);
            if(connections_>=4) { socket->abort(); socket->deleteLater(); continue; }
            ++connections_; socket->setReadBufferSize(Paint::MaxRequestBytes+1);
            connect(socket,&QLocalSocket::disconnected,this,[this,socket] { --connections_; socket->deleteLater(); });
            auto timer=new QTimer(socket); timer->setSingleShot(true); timer->start(30000);
            connect(timer,&QTimer::timeout,socket,[socket] { socket->abort(); });
            auto input=std::make_shared<QByteArray>(); auto done=std::make_shared<bool>(false);
            auto receive=[this,socket,input,done] {
                if(*done) return; *input += socket->readAll();
                if(input->size()>Paint::MaxRequestBytes+1) { *done=true; socket->abort(); return; }
                const auto newline=input->indexOf('\n'); if(newline<0) return;
                *done=true; QJsonObject reply;
                try {
                    if(!input->mid(newline+1).trimmed().isEmpty()) fail("Only one request is allowed per connection.");
                    reply=handle(window_,Paint::parseRequest(input->left(newline)));
                } catch(const std::exception &e) { reply={{"version",1},{"ok",false},{"error",QString::fromUtf8(e.what())}}; }
                socket->write(QJsonDocument(reply).toJson(QJsonDocument::Compact)+'\n'); socket->disconnectFromServer();
            };
            connect(socket,&QLocalSocket::readyRead,this,receive);
            if(socket->bytesAvailable()) receive();
        }
    });
}
void AgentServer::start(const QString &name) {
    const auto path=agentSocketPath(name); lock_=std::make_unique<QLockFile>(path+".lock");
    if(!lock_->tryLock(0)) fail("That Paint session is already running.");
    QLocalServer::removeServer(path); // only after holding our private named-session lock
    if(!server_.listen(path)) fail(server_.errorString());
}
void AgentServer::stop() {
    server_.close();
    for(auto socket:findChildren<QLocalSocket *>()) socket->abort();
    lock_.reset();
}
QJsonObject AgentServer::handle(Window *window,const QJsonObject &request) {
    if(request.value("version")!=QJsonValue(1)) fail("Unsupported protocol version.");
    const auto op=request.value("op").toString();
    const QSet<QString> keys = op=="apply" ? QSet<QString>{"version","op","expected_revision","commands"}
        : (op=="undo" || op=="redo") ? QSet<QString>{"version","op","expected_revision"}
        : QSet<QString>{"version","op"};
    for(auto i=request.begin();i!=request.end();++i) if(!keys.contains(i.key())) fail("Unknown request field: "+i.key());
    auto &doc=window->document();
    auto describe=[window,&doc] {
        auto result=Paint::state(doc); result["busy"]=window->busy();
        result["color"]=window->canvas()->colour().name(QColor::HexArgb);
        result["zoom"]=window->canvas()->zoom();
        result["stroke_width"]=window->canvas()->strokeWidth();
        result["text_size"]=window->canvas()->textSize(); return result;
    };
    if(op=="inspect") return describe();
    if(window->busy() || window->canvas()->isDrawing() || QApplication::activeModalWidget()) fail("busy: wait until the current edit, dialog or file operation finishes.");
    if(op=="snapshot") {
        QByteArray data; QBuffer buffer(&data); buffer.open(QIODevice::WriteOnly);
        if(!doc.image().save(&buffer,"PNG")) fail("Could not encode canvas.");
        auto result=describe(); result["png_base64"]=QString::fromLatin1(data.toBase64()); return result;
    }
    if(op!="apply" && op!="undo" && op!="redo") fail("Unknown agent operation.");
    if(!request.value("expected_revision").isString() || request.value("expected_revision").toString()!=doc.revision())
        fail("conflict: inspect the canvas and use its current expected_revision.");
    if(op=="apply") {
        if(!request.value("commands").isArray()) fail("commands must be an array.");
        const auto image=Paint::applyCommands(doc.image(),request.value("commands").toArray()); doc.commit(image);
        window->canvas()->clearSelection(); window->refresh();
    } else window->history(op=="redo");
    return describe();
}
