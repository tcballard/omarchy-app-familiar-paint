#pragma once
#include <QLocalServer>
#include <QLockFile>
#include <QJsonObject>
#include <memory>
class Window;
QString agentSocketPath(const QString &name);
QJsonObject sendAgentRequest(const QString &name, const QJsonObject &request);
class AgentServer : public QObject {
    Q_OBJECT
public:
    explicit AgentServer(Window *window);
    void start(const QString &name);
    void stop();
    static QJsonObject handle(Window *window, const QJsonObject &request);
private:
    Window *window_;
    QLocalServer server_;
    std::unique_ptr<QLockFile> lock_;
    int connections_ = 0;
};
