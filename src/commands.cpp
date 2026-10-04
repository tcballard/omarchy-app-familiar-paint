#include "commands.h"
#include "drawing.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTransform>
#include <QGuiApplication>
#include <QSet>
#include <stdexcept>
#include <cmath>
namespace Paint {
static void fail(QString s) { throw std::runtime_error(s.toStdString()); }
static int number(const QJsonObject &o, const QString &key, int low, int high, int fallback = 0, bool optional = false) {
    if (!o.contains(key) && optional) return fallback;
    auto v = o.value(key); double n = v.toDouble();
    if (!v.isDouble() || !std::isfinite(n) || n != std::floor(n) || n < low || n > high) fail(key + " must be an integer in [" + QString::number(low) + ", " + QString::number(high) + "]");
    return int(n);
}
static QPoint point(const QJsonValue &v) {
    if (!v.isArray() || v.toArray().size() != 2) fail("Coordinates must be [x, y].");
    auto a = v.toArray(); QJsonObject o{{"x", a[0]}, {"y", a[1]}};
    return {number(o,"x",-32768,32768), number(o,"y",-32768,32768)};
}
static QColor colour(const QJsonObject &o) {
    if (!o.value("color").isString()) fail("color must be a colour string, such as #efc95b.");
    QColor c(o.value("color").toString()); if (!c.isValid()) fail("Invalid color."); return c;
}
static void fields(const QJsonObject &o, QSet<QString> allowed) {
    allowed.insert("op"); for (auto i=o.begin(); i!=o.end(); ++i) if (!allowed.contains(i.key())) fail("Unknown command field: " + i.key());
}
QJsonObject parseRequest(const QByteArray &bytes) {
    if (bytes.size() > MaxRequestBytes) fail("Request exceeds 1 MiB.");
    QJsonParseError error; auto doc = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) fail("Expected a JSON object: " + error.errorString());
    const auto o = doc.object();
    if (o.value("version") != QJsonValue(1)) fail("Unsupported protocol version; expected version: 1.");
    return o;
}
QImage applyCommands(const QImage &initial, const QJsonArray &commands) {
    if (commands.isEmpty() || commands.size() > 128) fail("A batch must contain 1–128 commands.");
    QImage image = initial; qint64 work = 0; int pointsTotal = 0; int commandIndex = 0;
    for (const auto &value : commands) {
        if (!value.isObject()) fail("Every command must be an object.");
        const auto o = value.toObject(); const auto op = o.value("op").toString();
        if (op == "new" && commandIndex != 0) fail("new must be the first command.");
        ++commandIndex;
        work += qint64(image.width()) * image.height();
        if (work > 256'000'000) fail("Batch exceeds the pixel-work budget; split it into smaller batches.");
        if (op == "new" || op == "resize") {
            fields(o, op == "new" ? QSet<QString>{"width","height","color"} : QSet<QString>{"width","height"});
            int w=number(o,"width",1,16384), h=number(o,"height",1,16384); validateSize(w,h);
            if (op == "new") { auto c = colour(o); image = blank(w,h); image.fill(c); }
            else image = image.scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        } else if (op == "crop") {
            fields(o,{"x","y","width","height"});
            QRect rect(number(o,"x",0,image.width()-1),number(o,"y",0,image.height()-1),number(o,"width",1,16384),number(o,"height",1,16384));
            if (!image.rect().contains(rect)) fail("Crop must be inside the canvas."); image = image.copy(rect);
        } else if (op == "rotate") {
            fields(o,{"degrees"}); const int degrees=number(o,"degrees",-270,270);
            if (degrees % 90) fail("Rotation must be a multiple of 90 degrees.");
            image = image.transformed(QTransform().rotate(degrees));
        } else if (op == "flip") {
            fields(o,{"axis"}); const auto axis=o.value("axis").toString();
            if (axis!="horizontal" && axis!="vertical") fail("axis must be horizontal or vertical.");
            image=image.mirrored(axis=="horizontal",axis=="vertical");
        } else if (op == "fill") {
            fields(o,{"at","color"}); const auto at=point(o.value("at"));
            if (!image.rect().contains(at)) fail("Fill point must be inside the canvas.");
            image=floodFill(image,at,colour(o));
        } else if (op == "text") {
            fields(o,{"at","text","color","size","font"}); const auto at=point(o.value("at"));
            if (!o.value("text").isString() || o.value("text").toString().size()>10000) fail("text must be a string of at most 10000 characters.");
            QFont font=QGuiApplication::font();
            if(o.contains("font")) { if(!o.value("font").isString() || o.value("font").toString().size()>128) fail("Invalid font family."); font.setFamily(o.value("font").toString()); }
            drawText(image,at,o.value("text").toString(),colour(o),font,number(o,"size",8,200,20,true));
        } else if (op == "line" || op == "arrow" || op == "rect" || op == "ellipse" || op == "stroke") {
            fields(o,op=="stroke" ? QSet<QString>{"points","color","width"} : QSet<QString>{"from","to","color","width","filled"});
            const auto c=colour(o); const int width=number(o,"width",1,80,5,true);
            if(o.contains("filled") && !o.value("filled").isBool()) fail("filled must be true or false.");
            if(op=="stroke") {
                if(!o.value("points").isArray()) fail("points must be an array."); const auto points=o.value("points").toArray();
                pointsTotal += points.size(); if(points.isEmpty() || pointsTotal>8192) fail("Batch must have at most 8192 stroke points.");
                auto prev=point(points[0]); drawPrimitive(image,"line",prev,prev,c,width);
                for(int i=1;i<points.size();++i) { auto next=point(points[i]); drawPrimitive(image,"line",prev,next,c,width); prev=next; }
            } else drawPrimitive(image,op,point(o.value("from")),point(o.value("to")),c,width,o.value("filled").toBool());
        } else fail("Unknown drawing operation: " + op);
    }
    return image;
}
QJsonObject state(const Document &doc) {
    return {{"version",1},{"ok",true},{"width",doc.image().width()},{"height",doc.image().height()},
            {"revision",doc.revision()},{"dirty",doc.dirty()},{"path",doc.path()},
            {"can_undo",doc.canUndo()},{"can_redo",doc.canRedo()},{"format","ARGB32-premultiplied"}};
}
QJsonObject protocolSchema() {
    return {{"version",1},{"coordinate_system","integer image pixels; origin top-left; from/to endpoints inclusive"},
        {"operations",QJsonArray{"inspect","apply","undo","redo","snapshot"}},
        {"mutation_rule","apply/undo/redo require expected_revision from inspect; apply is one atomic undo step"},
        {"commands",QJsonObject{
            {"new","width, height, color (must be first; live creation is undoable and keeps current file association)"},
            {"line/arrow/rect/ellipse","from:[x,y], to:[x,y], color, width?:1..80, filled?:bool"},
            {"stroke","points:[[x,y],...], color, width?:1..80"},
            {"fill","at:[x,y], color"},{"text","at:[x,y], text, color, size?:8..200, font?:family"},
            {"crop","x, y, width, height"},{"resize","width, height"},{"rotate","degrees: multiple of 90"},{"flip","axis:horizontal|vertical"}}},
        {"limits",QJsonObject{{"request_bytes",MaxRequestBytes},{"commands",128},{"pixels",double(MaxPixels)},{"pixel_work",256000000},{"stroke_points",8192}}},
        {"example",QJsonObject{{"version",1},{"commands",QJsonArray{
            QJsonObject{{"op","new"},{"width",640},{"height",480},{"color","white"}},
            QJsonObject{{"op","ellipse"},{"from",QJsonArray{400,40}},{"to",QJsonArray{500,140}},{"color","#efc95b"},{"filled",true}}}}}}};
}
}
