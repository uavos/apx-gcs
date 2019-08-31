/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef DatalinkConnection_H
#define DatalinkConnection_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
//=============================================================================
class DatalinkConnection : public Fact
{
    Q_OBJECT
    Q_PROPERTY(quint16 rxNetwork READ rxNetwork WRITE setRxNetwork NOTIFY rxNetworkChanged)
    Q_PROPERTY(quint16 txNetwork READ txNetwork WRITE setTxNetwork NOTIFY txNetworkChanged)

    Q_PROPERTY(
        bool blockControls READ blockControls WRITE setBlockControls NOTIFY blockControlsChanged)
    Q_PROPERTY(bool blockService READ blockService WRITE setBlockService NOTIFY blockServiceChanged)

public:
    explicit DatalinkConnection(Fact *parent,
                                const QString &name,
                                const QString &title,
                                const QString &descr,
                                quint16 rxNetwork,
                                quint16 txNetwork);

protected:
    bool _allowed;

    virtual QByteArray read();
    virtual void write(const QByteArray &packet);

private:
    bool isControlPacket(const QByteArray &packet) const;

protected slots:
    void updateDescr();

    void readDataAvailable();
    void opened();
    void closed();

public slots:
    virtual void open();
    virtual void close();

    //export data
signals:
    void packetReceived(QByteArray packet, quint16 network);
public slots:
    void sendPacket(QByteArray packet, quint16 network);

    //-----------------------------------------
    //PROPERTIES
public:
    quint16 rxNetwork() const;
    void setRxNetwork(const quint16 &v);
    quint16 txNetwork() const;
    void setTxNetwork(const quint16 &v);

    bool blockControls() const;
    void setBlockControls(const bool &v);
    bool blockService() const;
    void setBlockService(const bool &v);

private:
    quint16 m_rxNetwork;
    quint16 m_txNetwork;

    bool m_blockControls;
    bool m_blockService;
signals:
    void rxNetworkChanged();
    void txNetworkChanged();

    void blockControlsChanged();
    void blockServiceChanged();
};
//=============================================================================
#endif
