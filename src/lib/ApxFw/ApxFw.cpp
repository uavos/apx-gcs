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
#include "ApxFw.h"

#include <App/App.h>
#include <App/AppDirs.h>

#include "quazip/JlCompress.h"

ApxFw::ApxFw(Fact *parent)
    : Fact(parent, "apxfw", tr("Firmware releases"), tr("Available firmware packages"), Group)
{
    setIcon("alarm-light");

    _versionPrefix = QVersionNumber::fromString(App::version());
    _versionPrefix = QVersionNumber(_versionPrefix.majorVersion(), _versionPrefix.minorVersion());

    m_packagePrefix = "APX_Nodes_Firmware";

    connect(this, &Fact::valueChanged, this, &ApxFw::updateCurrent, Qt::QueuedConnection);

    f_sync = new Fact(this, "sync", tr("Sync"), tr("Check for updates"), Action | Apply, "sync");
    connect(f_sync, &Fact::triggered, this, &ApxFw::sync);

    net.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    QTimer::singleShot(100, this, &ApxFw::sync);
}

void ApxFw::abort()
{
    if (!reply)
        return;
    reply->abort();
    reply = nullptr;
    setProgress(-1);
}

void ApxFw::sync()
{
    syncFacts();
    requestLatestTag();
    return;

    QDateTime t = QDateTime::currentDateTimeUtc();
    QDateTime t0 = QSettings().value(QString("%1_%2").arg(name()).arg(App::version())).toDateTime();
    qint64 tm = 8 * 60 * 60;
    qint64 tp = tm; //t0.secsTo(t);
    if (t0.isValid() && tp < tm) {
        apxMsgW() << QString("Firmware download in %1")
                         .arg(AppRoot::timeToString(static_cast<quint64>(tm - tp), true));

        QString s
            = QSettings().value(QString("%1_%2_latest").arg(name()).arg(App::version())).toString();
        if (!s.isEmpty()) {
            if (!extractRelease(s)) {
            }
        }
        return;
    }

    requestRelease(QString("tags/%1").arg(App::version()));
}

void ApxFw::syncFacts()
{
    if (!AppDirs::firmware().exists())
        AppDirs::firmware().mkpath(".");

    // development files
    if (f_dev)
        f_dev->deleteChildren();
    if (!devDir().entryList().isEmpty()) {
        if (!f_dev)
            f_dev = new Fact(this, "dev", "Development", "", Group);
        f_dev->setValue(QString::number(devDir().entryList().size()));
        makeFacts(f_dev, devDir());
    }

    QDir dir;
    //find existing package unzipped directory
    dir = QDir(AppDirs::firmware().absolutePath(),
               QString("%1-%2*").arg(m_packagePrefix).arg(_versionPrefix.toString()),
               QDir::NoSort,
               QDir::Dirs | QDir::NoDotAndDotDot);

    QMap<QVersionNumber, QFileInfo> mapDirs;
    for (auto fi : dir.entryInfoList()) {
        mapDirs.insert(QVersionNumber::fromString(fi.fileName().split('-').value(1)), fi);
    }
    //find release archives
    dir = QDir(AppDirs::firmware().absoluteFilePath("releases"),
               QString("%1-%2*.zip").arg(m_packagePrefix).arg(_versionPrefix.toString()),
               QDir::NoSort,
               QDir::Files);
    QMap<QVersionNumber, QFileInfo> mapPkg;
    for (auto fi : dir.entryInfoList()) {
        mapPkg.insert(QVersionNumber::fromString(fi.completeBaseName().split('-').value(1)), fi);
    }
    if (!mapPkg.isEmpty()) {
        QVersionNumber vDirs;
        if (!mapDirs.isEmpty())
            vDirs = mapDirs.lastKey();
        QVersionNumber vPkg;
        if (!mapPkg.isEmpty())
            vPkg = mapPkg.lastKey();

        if (vPkg > vDirs) {
            // extract pkg
            if (extractRelease(mapPkg.last().absoluteFilePath())) {
                mapDirs.insert(vPkg, QString("%1-%2").arg(m_packagePrefix).arg(vPkg.toString()));
            }
        }
    }

    if (!mapDirs.isEmpty()) {
        setValue(mapDirs.lastKey().toString());
    }
}

void ApxFw::updateCurrent()
{
    if (!f_current) {
        f_current = new Fact(this, "current", "", "", Group);
    }
    bool upd = f_current->size() > 0;
    f_current->deleteChildren();
    if (QVersionNumber::fromString(value().toString()).isNull())
        return;

    QDir dir = releaseDir();
    f_current->setTitle(dir.dirName());
    f_current->setValue(QString::number(dir.entryList().size()));
    makeFacts(f_current, dir);
    if (upd)
        apxMsg() << title().append(':') << tr("updated");
}

bool ApxFw::extractRelease(const QString &filePath)
{
    QFileInfo fzip(filePath);
    if (!fzip.exists())
        return false;

    QDir dir(AppDirs::firmware().absoluteFilePath(fzip.completeBaseName()));
    dir.mkpath(".");

    QStringList files = JlCompress::extractDir(fzip.absoluteFilePath(), dir.absolutePath());
    if (files.isEmpty()) {
        apxMsgW() << tr("Can't extract archive") << fzip.absoluteFilePath();
        dir.removeRecursively();
        QFile::remove(fzip.absoluteFilePath());
        return false;
    }
    qDebug() << "extracted" << files.size() << dir.absolutePath();
    apxMsg() << title().append(':') << dir.dirName();

    QSettings().setValue(QString("%1_%2_latest").arg(name()).arg(App::version()),
                         value().toString());
    return true;
}

QDir ApxFw::releaseDir() const
{
    return QDir(AppDirs::firmware().absoluteFilePath(
                    QString("%1-%2").arg(m_packagePrefix).arg(value().toString())),
                QString("*.apxfw"),
                QDir::NoSort,
                QDir::Files);
}
QDir ApxFw::devDir() const
{
    return QDir(AppDirs::firmware().absoluteFilePath("development"), "*.apxfw");
}

void ApxFw::makeFacts(Fact *fact, const QDir &dir)
{
    //create content tree
    foreach (QFileInfo fi, dir.entryInfoList()) {
        if (fi.suffix() != "apxfw")
            continue;
        QStringList st = fi.completeBaseName().split('-');
        QVersionNumber v = QVersionNumber::fromString(fi.completeBaseName().split('-').value(2));
        if (st.size() < 3 || v.isNull()) {
            qWarning() << "invalid firmware file" << fi.filePath();
            continue;
        }

        Fact *f_ng = fact->child(st.at(0));
        if (!f_ng)
            f_ng = new Fact(fact, st.at(0), "", "", Group | Count | FlatModel);
        Fact *f_hw = f_ng->child(st.at(1));
        if (!f_hw)
            f_hw = new Fact(f_ng, st.at(1).toLower(), st.at(1), "", Group | Count | Section);

        Fact *f = new Fact(f_hw,
                           fi.completeBaseName().toLower(),
                           fi.completeBaseName(),
                           fi.lastModified().toString());
        f->setValue(v.toString());
    }
}

void ApxFw::clean()
{
    QDir dir = releaseDir();
    //clean up other extracted packages
    foreach (QFileInfo fi,
             QDir(AppDirs::firmware().absolutePath(),
                  "APX_*",
                  QDir::NoSort,
                  QDir::Dirs | QDir::NoDotAndDotDot)
                 .entryInfoList()) {
        if (dir.absolutePath() == fi.absoluteFilePath())
            continue;
        QDir(fi.absoluteFilePath()).removeRecursively();
        qDebug() << "removed" << fi.absoluteFilePath();
    }
    //clean up dev firmwares
    QVersionNumber ver = QVersionNumber::fromString(App::version());
    foreach (QFileInfo fi,
             QDir(AppDirs::firmware().absolutePath(), "*.apxfw", QDir::NoSort, QDir::Files)
                 .entryInfoList()) {
        QStringList st = fi.completeBaseName().split('-');
        if (st.size() >= 3) {
            QVersionNumber fver = QVersionNumber::fromString(st.at(2));
            if (ver <= fver)
                continue;
        }
        QFile::remove(fi.absoluteFilePath());
        qDebug() << "removed" << fi.absoluteFilePath();
    }
}

QNetworkReply *ApxFw::request(const QString &r)
{
    return request(QUrl(QString("https://api.github.com/repos/uavos/apx-ap/releases%1").arg(r)));
}
QNetworkReply *ApxFw::request(const QUrl &url)
{
    QNetworkRequest *request = new QNetworkRequest(url);

    QSslConfiguration ssl = request->sslConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    request->setSslConfiguration(ssl);

    request->setRawHeader("Accept", "*/*");
    request->setRawHeader("User-Agent",
                          QString("%1 (v%2)")
                              .arg(QCoreApplication::applicationName())
                              .arg(App::version())
                              .toUtf8());

    QNetworkReply *reply = net.get(*request);
    return reply;
}

QNetworkReply *ApxFw::checkReply(QObject *sender)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender);
    if (!reply)
        return nullptr;
    reply->deleteLater();
    setProgress(-1);

    if (this->reply != reply) {
        qWarning() << "wrong reply" << reply;
        return nullptr;
    }
    this->reply = nullptr;

    if (reply->error()) {
        qWarning() << reply->errorString();
    }
    return reply;
}
bool ApxFw::isFirmwarePackageFile(const QString &s)
{
    if (!s.startsWith(m_packagePrefix + "-"))
        return false;
    if (!s.endsWith(".zip"))
        return false;
    return true;
}

void ApxFw::requestLatestTag()
{
    abort();
    setProgress(0);
    reply = request(QUrl("https://github.com/uavos/apx-ap/releases/latest"));
    connect(reply, &QNetworkReply::finished, this, &ApxFw::responseLatestTag);
}
void ApxFw::responseLatestTag()
{
    QNetworkReply *reply = checkReply(sender());
    if (!reply)
        return;
    QString s(reply->url().toString());
    qDebug() << s;
    s = s.mid(s.lastIndexOf('/') + 1);
    QVersionNumber v = QVersionNumber::fromString(s.mid(s.lastIndexOf('-') + 1));
    if (v.isNull()) {
        apxMsgW() << tr("APXFW release not found");
        qWarning() << "apxfw:" << s;
        return;
    }
    if (QVersionNumber::fromString(value().toString()) >= v) {
        qDebug() << "apxfw:"
                 << "latest";
        return;
    }
    apxMsg() << title().append(':') << v.toString() << tr("available");
    requestRelease(QString("tags/%1").arg(s));
}

void ApxFw::requestRelease(QString req)
{
    abort();
    setProgress(0);
    reply = request(QString("/%1").arg(req));
    connect(reply, &QNetworkReply::finished, this, &ApxFw::responseRelease);
}
void ApxFw::responseRelease()
{
    QNetworkReply *reply = checkReply(sender());
    if (!reply)
        return;

    QJsonParseError jserr;
    QJsonDocument json(QJsonDocument::fromJson(reply->readAll(), &jserr));
    if (jserr.error != QJsonParseError::NoError) {
        qWarning() << jserr.errorString();
        return;
    }
    if (json.object().contains("message")) {
        QString msg = json["message"].toString();
        apxMsgW() << title().append(':') << msg;
        if (msg.toLower() == "not found") {
            QDateTime t = QDateTime::currentDateTimeUtc();
            QSettings().setValue(QString("%1_%2").arg(name()).arg(App::version()), t);
            apxMsgW() << title().append(':') << tr("Requesting latest release");
            requestRelease("latest");
            return;
        }
    }
    while (json["assets"].toArray().count() > 0) {
        if (json["author"]["login"].toString() != "uavinda")
            break;
        //find asset
        QJsonArray ja = json["assets"].toArray();
        QUrl url;
        for (int i = 0; i < ja.size(); ++i) {
            QJsonObject jo = ja.at(i).toObject();
            QString s = jo["name"].toString();
            if (!isFirmwarePackageFile(s))
                continue;
            if (extractRelease(s)) {
                apxMsg() << title().append(':') << QFileInfo(s).completeBaseName();
                return;
            }
            url = QUrl(jo["browser_download_url"].toString());
            if (url.isValid())
                break;
        }
        if (!url.isValid()) {
            apxMsgW() << title().append(':') << "Missing asset";
            break;
        }
        requestDownload(url);
        qDebug() << "download" << url.toString();
        return;
    }
    apxMsgW() << title().append(':') << tr("Firmware not available");
    qWarning() << json;
}

void ApxFw::requestDownload(QUrl url)
{
    apxMsg() << title().append(':') << tr("Downloading firmware").append("...");
    abort();
    setProgress(0);
    reply = request(url);
    connect(reply, &QNetworkReply::finished, this, &ApxFw::responseDownload);
    connect(reply, &QNetworkReply::downloadProgress, this, &ApxFw::downloadProgress);
}
void ApxFw::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        setProgress((bytesReceived * 100) / bytesTotal);
    }
}
void ApxFw::responseDownload()
{
    QNetworkReply *reply = checkReply(sender());
    if (!reply)
        return;
    if (reply->error())
        return;
    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        qWarning() << "no data";
        return;
    }
    QString s = reply->header(QNetworkRequest::ContentDispositionHeader).toString();
    s = s.mid(s.lastIndexOf("filename=", Qt::CaseInsensitive));
    s = s.mid(s.indexOf("=") + 1);
    s = s.left(s.indexOf(";"));
    if (!isFirmwarePackageFile(s)) {
        apxMsgW() << title().append(':') << tr("Unknown response") << s;
        return;
    }

    QDir dir(AppDirs::firmware().absoluteFilePath("releases"));
    dir.mkpath(".");
    QFile fzip(dir.absoluteFilePath(s));
    if (!fzip.open(QFile::WriteOnly)) {
        qWarning() << "can't write" << fzip.fileName();
        return;
    }
    fzip.write(data);
    fzip.close();
    qDebug() << "downloaded" << fzip.fileName();
    if (extractRelease(fzip.fileName())) {
        syncFacts();
    }
}

QString ApxFw::getApxfwFileName(QString nodeName, QString hw)
{
    //find fw
    QString fname = QString("%1-%2").arg(nodeName).arg(hw);
    int ccnt = fname.split('-').size() + 1;
    QMap<QVersionNumber, QString> vmap;
    QString target_os("any");
#if defined(Q_OS_MAC)
    target_os = "darwin";
#elif defined(Q_OS_LINUX)
    target_os = "linux";
#endif
    //search fw package
    QDir dir(releaseDir().absolutePath(), QString("%1-*.apxfw").arg(fname));
    foreach (QFileInfo fi, dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        QStringList st(fi.completeBaseName().split('-'));
        if (st.size() < ccnt)
            continue;
        if (st.size() > ccnt && st.at(ccnt) != target_os)
            continue;
        vmap[QVersionNumber::fromString(st.at(ccnt - 1))] = fi.absoluteFilePath();
    }
    //search dev files
    dir = QDir(devDir().absolutePath(), QString("%1-*.apxfw").arg(fname));
    foreach (QFileInfo fi, dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        QStringList st(fi.completeBaseName().split('-'));
        if (st.size() < ccnt)
            continue;
        if (st.size() > ccnt && st.at(ccnt) != target_os)
            continue;
        vmap[QVersionNumber::fromString(st.at(ccnt - 1))] = fi.absoluteFilePath();
    }

    QString fileName;
    if (vmap.contains(QVersionNumber::fromString(App::version()))) {
        fileName = vmap.value(QVersionNumber::fromString(App::version()));
    } else if (!vmap.isEmpty()) {
        fileName = vmap.last();
    } else {
        apxMsgW() << tr("Firmware file not found").append(':') << fname;
        qWarning() << vmap;
        return QString();
    }
    return fileName;
}

bool ApxFw::loadFirmware(
    QString nodeName, QString hw, QString type, QByteArray *data, quint32 *startAddr)
{
    QString fileName = getApxfwFileName(nodeName, hw);

    if (fileName.isEmpty())
        return false;

    if (QFileInfo(fileName).absolutePath().startsWith(devDir().absolutePath())) {
        apxMsgW() << "Development firmware:" << QFileInfo(fileName).completeBaseName();
    }

    //load fw
    qDebug() << fileName;
    bool rv = false;
    if (fileName.endsWith(".hex", Qt::CaseInsensitive)) {
        rv = loadHexFile(fileName, data, startAddr);
    } else {
        rv = loadApfwFile(fileName, type, data, startAddr);
    }
    if (!rv) {
        apxMsgW() << tr("Can't load firmware file");
    }
    return rv;
}

bool ApxFw::loadHexFile(QString fileName, QByteArray *data, quint32 *startAddr)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning("%s",
                 QString(tr("Cannot read file") + " %1:\n%2.")
                     .arg(fileName)
                     .arg(file.errorString())
                     .toUtf8()
                     .data());
        return false;
    }
    QTextStream stream(&file);
    QByteArray ba;
    uint max_size = 4 * 1024 * 1024; //PM_SIZE];  //PM data from file
    ba.resize(max_size);
    uint8_t *file_data = (uint8_t *) ba.data();
    //read file
    bool data_addr_init = false;
    uint lcnt = 0;
    uint ExtAddr = 0;
    uint data_cnt = 0;  //number of data bytes in array
    uint data_addr = 0; //start address of data in array
    uint cnt = 0;
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        const char *bufLine = line.toUtf8().data();
        int ByteCount = 0, Address = 0, RecordType, v;
        sscanf(bufLine + 1, "%2x%4x%2x", &ByteCount, &Address, &RecordType);
        lcnt++;
        switch (RecordType) {
        case 0: //data record
            Address += ExtAddr;
            if (!data_addr_init) {
                data_addr_init = true;
                data_addr = Address;
            }
            if (Address < (int) data_addr) {
                qWarning("%s (0x%.8X, line %u)\n",
                         tr("Data address too low").toUtf8().data(),
                         Address,
                         lcnt);
            } else {
                for (int CharCount = 0; CharCount < ByteCount * 2; CharCount += 2, Address++) {
                    sscanf(bufLine + 9 + CharCount, "%2x", &v);
                    if (Address >= (int) (data_addr + max_size)) {
                        qWarning("%s (0x%.8X, line %u)\n",
                                 tr("Data address too high").toUtf8().data(),
                                 Address,
                                 lcnt);
                        break;
                    }
                    uint i = Address - data_addr;
                    file_data[i] = v;
                    cnt++;
                    if (data_cnt < (i + 1))
                        data_cnt = i + 1;
                    // get device type from file
                    //if ((dtype>0xff) && (Address==dtype_addr))dtype=v;
                }
            }
            break;
        case 1: //End Of File
            break;
        case 5: //Start Linear Address Record. EIP register of the 80386 and higher CPU.
            break;
        case 4: //Extended Linear Address Record
            sscanf(bufLine + 9, "%4x", &ExtAddr);
            ExtAddr = ExtAddr << 16;
            break;
        case 2: //Extended Linear Address Record
            sscanf(bufLine + 9, "%4x", &ExtAddr);
            ExtAddr = ExtAddr << 4;
            break;
        default:
            qWarning("%s: %2X (line %u)\n",
                     tr("Unknown hex record type").toUtf8().data(),
                     RecordType,
                     lcnt);
        }
    }
    ba.resize(cnt);
    *data = ba;
    if (startAddr)
        *startAddr = data_addr;
    qDebug() << "File: " << ba.size();
    return ba.size() > 0;
}

bool ApxFw::loadApfwFile(QString fileName, QString section, QByteArray *data, quint32 *startAddr)
{
    if (fileName.isEmpty())
        return false;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning("%s",
                 QString(tr("Cannot read file") + " %1:\n%2.")
                     .arg(fileName)
                     .arg(file.errorString())
                     .toUtf8()
                     .data());
        return false;
    }
    QJsonParseError err;
    QString errString;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    while (1) {
        errString = err.errorString();
        if (err.error != QJsonParseError::NoError)
            break;
        errString = "object";
        if (!doc.isObject())
            break;
        QVariantMap m = doc.object().toVariantMap();
        if (m.value("magic").toString() != "APXFW") {
            errString = "magic";
            break;
        }

        QVariantMap msect = m.value(section).toMap();
        if (msect.isEmpty()) {
            errString = "missing section '" + section + "'";
            break;
        }

        quint32 size = msect.value("size").toUInt();
        if (!size) {
            errString = "zero size";
            break;
        }

        if (startAddr) {
            if (!msect.contains("offset")) {
                errString = "missing offset";
                break;
            }
            *startAddr = msect.value("offset").toUInt();
        }

        QByteArray ba = QByteArray::fromBase64(msect.value("data").toString().toUtf8());
        errString = "missing image data";
        if (ba.isEmpty())
            break;
        ba.prepend(static_cast<char>(size));
        ba.prepend(static_cast<char>(size >> 8));
        ba.prepend(static_cast<char>(size >> 16));
        ba.prepend(static_cast<char>(size >> 24));
        ba = qUncompress(ba);
        errString = "unzip";
        if (static_cast<quint32>(ba.size()) != size)
            break;
        uint32_t adr = startAddr ? *startAddr : 0;
        qDebug() << "File: " << QString::number(adr, 16) << ba.size();
        *data = ba;
        return ba.size() > 0;
    }
    apxMsgW() << tr("Error parsing firmware file") << QString("(%1)").arg(errString);
    return false;
}

bool ApxFw::loadFileMHX(QString ver, QByteArray *data)
{
    bool bErr = true;
    //parse version
    QString mname;
    uint iver = 0;
    while (1) {
        if (!ver.contains('_'))
            break;
        mname = ver.left(ver.indexOf('_'));
        QString s = ver.mid(ver.indexOf('_') + 1);
        if (s.startsWith('v', Qt::CaseInsensitive))
            s.remove(0, 1);
        iver = s.toFloat() * 1000.0;
        if (iver < 1000 || mname.size() < 4)
            break;
        qDebug("MHX radio: %s (ver %u)", mname.toUpper().toUtf8().data(), iver);
        bErr = false;
        break;
    }
    if (bErr) {
        qWarning("%s (%s)", tr("Error parsing MHX version").toUtf8().data(), ver.toUtf8().data());
        return false;
    }
    //load corresponding file
    QString fileName;
    while (1) {
        QString prefix(AppDirs::res().absoluteFilePath("firmware"));
        QDir dir(prefix);
        if (!dir.cd("mhx"))
            break;
        foreach (QFileInfo fi, dir.entryInfoList()) {
            QString s = fi.baseName();
            if (!s.startsWith(mname, Qt::CaseInsensitive))
                continue;
            s = s.mid(ver.indexOf('_') + 1);
            if (s.startsWith('v', Qt::CaseInsensitive))
                s.remove(0, 1);
            uint iv = s.toUInt();
            if (iv < 1000)
                continue;
            if (iv <= iver) {
                bErr = false;
                qDebug("%s: %s",
                       tr("Older firmware file").toUtf8().data(),
                       fi.baseName().toUtf8().data());
                continue;
            }
            qDebug("%s: %s", tr("New firmware").toUtf8().data(), fi.baseName().toUtf8().data());
            fileName = fi.absoluteFilePath();
            bErr = false;
            break;
        }
        break;
    }
    if (bErr) {
        qWarning("%s", tr("MHX firmware files not found").toUtf8().data());
        return false;
    }
    if (fileName.isEmpty())
        return false;
    setValue(tr("Loading file") + "...");
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        qWarning("%s",
                 QString(tr("Cannot read file") + " %1:\n%2.")
                     .arg(fileName)
                     .arg(file.errorString())
                     .toUtf8()
                     .data());
        return false;
    }

    *data = file.readAll();
    //qDebug()<<fileData.size();
    return true;
}

QJsonArray ApxFw::loadParameters(QString nodeName, QString hw)
{
    QFile file(getApxfwFileName(nodeName, hw));
    if (!file.exists() || !file.open(QFile::ReadOnly | QFile::Text)) {
        apxMsgW() << tr("Firmware package missing") << nodeName << hw;
    }
    QJsonDocument json = QJsonDocument::fromJson(file.readAll());
    file.close();

    do {
        QJsonObject params = json["parameters"].toObject();
        if (params.isEmpty())
            break;
        QByteArray ba = QByteArray::fromBase64(params["data"].toString().toUtf8());
        if (ba.isEmpty())
            break;

        quint32 size = params["size"].toInt();

        ba.prepend(static_cast<char>(size));
        ba.prepend(static_cast<char>(size >> 8));
        ba.prepend(static_cast<char>(size >> 16));
        ba.prepend(static_cast<char>(size >> 24));
        ba = qUncompress(ba);
        if (ba.isEmpty())
            break;

        json = QJsonDocument::fromJson(ba);
        if (!json.isArray())
            break;
        return json.array();
    } while (0);
    apxMsgW() << tr("Parameters package error") << nodeName << hw;
    return QJsonArray();
}
