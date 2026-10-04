#pragma once
#include "document.h"
#include <QJsonObject>
#include <QJsonArray>
namespace Paint {
constexpr int MaxRequestBytes = 1024 * 1024;
constexpr int MaxResponseBytes = 96 * 1024 * 1024;
QJsonObject parseRequest(const QByteArray &bytes);
QImage applyCommands(const QImage &initial, const QJsonArray &commands);
QJsonObject state(const Document &doc);
QJsonObject protocolSchema();
}
