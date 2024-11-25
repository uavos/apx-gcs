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

#include "cpl_minixml.h"
#include "cpl_string.h"
#include "gdal.h"
#include "gdal_version.h"
#include "ogr_spatialref.h"
#include <cmath>
#include <vector>

OfflineElevationDB::OfflineElevationDB(QString &path)
    : m_dbPath(path)
{}

double OfflineElevationDB::getElevationASTER(double latitude, double longitude)
{
    double elevation{NAN};
    auto fileName = createASTERFileName(latitude, longitude);
    if (!QFile::exists(fileName)) {
        return elevation;
    }
    qDebug() << "Opening file\n\n\n" << fileName;

    elevation = getElevationFromGeoFile(fileName, latitude, longitude);
    return elevation;
}

QString OfflineElevationDB::createASTERFileName(double latitude, double longitude)
{
    int lat = fabs(static_cast<int>(latitude));
    int lon = fabs(static_cast<int>(longitude));
    auto fileName = QString("ASTGTMV003_%1%2%3%4_dem.tif")
                        .arg((latitude >= 0) ? 'N' : 'S')
                        .arg((latitude >= 0 ? lat : ++lat), 2, 10, QChar('0'))
                        .arg((longitude >= 0) ? 'E' : 'W')
                        .arg((longitude >= 0 ? lon : ++lon), 3, 10, QChar('0'));

    auto path = QString("%1/%2").arg(m_dbPath).arg(fileName);
    return path;
}

double OfflineElevationDB::getElevation(double latitude, double longitude)
{
    return getElevationASTER(latitude, longitude);
}

double OfflineElevationDB::getElevationFromGeoFile(QString &fileName,
                                                   double latitude,
                                                   double longitude)
{
    const char *srcFilename = nullptr;
    char *srcSRS = nullptr;
    std::vector<int> anBandList;
    double elevation{NAN};
    int nOverview = -1;

    GDALAllRegister();
    CPLFree(srcSRS);
    srcSRS = SanitizeSRS("WGS84");

    if (!srcSRS)
        return NAN;

    srcFilename = fileName.toUtf8().constData();

    qDebug() << "Opening file" << srcFilename;

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
    double dfGeoX = longitude;
    double dfGeoY = latitude;

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

        if ((latitude != NAN && longitude != NAN)
            || (fscanf(stdin, "%lf %lf", &dfGeoX, &dfGeoY) != 2)) {
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

    return elevation;
}

char *OfflineElevationDB::SanitizeSRS(const char *userInput)
{
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    char *result = nullptr;

    if (OSRSetFromUserInput(hSRS, userInput) == OGRERR_NONE)
        OSRExportToWkt(hSRS, &result);
    else {
        apxMsgW() << tr("Error %1. Translating source or target SRS failed: %2")
                         .arg(CPLE_AppDefined)
                         .arg(userInput);
        return nullptr;
    }

    OSRDestroySpatialReference(hSRS);

    return result;
}
