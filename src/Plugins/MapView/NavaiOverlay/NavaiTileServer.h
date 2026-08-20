#pragma once

#include <QTcpServer>
#include <QSet>

class NavaiTileModel;
class QTcpSocket;

class NavaiTileServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit NavaiTileServer(QObject *parent = nullptr);
    bool start(quint16 port = 9293);
    void setModel(const NavaiTileModel *model);
    void invalidate();
    QString urlTemplate() const;

private slots:
    void acceptClient();
    void readClient();

private:
    QByteArray render(int z, int x, int y) const;
    void reply(QTcpSocket *socket, const QByteArray &body, int status = 200) const;
    const NavaiTileModel *_model = nullptr;
    mutable QHash<QString, QByteArray> _cache;
    QSet<QTcpSocket *> _clients;
};
