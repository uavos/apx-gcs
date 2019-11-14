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
#include "Releases.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <JlCompress.h>
//=============================================================================
Releases::Releases(Fact *parent)
    : Fact(parent, "releases", tr("Releases"), tr("Available firmware packages"), Group | Const)
    , f_current(nullptr)
    , f_dev(nullptr)
    , reply(nullptr)
{
    setIcon("alarm-light");

    m_packagePrefix = "APX_Nodes_Firmware";

    f_sync = new Fact(this, "sync", tr("Sync"), tr("Check for updates"), Action | Apply, "sync");
    connect(f_sync, &Fact::triggered, this, &Releases::sync);

    net.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    QTimer::singleShot(2300, this, &Releases::sync);
}
//=============================================================================
void Releases::abort()
{
    if (!reply)
        return;
    reply->abort();
    reply = nullptr;
    setProgress(-1);
}
//=============================================================================
void Releases::sync()
{
    m_releaseFile.clear();
    if (!AppDirs::firmware().exists())
        AppDirs::firmware().mkpath(".");
    //find existing package
    QDir dir = releaseDir();
    if (dir.exists() && (!dir.entryList().isEmpty())) {
        qDebug() << "firmware package available" << dir.absolutePath();
        makeReleaseFact(dir);
        return;
    }
    if (dir.exists() && (!dir.removeRecursively())) {
        apxMsgW() << tr("Can't remove directory") << dir.absolutePath();
        return;
    }
    //find release archive
    if (extractRelease(QString("%1-%2.zip").arg(m_packagePrefix).arg(App::version())))
        return;
    requestRelease(QString("tags/%1").arg(App::version()));
}
//=============================================================================
bool Releases::extractRelease(const QString &fname)
{
    QDir dir = releaseDir();
    QFileInfo fzip(QDir(AppDirs::firmware().absoluteFilePath("releases")).absoluteFilePath(fname));
    if (!fzip.exists())
        return false;
    dir.mkpath(".");
    QStringList files = JlCompress::extractDir(fzip.absoluteFilePath(), dir.absolutePath());
    if (files.isEmpty()) {
        apxMsgW() << tr("Can't extract archive") << fzip.absoluteFilePath();
        dir.removeRecursively();
        QFile::remove(fzip.absoluteFilePath());
        return false;
    }
    qDebug() << "extracted" << files.size() << dir.absolutePath();
    dir.refresh();
    makeReleaseFact(dir);
    apxMsg() << title().append(':') << tr("Firmware available");
    return true;
}
//=============================================================================
QDir Releases::releaseDir() const
{
    return QDir(AppDirs::firmware().absoluteFilePath(
                    QString("%1-%2").arg(m_packagePrefix).arg(releaseVersion())),
                "*.apxfw");
}
QDir Releases::devDir() const
{
    return QDir(AppDirs::firmware().absoluteFilePath("development"), "*.apxfw");
}
//=============================================================================
QString Releases::releaseVersion() const
{
    QString s = App::version();
    if (!m_releaseFile.isEmpty()) {
        s = QFileInfo(m_releaseFile).completeBaseName();
        s = s.mid(s.lastIndexOf('-') + 1);
        QVersionNumber v = QVersionNumber::fromString(s);
        if (!v.isNull())
            s = v.toString();
    }
    return s;
}
//=============================================================================
void Releases::makeReleaseFact(const QDir &dir)
{
    if (!f_current) {
        f_current = new Fact(this, "current", "", "", Group);
        clean();
        f_sync->setEnabled(false);
    }
    f_current->setTitle(dir.dirName());
    f_current->setStatus(QString::number(dir.entryList().size()));
    makeReleaseFactDo(f_current, dir);

    if (!f_dev && !devDir().entryList().isEmpty()) {
        f_dev = new Fact(this, "dev", "Development", "", Group);
        f_dev->setStatus(QString::number(devDir().entryList().size()));
        makeReleaseFactDo(f_dev, devDir());
    }
}
void Releases::makeReleaseFactDo(Fact *fact, const QDir &dir)
{
    //create content tree
    foreach (QFileInfo fi, dir.entryInfoList()) {
        if (fi.suffix() != "apxfw")
            continue;
        QStringList st = fi.completeBaseName().split('-');
        if (st.size() < 3) {
            qWarning() << "invalid firmware file" << fi.filePath();
            continue;
        }
        Fact *f_ng = fact->child(st.at(0));
        if (!f_ng)
            f_ng = new Fact(fact, st.at(0), "", "", Group);
        Fact *f_hw = f_ng->child(st.at(1));
        if (!f_hw)
            f_hw = new Fact(f_ng, st.at(1).toLower(), st.at(1), "", Group);

        Fact *f = new Fact(f_hw,
                           fi.completeBaseName().toLower(),
                           QString("%1: %2").arg(f_ng->title()).arg(f_hw->title()),
                           fi.completeBaseName());
        f->setStatus("APXFW");
    }
}
//=============================================================================
void Releases::clean()
{
    QDir dir = releaseDir();
    //clean up other extracted packages
    foreach (QFileInfo fi,
             QDir(AppDirs::firmware().absolutePath(),
                  "*",
                  QDir::NoSort,
                  QDir::Dirs | QDir::NoDotAndDotDot)
                 .entryInfoList()) {
        if (dir.absolutePath() == fi.absoluteFilePath())
            continue;
        if (!fi.baseName().startsWith("APX"))
            continue;
        QDir(fi.absoluteFilePath()).removeRecursively();
        qDebug() << "removed" << fi.absoluteFilePath();
    }
    //clean up dev firmwares
    QVersionNumber ver = QVersionNumber::fromString(App::version());
    foreach (QFileInfo fi,
             QDir(AppDirs::firmware().absolutePath(), "*.apxfw", QDir::NoSort, QDir::Files)
                 .entryInfoList()) {
        QString s = fi.completeBaseName();
        s.remove(0, s.lastIndexOf('-') + 1);
        if (s.contains('.')) {
            QVersionNumber fver = QVersionNumber::fromString(s);
            if (ver <= fver)
                continue;
        }
        QFile::remove(fi.absoluteFilePath());
        qDebug() << "removed" << fi.absoluteFilePath();
    }
}
//=============================================================================
//=============================================================================
QNetworkReply *Releases::request(const QString &r)
{
    return request(
        QUrl(QString("https://api.github.com/repos/uavos/apx-releases/releases%1").arg(r)));
}
QNetworkReply *Releases::request(const QUrl &url)
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
//=============================================================================
QNetworkReply *Releases::checkReply(QObject *sender)
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
bool Releases::isFirmwarePackageFile(const QString &s, const QString &ver)
{
    if (!ver.isEmpty())
        return s == QString("%1-%2.zip").arg(m_packagePrefix).arg(ver);
    if (!s.startsWith(m_packagePrefix + "-"))
        return false;
    if (!s.endsWith(".zip"))
        return false;
    return true;
}
//=============================================================================
//=============================================================================
void Releases::requestRelease(QString req)
{
    abort();
    setProgress(0);
    reply = request(QString("/%1").arg(req));
    connect(reply, &QNetworkReply::finished, this, &Releases::responseRelease);
}
void Releases::responseRelease()
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
            m_releaseFile = s;
            if (extractRelease(s)) {
                apxMsg() << title().append(':') << QFileInfo(s).completeBaseName();
                return;
            }
            m_releaseFile.clear();
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
//=============================================================================
void Releases::requestDownload(QUrl url)
{
    apxMsg() << title().append(':') << tr("Downloading firmware").append("...");
    abort();
    setProgress(0);
    reply = request(url);
    connect(reply, &QNetworkReply::finished, this, &Releases::responseDownload);
    connect(reply, &QNetworkReply::downloadProgress, this, &Releases::downloadProgress);
}
void Releases::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        setProgress((bytesReceived * 100) / bytesTotal);
    }
}
void Releases::responseDownload()
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
    if (!isFirmwarePackageFile(s, App::version())) {
        apxMsg() << title().append(':') << tr("Received") << s;
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
    m_releaseFile = s;
    if (!extractRelease(s)) {
        m_releaseFile.clear();
    }
}
//=============================================================================
//=============================================================================
//=============================================================================
QString Releases::getApxfwFileName(QString nodeName, QString hw)
{
    //map deprecated hardware
    if (nodeName == "gimbal")
        nodeName = "nav";
    else if (hw == "AP7" && nodeName == "servo")
        hw = "HB1";
    else if (hw == "AP8" && nodeName == "servo")
        hw = "HB2";
    else if (hw == "AP9" && nodeName == "servo")
        hw = "HB3";
    else if (hw == "AP10" && nodeName == "servo")
        hw = "HB4";
    else if (hw == "AP9" && nodeName == "bldc") {
        hw = "BM1";
        nodeName = "servo";
    } else if (hw == "AP9R1" && nodeName == "bldc") {
        hw = "BM2";
        nodeName = "servo";
    } else if (hw == "AP9R1" && nodeName == "cas")
        hw = "AP9";
    else if (hw == "AP9R1" && nodeName == "mhx")
        hw = "AP9";
    else if (hw == "AP9R1" && nodeName == "ifc")
        hw = "AP9";
    else if (hw == "RUS" && nodeName == "ifcs") {
        hw = "RS1";
        nodeName = "ifc";
    } else if (hw == "AP9" && nodeName == "swc") {
        hw = "SW1";
        nodeName = "ifc";
    } else if (hw == "RUS" && nodeName == "swcm") {
        hw = "SW2";
        nodeName = "ifc";
    } else if (hw == "AP10" && nodeName == "ers")
        hw = "RS1";
    else if (hw == "AP9" && nodeName == "jsw")
        hw = "RS1";
    else if (hw == "AP10" && nodeName == "jsw")
        hw = "RS2";
    else if (hw == "AP9" && nodeName == "ghanta")
        hw = "BR1";
    else if (hw == "RUS" && nodeName == "ghanta")
        hw = "RS1";
    //find fw
    QString fname = QString("%1-%2").arg(nodeName).arg(hw);
    int ccnt = fname.split('-').size() + 1;
    QMap<QVersionNumber, QString> vmap;
    //search fw package
    QDir dir(releaseDir().absolutePath(), QString("%1-*.apxfw").arg(fname));
    foreach (QFileInfo fi, dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        QStringList st(fi.completeBaseName().split('-'));
        if (st.size() != ccnt)
            continue;
        vmap[QVersionNumber::fromString(st.last())] = fi.absoluteFilePath();
    }
    //search dev files
    dir = QDir(devDir().absolutePath(), QString("%1-*.apxfw").arg(fname));
    foreach (QFileInfo fi, dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        QStringList st(fi.completeBaseName().split('-'));
        if (st.size() != ccnt)
            continue;
        vmap[QVersionNumber::fromString(st.last())] = fi.absoluteFilePath();
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
    if (QFileInfo(fileName).absolutePath().startsWith(devDir().absolutePath())) {
        apxMsgW() << "Development firmware:" << QFileInfo(fileName).completeBaseName();
    }
    return fileName;
}
//=============================================================================
bool Releases::loadFirmware(
    QString nodeName, QString hw, Firmware::UpgradeType type, QByteArray *data, quint32 *startAddr)
{
    QString fileName = getApxfwFileName(nodeName, hw);

    //load fw
    qDebug() << fileName;
    bool rv = false;
    if (fileName.endsWith(".hex", Qt::CaseInsensitive)) {
        rv = loadHexFile(fileName, data, startAddr);
    } else {
        rv = loadApfwFile(fileName,
                          (type == Firmware::LD || type == Firmware::STM_LD) ? "loader" : "firmware",
                          data,
                          startAddr);
    }
    if (!rv) {
        apxMsgW() << tr("Can't load firmware file");
    }
    return rv;
}
//=============================================================================
//=============================================================================
bool Releases::loadHexFile(QString fileName, QByteArray *data, quint32 *startAddr)
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
    *startAddr = data_addr;
    qDebug() << "File: " << ba.size();
    return ba.size() > 0;
}
//=============================================================================
bool Releases::loadApfwFile(QString fileName, QString section, QByteArray *data, quint32 *startAddr)
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

        if (!msect.contains("offset")) {
            errString = "missing offset";
            break;
        }
        *startAddr = msect.value("offset").toUInt();

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
        qDebug() << "File: " << ba.size();
        *data = ba;
        return ba.size() > 0;
    }
    apxMsgW() << tr("Error parsing firmware file") << QString("(%1)").arg(errString);
    return false;
}
//=============================================================================
bool Releases::loadFileMHX(QString ver, QByteArray *data)
{
    bool bErr = true;
    //parse version
    QString mname;
    uint iver;
    while (1) {
        if (!ver.contains('_'))
            break;
        mname = ver.left(ver.indexOf('_'));
        QString s = ver.mid(ver.indexOf('_') + 1);
        if (s.startsWith('v', Qt::CaseInsensitive))
            s.remove(0, 1);
        iver = s.toFloat() * 1000;
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
    setStatus(tr("Loading file") + "...");
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
//=============================================================================
