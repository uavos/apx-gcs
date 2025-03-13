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

#include "ElevationDB.h"
#include <App/AppLog.h>

#include <QFile>
#include <QFuture>
#include <QtConcurrent>

#include <cmath>
#include <vector>

#ifdef Q_OS_LINUX
#include "cpl_minixml.h"
#include "cpl_string.h"
#include "gdal.h"
#include "gdal_version.h"
#include "ogr_spatialref.h"
#endif

void AbstractElevationDB::receiveCoordinate(const QGeoCoordinate &coordinate)
{
    if(!coordinate.isValid()) {
        QGeoCoordinate checking(coordinate.latitude(), coordinate.longitude());
        if(!checking.isValid()) {
            apxMsgW() << tr("Invalid coordinate %1, %2, %3")
                                 .arg(coordinate.latitude())
                                 .arg(coordinate.longitude())
                                 .arg(coordinate.altitude());
            return;
        }
    }
    emit coordinateReceived(coordinate);
}

OfflineElevationDB::OfflineElevationDB(const QString &path)
    : m_dbPath(path)
{
    m_paths << "/usr/bin/"
            << "/usr/local/bin/"
            << "/opt/bin/"
            << "/opt/homebrew/bin/";
    connect(this, &OfflineElevationDB::utilChanged, this, &OfflineElevationDB::updateUtilPath);
}


QString OfflineElevationDB::createASTERFileName(double lat, double lon)
{
    int la = fabs(static_cast<int>(lat));
    int lo = fabs(static_cast<int>(lon));
    auto fileName = QString("ASTGTMV003_%1%2%3%4_dem.tif")
                        .arg((lat >= 0) ? 'N' : 'S')
                        .arg((lat >= 0 ? la : ++la), 2, 10, QChar('0'))
                        .arg((lon >= 0) ? 'E' : 'W')
                        .arg((lon >= 0 ? lo : ++lo), 3, 10, QChar('0'));

    auto path = QString("%1/%2").arg(m_dbPath).arg(fileName);
    return path;
}

void OfflineElevationDB::setUtil(Util util)
{
    if (m_util == util)
        return;
    
    m_util = util;
    emit utilChanged();
}

double OfflineElevationDB::getElevationTiffASTER(const QImage &image, const QString &file, double lat, double lon)
{
    double temp;
    auto modY = std::modf(lat, &temp);
    auto modX = std::modf(lon, &temp);
    if (modY < 0)
        modY++;
    if (modX < 0)
        modX++;

    double elevation{NAN};
    auto imageHeight = image.height();
    auto imageWidht = image.width();
    if (imageHeight == 0 || imageWidht == 0) {
        apxMsgW() << tr("Location is off this file").append(": ") << file;
        return elevation;
    }

    int pixelY = static_cast<int>(0.5 + std::abs((imageHeight - 1) * (1 - modY)));
    int pixelX = static_cast<int>(0.5 + std::abs((imageWidht - 1) * modX));
    if (pixelY >= imageHeight)
        pixelY = imageHeight - 1;
    if (pixelX >= imageWidht)
        pixelX = imageWidht - 1;

    uchar *src = const_cast<uchar *>(image.scanLine(pixelY));
    const short *line = reinterpret_cast<short *>(src);
    elevation = static_cast<double>(line[pixelX]);

    return elevation;
}

// Command example: "gdallocationinfo -wgs84 -valonly ASTGTMV003_N27E070_dem.tif 70.50 27.05"
// ATTENTION! Input order longitude-latitude: "gdallocationinfo <report parameters> <filename> <longitude> <latitude>"
// Documentation https://gdal.org/programs/gdallocationinfo.html
double OfflineElevationDB::getElevationGdallocationInfo(const QString &util, const QString &file, double lat, double lon)
{
    double elevation{NAN};
    if (util.isEmpty())
        return elevation;

    bool ok{false};
    auto command = QString("%1 -wgs84 -valonly %2 %3 %4").arg(util).arg(file).arg(lon).arg(lat);
    auto result = getDataFromGdallocationInfo(command);
    elevation = result.toDouble(&ok);

    return ok ? elevation : NAN;
}

QGeoCoordinate OfflineElevationDB::requestCoordinateTiffASTER(const QImage &image, const QString &file, double lat, double lon)
{
    auto elv = getElevationTiffASTER(image, file, lat, lon);
    return QGeoCoordinate(lat, lon, elv);
}

QGeoCoordinate OfflineElevationDB::requestCoordinateGdallocationInfo(const QString &util, const QString &file, double lat, double lon)
{
    double elv = getElevationGdallocationInfo(util, file, lat, lon);
    return QGeoCoordinate(lat, lon, elv);
}

QString OfflineElevationDB::getDataFromGdallocationInfo(const QString &command)
{
    QProcess p;
    p.startCommand(command);

    p.waitForFinished();
    auto result = QString(p.readAllStandardOutput());
    if (result.isEmpty())
        apxMsgW() << tr("Gdallocationinfo empty report! Command line: %1").arg(command);

    return result;
}

QString OfflineElevationDB::searchUtil(const QString &name)
{
    QProcess p;
    auto command = QString("which %1").arg(name);
    p.startCommand(command);
    p.waitForFinished();
    auto result = QString(p.readAllStandardOutput());
    if (!result.isEmpty())
        return result;
    for (const auto &p : m_paths)
        if (QFile::exists(p + name))
            return p + name;
    apxMsgW() << tr("Util %1 not found!").arg(name);
    return result;
}

void OfflineElevationDB::updateUtilPath() {
    m_utilPath = "";
    if(m_util == GDALLOCATIONINFO)
        m_utilPath = searchUtil("gdallocationinfo");
    if (!m_utilPath.isEmpty())
        apxMsg() << tr("The gdallocationinfo util is used");
}


void OfflineElevationDB::requestCoordinate(double latitude, double longitude) {
    requestCoordinateASTER(latitude, longitude);
}

void OfflineElevationDB::requestCoordinateASTER(double latitude, double longitude)
{
    auto fileName = createASTERFileName(latitude, longitude);
    if (!QFile::exists(fileName)) {
        emit coordinateReceived(QGeoCoordinate(latitude,longitude));
        return;
    }

    QFuture<QGeoCoordinate> future;
    if (m_util == GDALLOCATIONINFO) {
        future = QtConcurrent::run(requestCoordinateGdallocationInfo, m_utilPath, fileName, latitude, longitude);
    } else {
        setImage(fileName);
        future = QtConcurrent::run(requestCoordinateTiffASTER, m_image, fileName, latitude, longitude);
    }
    QFutureWatcher<QGeoCoordinate> *watcher = new QFutureWatcher<QGeoCoordinate>(this);
    connect(watcher, &QFutureWatcher<QGeoCoordinate>::finished, this, [watcher, this]() {
        auto result = watcher->result();
        this->receiveCoordinate(result);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void OfflineElevationDB::setImage(const QString &fileName)
{
    if (fileName.isEmpty())
        return;

    if (m_fileName != fileName) {
        m_fileName = fileName;
        m_image = QImage(m_fileName);
    }
}

double OfflineElevationDB::getElevationFromGeoFile(QString file, double lat, double lon)
{
    double elevation{NAN};

#ifdef Q_OS_LINUX
    char *srcSRS = nullptr;
    std::vector<int> anBandList;
    int nOverview = -1;

    GDALAllRegister();
    CPLFree(srcSRS);
    srcSRS = SanitizeSRS("WGS84");

    if (!srcSRS)
        return NAN;

    auto src = file.toStdString();
    auto srcFilename = src.c_str();

    // Open source file
    GDALDatasetH hSrcDS = GDALOpenEx(srcFilename,
                                     GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR,
                                     nullptr,
                                     nullptr,
                                     nullptr);

    if (hSrcDS == nullptr)
        return NAN;

    // Setup coordinate transformation, if required
    OGRSpatialReferenceH hSrcSRS = nullptr;
    OGRCoordinateTransformationH hCT = nullptr;
    if (srcSRS != nullptr && !EQUAL(srcSRS, "-geoloc")) {
        hSrcSRS = OSRNewSpatialReference(srcSRS);
        OSRSetAxisMappingStrategy(hSrcSRS, OAMS_TRADITIONAL_GIS_ORDER);
        auto hTrgSRS = GDALGetSpatialRef(hSrcDS);
        if (!hTrgSRS)
            return NAN;

        hCT = OCTNewCoordinateTransformation(hSrcSRS, hTrgSRS);
        if (hCT == nullptr)
            return NAN;
    }

    // If no bands were requested, we will query them all
    if (anBandList.empty()) {
        for (int i = 0; i < GDALGetRasterCount(hSrcDS); i++)
            anBandList.push_back(i + 1);
    }

    // Turn the location into a pixel and line location.
    bool inputAvailable = true;
    double dfGeoX = lon;
    double dfGeoY = lat;

    while (inputAvailable) {
        int iPixel;
        int iLine;

        if (hCT) {
            if (!OCTTransform(hCT, 1, &dfGeoX, &dfGeoY, nullptr))
                return NAN;
        }

        if (srcSRS != nullptr) {
            double adfGeoTransform[6] = {};
            if (GDALGetGeoTransform(hSrcDS, adfGeoTransform) != CE_None) {
                apxMsgW() << tr("Error %1. Cannot get geotransform.").arg(CPLE_AppDefined);
                return NAN;
            }

            double adfInvGeoTransform[6] = {};
            if (!GDALInvGeoTransform(adfGeoTransform, adfInvGeoTransform)) {
                apxMsgW() << tr("Error %1. Cannot invert geotransform.").arg(CPLE_AppDefined);
                return NAN;
            }

            iPixel = static_cast<int>(floor(adfInvGeoTransform[0] + adfInvGeoTransform[1] * dfGeoX
                                            + adfInvGeoTransform[2] * dfGeoY));
            iLine = static_cast<int>(floor(adfInvGeoTransform[3] + adfInvGeoTransform[4] * dfGeoX
                                           + adfInvGeoTransform[5] * dfGeoY));
        } else {
            iPixel = static_cast<int>(floor(dfGeoX));
            iLine = static_cast<int>(floor(dfGeoY));
        }

        CPLString osLine;
        bool bPixelReport = true;

        if (iPixel < 0 || iLine < 0 || iPixel >= GDALGetRasterXSize(hSrcDS)
            || iLine >= GDALGetRasterYSize(hSrcDS)) {
            apxMsgW() << tr("Location is off this file! No further details to report.");
            bPixelReport = false;
        }

        // Process each band
        for (int i = 0; bPixelReport && i < static_cast<int>(anBandList.size()); i++) {
            GDALRasterBandH hBand = GDALGetRasterBand(hSrcDS, anBandList[i]);
            int iPixelToQuery = iPixel;
            int iLineToQuery = iLine;

            if (nOverview >= 0 && hBand != nullptr) {
                GDALRasterBandH hOvrBand = GDALGetOverview(hBand, nOverview);
                if (hOvrBand != nullptr) {
                    auto nOvrXSize = GDALGetRasterBandXSize(hOvrBand);
                    auto nOvrYSize = GDALGetRasterBandYSize(hOvrBand);
                    iPixelToQuery = static_cast<int>(
                        0.5 + 1.0 * iPixel / GDALGetRasterXSize(hSrcDS) * nOvrXSize);
                    iLineToQuery = static_cast<int>(
                        0.5 + 1.0 * iLine / GDALGetRasterYSize(hSrcDS) * nOvrYSize);
                    if (iPixelToQuery >= nOvrXSize)
                        iPixelToQuery = nOvrXSize - 1;
                    if (iLineToQuery >= nOvrYSize)
                        iLineToQuery = nOvrYSize - 1;
                } else {
                    apxMsgW() << tr("Error %1. Cannot get overview %2 of band %3")
                                     .arg(CPLE_AppDefined)
                                     .arg(nOverview + 1)
                                     .arg(anBandList[i]);
                }
                hBand = hOvrBand;
            }

            if (hBand == nullptr)
                continue;

            double adfPixel[2] = {0, 0};
            const bool bIsComplex = CPL_TO_BOOL(GDALDataTypeIsComplex(GDALGetRasterDataType(hBand)));

            if (GDALRasterIO(hBand,
                             GF_Read,
                             iPixelToQuery,
                             iLineToQuery,
                             1,
                             1,
                             adfPixel,
                             1,
                             1,
                             bIsComplex ? GDT_CFloat64 : GDT_Float64,
                             0,
                             0)
                == CE_None) {
                CPLString osValue;

                if (bIsComplex)
                    osValue.Printf("%.15g+%.15gi", adfPixel[0], adfPixel[1]);
                else
                    osValue.Printf("%.15g", adfPixel[0]);

                elevation = std::stod(osValue.c_str());
            }
        }

        if ((lat != NAN && lon != NAN) || (fscanf(stdin, "%lf %lf", &dfGeoX, &dfGeoY) != 2)) {
            inputAvailable = false;
        }
    }

    // Cleanup
    if (hCT) {
        OSRDestroySpatialReference(hSrcSRS);
        OCTDestroyCoordinateTransformation(hCT);
    }

    GDALClose(hSrcDS);
    GDALDumpOpenDatasets(stderr);
    GDALDestroyDriverManager();
    CPLFree(srcSRS);
#endif

    return elevation;
}

char *OfflineElevationDB::SanitizeSRS(const char *userInput)
{
    char *result = nullptr;

#ifdef Q_OS_LINUX
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (OSRSetFromUserInput(hSRS, userInput) == OGRERR_NONE)
        OSRExportToWkt(hSRS, &result);
    else {
        apxMsgW() << tr("Error %1. Translating source or target SRS failed: %2")
                         .arg(CPLE_AppDefined)
                         .arg(userInput);
        return nullptr;
    }

    OSRDestroySpatialReference(hSRS);
#endif

    return result;
}
