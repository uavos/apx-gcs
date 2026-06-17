#include "OverlayTileServer.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QRegularExpression>
#include <QUrl>

OverlayTileServer::OverlayTileServer(QObject *parent)
    : QTcpServer(parent)
{
    connect(
        this,
        &QTcpServer::newConnection,
        this,
        &OverlayTileServer::acceptClient
    );
}

bool OverlayTileServer::startServer(quint16 port)
{
    if (isListening())
        return true;

    const bool ok =
        listen(QHostAddress::LocalHost, port);

    emit changed();

    return ok;
}

void OverlayTileServer::setSessionDir(const QString &path)
{
    _sessionDir = path;
    emit changed();
}

void OverlayTileServer::setZoomRange(int minZoom, int nativeZoom)
{
    _minZoom = minZoom;
    _nativeZoom = nativeZoom;
    emit changed();
}

QString OverlayTileServer::urlTemplate() const
{
    const quint16 p =
        serverPort() > 0 ? serverPort() : 9292;

    return QString(
        "http://127.0.0.1:%1/tile/%z/%x/%y.png"
    ).arg(p);
}

void OverlayTileServer::acceptClient()
{
    while (hasPendingConnections()) {
        QTcpSocket *socket =
            nextPendingConnection();

        _clients.insert(socket);

        connect(
            socket,
            &QTcpSocket::readyRead,
            this,
            &OverlayTileServer::readClient
        );

        connect(
            socket,
            &QTcpSocket::disconnected,
            this,
            [this, socket]() {
                _clients.remove(socket);
                socket->deleteLater();
            }
        );
    }
}

void OverlayTileServer::readClient()
{
    QTcpSocket *socket =
        qobject_cast<QTcpSocket *>(sender());

    if (!socket)
        return;

    const QByteArray request =
        socket->readAll();

    int z = 0;
    int x = 0;
    int y = 0;

    if (!parseTileRequest(request, &z, &x, &y)) {
        sendBytes(
            socket,
            transparentPng(),
            "image/png"
        );

        return;
    }

    sendTile(socket, z, x, y);
}

bool OverlayTileServer::parseTileRequest(
    const QByteArray &request,
    int *z,
    int *x,
    int *y) const
{
    const QList<QByteArray> lines =
        request.split('\n');

    if (lines.isEmpty())
        return false;

    const QByteArray firstLine =
        lines.first().trimmed();

    if (!firstLine.startsWith("GET "))
        return false;

    const int a =
        firstLine.indexOf(' ');

    const int b =
        firstLine.indexOf(' ', a + 1);

    if (a < 0 || b < 0)
        return false;

    QByteArray rawPath =
        firstLine.mid(a + 1, b - a - 1);

    const QString path =
        QUrl::fromPercentEncoding(rawPath);

    QStringList parts =
        path.split('/', Qt::SkipEmptyParts);

    QList<int> numbers;

    for (QString p : parts) {
        if (p.endsWith(".png"))
            p.chop(4);

        bool ok = false;
        const int value = p.toInt(&ok);

        if (ok)
            numbers.push_back(value);
    }

    if (numbers.size() < 3)
        return false;

    *z = numbers.at(numbers.size() - 3);
    *x = numbers.at(numbers.size() - 2);
    *y = numbers.at(numbers.size() - 1);

    return true;
}

void OverlayTileServer::sendTile(
    QTcpSocket *socket,
    int z,
    int x,
    int y)
{
    const QByteArray png =
        loadTileOrFallback(z, x, y);

    sendBytes(
        socket,
        png.isEmpty() ? transparentPng() : png,
        "image/png"
    );
}

QByteArray OverlayTileServer::loadTileOrFallback(
    int z,
    int x,
    int y) const
{
    if (_sessionDir.isEmpty())
        return {};

    const QString exact =
        tilePath(z, x, y);

    if (QFile::exists(exact)) {
        QFile f(exact);

        if (f.open(QIODevice::ReadOnly))
            return f.readAll();
    }

    // If the map requests a zoom level above nativeZoom,
    // crop the matching area from the native tile and scale it.
    // Example: z17/x/y is rendered from a quarter of the z16 tile.
    if (z > _nativeZoom) {
        const int dz =
            z - _nativeZoom;

        if (dz > 8)
            return {};

        const int scale =
            1 << dz;

        const int parentX =
            x / scale;

        const int parentY =
            y / scale;

        const QString parentPath =
            tilePath(
                _nativeZoom,
                parentX,
                parentY
            );

        if (!QFile::exists(parentPath))
            return {};

        QImage parent(parentPath);

        if (parent.isNull())
            return {};

        const int cropSize =
            parent.width() / scale;

        if (cropSize <= 0)
            return {};

        const int subX =
            x % scale;

        const int subY =
            y % scale;

        const QRect crop(
            subX * cropSize,
            subY * cropSize,
            cropSize,
            cropSize
        );

        QImage out =
            parent.copy(crop).scaled(
                256,
                256,
                Qt::IgnoreAspectRatio,
                Qt::FastTransformation
            );

        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        out.save(&buffer, "PNG");

        return bytes;
    }

    return {};
}

QString OverlayTileServer::tilePath(
    int z,
    int x,
    int y) const
{
    return
        _sessionDir +
        QString("/z%1/%2/%3.png")
            .arg(z)
            .arg(x)
            .arg(y);
}

QByteArray OverlayTileServer::transparentPng() const
{
    QImage img(
        256,
        256,
        QImage::Format_ARGB32_Premultiplied
    );

    img.fill(Qt::transparent);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");

    return bytes;
}

void OverlayTileServer::sendBytes(
    QTcpSocket *socket,
    const QByteArray &body,
    const QByteArray &contentType,
    int statusCode,
    const QByteArray &statusText)
{
    QByteArray header;

    header +=
        "HTTP/1.1 " +
        QByteArray::number(statusCode) +
        " " +
        statusText +
        "\r\n";

    header += "Content-Type: " + contentType + "\r\n";
    header += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    header += "Access-Control-Allow-Origin: *\r\n";
    header += "Cache-Control: no-cache, no-store, must-revalidate\r\n";
    header += "Pragma: no-cache\r\n";
    header += "Expires: 0\r\n";
    header += "Connection: close\r\n";
    header += "\r\n";

    socket->write(header);
    socket->write(body);
    socket->disconnectFromHost();
}
