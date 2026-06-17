#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSet>
#include <QString>

class OverlayTileServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit OverlayTileServer(QObject *parent = nullptr);

    bool startServer(quint16 port = 9292);

    void setSessionDir(const QString &path);
    void setZoomRange(int minZoom, int nativeZoom);

    QString urlTemplate() const;
    bool ready() const { return isListening(); }

signals:
    void changed();

private slots:
    void acceptClient();
    void readClient();

private:
    void sendTile(QTcpSocket *socket, int z, int x, int y);
    void sendBytes(
        QTcpSocket *socket,
        const QByteArray &body,
        const QByteArray &contentType,
        int statusCode = 200,
        const QByteArray &statusText = "OK"
    );

    bool parseTileRequest(
        const QByteArray &request,
        int *z,
        int *x,
        int *y
    ) const;

    QByteArray transparentPng() const;
    QByteArray loadTileOrFallback(int z, int x, int y) const;

    QString tilePath(int z, int x, int y) const;

    QString _sessionDir;

    int _minZoom = 12;
    int _nativeZoom = 16;

    QSet<QTcpSocket *> _clients;
};
