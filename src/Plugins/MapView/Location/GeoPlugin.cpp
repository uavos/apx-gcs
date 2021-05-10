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
#include "GeoPlugin.h"
#include "GeoTiledMappingManagerEngine.h"
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

Q_EXTERN_C Q_DECL_EXPORT const char *qt_plugin_query_metadata();
Q_EXTERN_C Q_DECL_EXPORT QT_PREPEND_NAMESPACE(QObject) * qt_plugin_instance();

void GeoPlugin::init()
{
    //load static plugin
    Q_IMPORT_PLUGIN(GeoPlugin)
}

const QT_PREPEND_NAMESPACE(QStaticPlugin) qt_static_plugin_GeoPlugin()
{
    QT_PREPEND_NAMESPACE(QStaticPlugin) plugin = {qt_plugin_instance, qt_plugin_query_metadata};
    return plugin;
}

QGeoCodingManagerEngine *GeoPlugin::createGeocodingManagerEngine(const QVariantMap &,
                                                                 QGeoServiceProvider::Error *,
                                                                 QString *) const
{
    //qDebug()<<"createGeocodingManagerEngine";
    return nullptr;
    //return new QGeoCodingManagerEngineAPX(parameters, error, errorString);
}

QGeoMappingManagerEngine *GeoPlugin::createMappingManagerEngine(const QVariantMap &parameters,
                                                                QGeoServiceProvider::Error *error,
                                                                QString *errorString) const
{
    //qDebug()<<"createMappingManagerEngine";
    return new GeoTiledMappingManagerEngine(parameters, error, errorString);
}

QGeoRoutingManagerEngine *GeoPlugin::createRoutingManagerEngine(const QVariantMap &,
                                                                QGeoServiceProvider::Error *,
                                                                QString *) const
{
    return nullptr;
}

QPlaceManagerEngine *GeoPlugin::createPlaceManagerEngine(const QVariantMap &,
                                                         QGeoServiceProvider::Error *,
                                                         QString *) const
{
    return nullptr;
}
