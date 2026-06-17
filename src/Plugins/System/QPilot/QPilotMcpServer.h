/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2026, QPilot contributors
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

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>

class QPilotTelemetry;
class QTcpServer;
class QTcpSocket;

class QPilotMcpServer : public QObject
{
    Q_OBJECT

public:
    explicit QPilotMcpServer(QPilotTelemetry *telemetry, QObject *parent = nullptr);
    ~QPilotMcpServer() override;

    void start();
    void stop();

private:
    QPilotTelemetry *_telemetry = nullptr;
    QTcpServer *_server = nullptr;

    static constexpr quint16 _port = 9876;

    void acceptConnection();
    void readRequest(QTcpSocket *socket);
    void handleMcp(QTcpSocket *socket, const QJsonObject &request);

    void sendResult(QTcpSocket *socket, const QJsonValue &id, const QJsonObject &result);
    void sendError(QTcpSocket *socket, const QJsonValue &id, int code, const QString &message);
    void sendJson(QTcpSocket *socket, const QJsonObject &object);
};
