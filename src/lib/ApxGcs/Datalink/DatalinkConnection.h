/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <Fact/Fact.h>
#include <QtCore>

#include <serial/SerialCodec.h>

class DatalinkConnection : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)

    Q_PROPERTY(bool activated READ activated WRITE setActivated NOTIFY activatedChanged)

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

    void setEncoder(SerialEncoder *encoder);
    void setDecoder(SerialDecoder *decoder);

public:
    QString url() const;
    void setUrl(const QString &v);
    QString status() const;
    void setStatus(const QString &v);

    bool activated() const;
    void setActivated(const bool &v);

    quint16 rxNetwork() const;
    void setRxNetwork(const quint16 &v);
    quint16 txNetwork() const;
    void setTxNetwork(const quint16 &v);

    bool blockControls() const;
    void setBlockControls(const bool &v);
    bool blockService() const;
    void setBlockService(const bool &v);

protected:
    // helpers
    bool isControlPacket(const QByteArray &packet) const;
    virtual void resetDataStream();

    // interface with codec implementation
    virtual QByteArray read();
    virtual void write(const QByteArray &packet);

    SerialEncoder *_encoder{};
    SerialDecoder *_decoder{};

private:
    // receiver fifo and packets queue
    QQueue<QByteArray> _rx_fifo;
    QByteArray _readPacket();

private:
    QString m_url;
    QString m_status;
    bool m_activated;

    quint16 m_rxNetwork;
    quint16 m_txNetwork;

    bool m_blockControls;
    bool m_blockService;

protected slots:
    void updateDescr();
    void updateTitle();

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

signals:
    void urlChanged();
    void statusChanged();

    void activatedChanged();

    void rxNetworkChanged();
    void txNetworkChanged();

    void blockControlsChanged();
    void blockServiceChanged();
};
