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

#include <ApxMisc/JsonHelpers.h>
#include <Database/NodesReqMeta.h>

// TODO collect and display changelog based on minimum node version
// see https://doc.qt.io/qt-5/qtwebengine-webenginewidgets-markdowneditor-example.html

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

    connect(&_gh, &GithubReleases::latestVersionInfo, this, &ApxFw::latestVersionInfo);
    connect(&_gh, &GithubReleases::releaseInfo, this, &ApxFw::releaseInfo);
    connect(&_gh, &GithubReleases::downloadProgress, this, &ApxFw::downloadProgress);
    connect(&_gh, &GithubReleases::downloadFinished, this, &ApxFw::downloadFinished);
    connect(&_gh, &GithubReleases::finished, this, [this]() { setProgress(-1); });
    connect(&_gh, &GithubReleases::error, this, [this](QString msg) {
        apxMsgW() << title().append(':') << msg;
        setProgress(-1);
    });

    QTimer::singleShot(5000, this, &ApxFw::sync);
}

void ApxFw::sync()
{
    syncFacts();
    _gh.requestLatestVersionInfo();
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
        updateNodesMeta(devDir());
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
                mapDirs.insert(vPkg,
                               QFileInfo(
                                   QString("%1-%2").arg(m_packagePrefix).arg(vPkg.toString())));
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

    updateNodesMeta(dir);

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

void ApxFw::makeFacts(Fact *fact, QDir dir)
{
    //create content tree
    for (auto fi : dir.entryInfoList()) {
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

        auto fact_name = fi.completeBaseName().toLower().replace('.', '_');
        Fact *f = new Fact(f_hw, fact_name, fi.completeBaseName(), fi.lastModified().toString());
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

bool ApxFw::isFirmwarePackageFile(const QString &s)
{
    if (!s.startsWith(m_packagePrefix + "-"))
        return false;
    if (!s.endsWith(".zip"))
        return false;
    return true;
}

void ApxFw::latestVersionInfo(QVersionNumber version, QString tag)
{
    auto v_current = QVersionNumber::fromString(value().toString());
    if (v_current >= version) {
        qDebug() << "no updates:" << v_current << ">=" << version;
        return;
    }
    if (_versionPrefix.majorVersion() < version.majorVersion()
        && _versionPrefix.minorVersion() < version.minorVersion()) {
        qDebug() << "old gcs:" << v_current << ">=" << version << "prefix:" << _versionPrefix;
        return;
    }
    apxMsg() << title().append(':') << version.toString() << tr("available");
    _gh.requestReleaseInfo(tag);
}

void ApxFw::releaseInfo(QJsonDocument json)
{
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
        apxMsg() << title().append(':') << tr("Downloading firmware").append("...");
        qDebug() << "download" << url.toString();
        _gh.requestDownload(url);
        return;
    }
    apxMsgW() << title().append(':') << tr("Firmware not available");
    qWarning() << json;
}

void ApxFw::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        setProgress((bytesReceived * 100) / bytesTotal);
    }
}

void ApxFw::downloadFinished(QString assetName, QFile *data)
{
    if (!isFirmwarePackageFile(assetName)) {
        apxMsgW() << title().append(':') << tr("Unknown response") << assetName;
        return;
    }

    QDir dir(AppDirs::firmware().absoluteFilePath("releases"));
    dir.mkpath(".");
    auto newFileName = dir.absoluteFilePath(assetName);

    if (!data->copy(newFileName)) {
        apxMsgW() << tr("Can't copy file:") << newFileName;
        return;
    }

    qDebug() << "downloaded" << newFileName << QFileInfo(newFileName).size();

    if (extractRelease(newFileName)) {
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

    auto app_ver = QVersionNumber::fromString(App::version());

    //search fw package
    QDir dir(releaseDir().absolutePath(), QString("%1-*.apxfw").arg(fname));
    foreach (QFileInfo fi, dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        QStringList st(fi.completeBaseName().split('-'));
        if (st.size() < ccnt)
            continue;
        if (st.size() > ccnt && st.at(ccnt) != target_os)
            continue;

        auto ver = QVersionNumber::fromString(st.at(ccnt - 1));
        vmap[ver] = fi.absoluteFilePath();
    }

    //search dev files
    dir = QDir(devDir().absolutePath(), QString("%1-*.apxfw").arg(fname));
    foreach (QFileInfo fi, dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        QStringList st(fi.completeBaseName().split('-'));
        if (st.size() < ccnt)
            continue;
        if (st.size() > ccnt && st.at(ccnt) != target_os)
            continue;

        auto ver = QVersionNumber::fromString(st.at(ccnt - 1));
        vmap[ver] = fi.absoluteFilePath(); // dev same version always overrides
    }

    // match only major.minor
    for (auto k : vmap.keys()) {
        if (k.majorVersion() == app_ver.majorVersion() && k.minorVersion() == app_ver.minorVersion())
            continue;
        vmap.remove(k);
    }

    QString fileName;
    if (!vmap.isEmpty()) {
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
        auto jso = doc.object();
        if (jso.value("magic").toString() != "APXFW") {
            errString = "magic";
            break;
        }

        auto jso_sect = jso.value(section).toObject();
        if (jso_sect.isEmpty()) {
            errString = "missing section '" + section + "'";
            break;
        }

        auto size = jso_sect.value("size").toVariant().toUInt();
        if (!size) {
            errString = "zero size";
            break;
        }

        if (startAddr) {
            if (!jso_sect.contains("origin")) {
                errString = "missing origin";
                break;
            }
            bool ok;
            *startAddr = jso_sect.value("origin").toString().toUInt(&ok, 16);
            if (!ok) {
                errString = "missing origin convert error";
                break;
            }
        }

        QByteArray ba = QByteArray::fromBase64(jso_sect.value("data").toString().toUtf8());
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

void ApxFw::updateNodesMeta(QDir dir)
{
    QJsonObject meta;

    for (auto fi : dir.entryInfoList(QStringList() << "*.apxfw", QDir::Files)) {
        if (fi.suffix() != "apxfw")
            continue;

        auto st = fi.completeBaseName().split('-');
        auto version = QVersionNumber::fromString(fi.completeBaseName().split('-').value(2))
                           .toString();
        if (st.size() < 3 || version.isEmpty()) {
            qWarning() << "invalid firmware file" << fi.filePath();
            continue;
        }

        QFile file(fi.absoluteFilePath());
        if (!file.exists() || !file.open(QFile::ReadOnly | QFile::Text))
            continue;

        QJsonDocument json = QJsonDocument::fromJson(file.readAll());
        file.close();

        QJsonObject params = json["parameters"].toObject();
        if (params.isEmpty())
            continue;
        QByteArray ba = QByteArray::fromBase64(params["data"].toString().toUtf8());
        if (ba.isEmpty())
            continue;

        quint32 size = params["size"].toInt();

        ba.prepend(static_cast<char>(size));
        ba.prepend(static_cast<char>(size >> 8));
        ba.prepend(static_cast<char>(size >> 16));
        ba.prepend(static_cast<char>(size >> 24));
        ba = qUncompress(ba);
        if (ba.isEmpty())
            continue;

        json = QJsonDocument::fromJson(ba);
        if (!json.isArray())
            continue;

        for (const auto &i : json.array()) {
            updateNodesMeta(&meta, version, i, QStringList());
        }
    }

    meta = json::fix_numbers(json::filter_names(meta));
    if (meta.isEmpty())
        return;

    // push to DB
    auto req = new db::nodes::SaveFieldMeta(meta);
    req->exec();
}

void ApxFw::updateNodesMeta(QJsonObject *meta,
                            QString version,
                            const QJsonValue &jsv,
                            QStringList path)
{
    if (!jsv.isObject())
        return;

    const auto jso = jsv.toObject();
    if (!jso.contains("name"))
        return;
    auto name = jso.value("name").toString();
    if (name.isEmpty())
        return;
    path << name;
    name = path.join('.');

    bool is_group = jso.contains("content");

    auto m = (*meta)[name].toObject();

    do {
        if (m.contains("version")) {
            auto mver = QVersionNumber::fromString(m.value("version").toString());
            auto nver = QVersionNumber::fromString(version);
            if (nver < mver)
                break; // downgrade not allowed
            m = {};
        }

        auto descr = jso["descr"].toString();
        auto title = jso["title"].toString();

        if (!is_group) {
            if (descr.isEmpty()) {
                descr = title;
            } else if (!title.isEmpty()) {
                descr.prepend(title + "\n");
            }
        }

        descr = descr.trimmed();

        if (descr.isEmpty())
            descr = m["descr"].toString();

        m["version"] = version;
        m["descr"] = descr;

        // default values
        if (jso.contains("default")) {
            auto def = jso["default"];
            if (def.isObject()) {
                // expand object defaults to separate sub fields
                const auto jsdef = def.toObject();
                for (auto it = jsdef.begin(); it != jsdef.end(); ++it) {
                    auto s = name + "." + it.key();
                    (*meta)[s] = (*meta)[s].toObject().value("def") = it.value();
                }
            } else {
                m["def"] = def;
            }
        }

        for (auto n : {"min", "max", "increment", "decimal"}) {
            m[n] = jso[n];
        }

        (*meta)[name] = m;
    } while (0);

    // parse child objects
    if (!is_group)
        return;

    for (const auto &v : jso["content"].toArray()) {
        updateNodesMeta(meta, version, v, path);
    }
}
