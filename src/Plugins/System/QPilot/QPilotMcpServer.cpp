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
#include "QPilotMcpServer.h"

#include "QPilotTelemetry.h"

#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTcpSocket>

QPilotMcpServer::QPilotMcpServer(QPilotTelemetry *telemetry, QObject *parent)
    : QObject(parent)
    , _telemetry(telemetry)
{
}

QPilotMcpServer::~QPilotMcpServer()
{
    stop();
}

void QPilotMcpServer::start()
{
    if (_server) {
        return;
    }

    _server = new QTcpServer(this);

    connect(_server, &QTcpServer::newConnection, this, &QPilotMcpServer::acceptConnection);

    if (!_server->listen(QHostAddress::LocalHost, _port)) {
        qWarning().noquote() << "[QPilot] MCP listen failed:" << _server->errorString();

        _server->deleteLater();
        _server = nullptr;

        return;
    }

    qInfo().noquote() << "[QPilot] MCP listening on http://127.0.0.1:" << _port << "/mcp";
}

void QPilotMcpServer::stop()
{
    if (!_server) {
        return;
    }

    _server->close();
    _server->deleteLater();
    _server = nullptr;

    qInfo().noquote() << "[QPilot] MCP server stopped";
}

void QPilotMcpServer::acceptConnection()
{
    while (_server && _server->hasPendingConnections()) {
        QTcpSocket *socket = _server->nextPendingConnection();

        if (!socket) {
            continue;
        }

        socket->setParent(this);

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            readRequest(socket);
        });

        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void QPilotMcpServer::readRequest(QTcpSocket *socket)
{
    QByteArray buffer = socket->property("qpilot_buffer").toByteArray() + socket->readAll();

    const int headerEnd = buffer.indexOf("\r\n\r\n");

    if (headerEnd < 0) {
        socket->setProperty("qpilot_buffer", buffer);
        return;
    }

    const QByteArray header = buffer.left(headerEnd);
    const QByteArray bodyPart = buffer.mid(headerEnd + 4);
    const QList<QByteArray> lines = header.split('\n');
    const QList<QByteArray> firstLine = lines.value(0).trimmed().split(' ');

    if (firstLine.size() < 2
        || firstLine.value(0).trimmed().toUpper() != "POST"
        || firstLine.value(1).trimmed() != "/mcp") {
        sendError(socket, QJsonValue(), -32600, "Only POST /mcp is supported");
        return;
    }

    int contentLength = 0;

    for (const QByteArray &rawLine : lines) {
        const QByteArray line = rawLine.trimmed();
        const int colon = line.indexOf(':');

        if (colon > 0 && line.left(colon).trimmed().toLower() == "content-length") {
            contentLength = line.mid(colon + 1).trimmed().toInt();
            break;
        }
    }

    if (contentLength <= 0 || contentLength > 1024 * 1024) {
        sendError(socket, QJsonValue(), -32600, "Invalid Content-Length");
        return;
    }

    if (bodyPart.size() < contentLength) {
        socket->setProperty("qpilot_buffer", buffer);
        return;
    }

    socket->setProperty("qpilot_buffer", QByteArray());

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(bodyPart.left(contentLength), &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        sendError(socket, QJsonValue(), -32700, "Parse error");
        return;
    }

    handleMcp(socket, doc.object());
}

void QPilotMcpServer::handleMcp(QTcpSocket *socket, const QJsonObject &request)
{
    const QJsonValue id = request.value("id");
    const QString method = request.value("method").toString();

    if (request.value("jsonrpc").toString() != "2.0") {
        sendError(socket, id, -32600, "Invalid JSON-RPC request");
        return;
    }

    if (method == "notifications/initialized") {
        QByteArray response;
        response += "HTTP/1.1 202 Accepted\r\n";
        response += "Connection: close\r\n";
        response += "Content-Length: 0\r\n\r\n";

        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();

        return;
    }

    if (method == "initialize") {
        QJsonObject tools;
        tools["listChanged"] = false;

        QJsonObject capabilities;
        capabilities["tools"] = tools;

        QJsonObject serverInfo;
        serverInfo["name"] = "qpilot";
        serverInfo["version"] = "0.3.0";

        QJsonObject result;
        result["protocolVersion"] = "2025-06-18";
        result["capabilities"] = capabilities;
        result["serverInfo"] = serverInfo;
        result["instructions"] =
            "QPilot is a read-only APX Ground Control telemetry MCP server. "
            "Use gcs_telemetry_json to read collected Mandala leaf facts, ECAM telemetry from http://172.29.100.57:9380/ecam, "
            "current values, total statistics, 30-second window statistics, HAPS derived facts when available, "
            "and a LiPo battery charge summary when battery facts are available. "
            "QPilot does not write facts, execute commands, upload missions, or control vehicles.";

        sendResult(socket, id, result);
        return;
    }

    if (method == "ping") {
        sendResult(socket, id, QJsonObject{});
        return;
    }

    if (method == "tools/list") {
        QJsonObject ids;
        ids["type"] = "string";
        ids["description"] =
            "Optional comma-separated telemetry identifiers. "
            "Mandala facts use their original ids, ECAM facts use the ecam.* prefix, "
            "and HAPS derived facts use the haps.* prefix. "
            "If omitted, all collected telemetry facts are returned.";

        QJsonObject batteryCells;
        batteryCells["type"] = "integer";
        batteryCells["description"] =
            "Battery cell count for voltage-based charge estimate. Default is 6.";

        QJsonObject properties;
        properties["ids"] = ids;
        properties["battery_cells"] = batteryCells;

        QJsonObject inputSchema;
        inputSchema["type"] = "object";
        inputSchema["properties"] = properties;
        inputSchema["additionalProperties"] = false;

        QJsonObject tool;
        tool["name"] = "gcs_telemetry_json";
        tool["description"] =
            "Read QPilot telemetry JSON from APX Ground Control. "
            "Returns current values for collected Mandala facts, ECAM facts, HAPS derived facts, "
            "total statistics, 30-second window statistics, and battery summary. "
            "Read-only. No raw flight history is returned.";
        tool["inputSchema"] = inputSchema;

        sendResult(socket, id, QJsonObject{{"tools", QJsonArray{tool}}});
        return;
    }

    if (method == "tools/call") {
        const QJsonObject params = request.value("params").toObject();
        const QString name = params.value("name").toString();
        const QJsonObject arguments = params.value("arguments").toObject();

        QJsonObject payload;

        if (name == "gcs_telemetry_json") {
            payload = _telemetry->toolJson(arguments.value("ids").toString(),
                                           arguments.value("battery_cells").toInt(6));
        } else {
            payload["ok"] = false;
            payload["error"] = "unknown_tool";
            payload["tool"] = name;
        }

        QJsonObject item;
        item["type"] = "text";
        item["text"] = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));

        sendResult(socket,
                   id,
                   QJsonObject{
                       {"content", QJsonArray{item}},
                       {"isError", !payload.value("ok").toBool(false)}
                   });

        return;
    }

    sendError(socket, id, -32601, "Method not found");
}

void QPilotMcpServer::sendResult(QTcpSocket *socket, const QJsonValue &id, const QJsonObject &result)
{
    sendJson(socket,
             QJsonObject{
                 {"jsonrpc", "2.0"},
                 {"id", id},
                 {"result", result}
             });
}

void QPilotMcpServer::sendError(QTcpSocket *socket,
                                const QJsonValue &id,
                                int code,
                                const QString &message)
{
    sendJson(socket,
             QJsonObject{
                 {"jsonrpc", "2.0"},
                 {"id", id},
                 {"error", QJsonObject{
                     {"code", code},
                     {"message", message}
                 }}
             });
}

void QPilotMcpServer::sendJson(QTcpSocket *socket, const QJsonObject &object)
{
    const QByteArray body = QJsonDocument(object).toJson(QJsonDocument::Compact);

    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json; charset=utf-8\r\n";
    response += "Connection: close\r\n";
    response += "Content-Length: ";
    response += QByteArray::number(body.size());
    response += "\r\n\r\n";
    response += body;

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}
