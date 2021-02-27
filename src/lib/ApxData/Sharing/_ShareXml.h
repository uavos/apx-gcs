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

#include <QDomDocument>
#include <QtCore>

#include <Database/DatabaseRequest.h>

class ShareXml : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit ShareXml(
        DatabaseSession *db, QString tagName, int format, QString title, QString fileName)
        : DatabaseRequest(db)
        , title(title)
        , tagName(tagName)
        , format(format)
        , fileName(fileName)
    {}
    QString title;
    //result
    QVariantMap info;

protected:
    QString tagName;
    int format;
    QString fileName;

    //helpers
public:
    static void writeInfo(QDomNode &dom,
                          QString name,
                          QVariantMap info,
                          QStringList filter = QStringList());
    static QVariantMap readInfo(const QDomNode &dom, QString name);
};

class ShareXmlExport : public ShareXml
{
    Q_OBJECT
public:
    explicit ShareXmlExport(
        DatabaseSession *db, QString tagName, int format, QString title, QString fileName)
        : ShareXml(db, tagName, format, title, fileName)
    {}
    //result
    QByteArray data;

protected:
    virtual bool run(QSqlQuery &query);
    virtual bool write(QDomNode &dom);

signals:
    void exported(QByteArray data, QString fileName);
};

class ShareXmlImport : public ShareXml
{
    Q_OBJECT
public:
    explicit ShareXmlImport(
        DatabaseSession *db, QString tagName, int format, QString title, QString fileName)
        : ShareXml(db, tagName, format, title, fileName)
    {}

    QByteArray data;
    qint64 defaultTime;

protected:
    virtual bool read(const QDomNode &dom);
    virtual bool readOldFormat(const QDomNode &dom, int fmt);
    virtual bool save(QSqlQuery &query);

private:
    bool readData(QSqlQuery &query);
    bool readData(const QDomNode &dom);

protected:
    bool run(QSqlQuery &query);
signals:
    void imported(QString hash, QString title);
};
