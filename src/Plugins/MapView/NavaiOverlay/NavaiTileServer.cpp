#include "NavaiTileServer.h"
#include "NavaiOverlay.h"

#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QTcpSocket>
#include <cmath>

namespace {
constexpr int kTileSize = 256;
constexpr double kPi = 3.14159265358979323846;

QPointF project(double latitude, double longitude, int zoom)
{
    const double scale = kTileSize * std::pow(2.0, zoom);
    const double x = (longitude + 180.0) / 360.0 * scale;
    const double s = std::sin(latitude * kPi / 180.0);
    const double y = (0.5 - std::log((1.0 + s) / (1.0 - s)) / (4.0 * kPi)) * scale;
    return {x, y};
}
}

NavaiTileServer::NavaiTileServer(QObject *parent) : QTcpServer(parent)
{
    connect(this, &QTcpServer::newConnection, this, &NavaiTileServer::acceptClient);
}

bool NavaiTileServer::start(quint16 port)
{
    return isListening() || listen(QHostAddress::LocalHost, port);
}

void NavaiTileServer::setModel(const NavaiTileModel *model) { _model = model; }
void NavaiTileServer::invalidate() { _cache.clear(); }
QString NavaiTileServer::urlTemplate() const { return QStringLiteral("http://127.0.0.1:%1/navai-grid-v2/tile/%z/%x/%y.png").arg(serverPort()); }

void NavaiTileServer::acceptClient()
{
    while (hasPendingConnections()) {
        auto *socket = nextPendingConnection();
        _clients.insert(socket);
        connect(socket, &QTcpSocket::readyRead, this, &NavaiTileServer::readClient);
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] { _clients.remove(socket); socket->deleteLater(); });
    }
}

void NavaiTileServer::readClient()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) return;
    const QList<QByteArray> parts = socket->readAll().split(' ');
    QRegularExpression rx("^/navai-grid-v2/tile/(\\d+)/(\\d+)/(\\d+)\\.png$");
    const auto match = parts.size() > 1 ? rx.match(QString::fromLatin1(parts[1])) : QRegularExpressionMatch();
    if (!match.hasMatch()) { reply(socket, {}, 404); return; }
    reply(socket, render(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()));
}

void NavaiTileServer::reply(QTcpSocket *socket, const QByteArray &body, int status) const
{
    const QByteArray header = "HTTP/1.1 " + QByteArray::number(status) + (status == 200 ? " OK\r\n" : " Not Found\r\n")
        + "Content-Type: image/png\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
    socket->write(header); socket->write(body); socket->disconnectFromHost();
}

QByteArray NavaiTileServer::render(int z, int x, int y) const
{
    const QString key = QStringLiteral("%1/%2/%3").arg(z).arg(x).arg(y);
    if (_cache.contains(key)) return _cache.value(key);
    QImage image(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    if (_model) {
        const auto tiles = _model->tiles();
        QPainter painter(&image);
        auto polygon = [z, x, y](const QVariantMap &tile) {
            QPolygonF p;
            for (const auto &coordinate : tile.value("polygon").toList()) {
                const auto c = coordinate.value<QGeoCoordinate>();
                p << project(c.latitude(), c.longitude(), z) - QPointF(x * kTileSize, y * kTileSize);
            }
            return p;
        };
        painter.setBrush(Qt::NoBrush); painter.setPen(QPen(QColor(153, 153, 153, 38), 0.5));
        for (const auto &tile : tiles) painter.drawPolygon(polygon(tile));
    }
    QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly); image.save(&buffer, "PNG");
    if (_cache.size() > 256) _cache.clear();
    _cache.insert(key, bytes); return bytes;
}
