#include "GeoPlugin.h"
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include "GeoTiledMappingManagerEngine.h"
//=============================================================================
Q_EXTERN_C Q_DECL_EXPORT const char *qt_plugin_query_metadata();
Q_EXTERN_C Q_DECL_EXPORT QT_PREPEND_NAMESPACE(QObject) *qt_plugin_instance();
//=============================================================================
const QT_PREPEND_NAMESPACE(QStaticPlugin) qt_static_plugin_GeoPlugin()
{
  QT_PREPEND_NAMESPACE(QStaticPlugin) plugin = { qt_plugin_instance, qt_plugin_query_metadata};
  return plugin;
}
//=============================================================================
QGeoCodingManagerEngine*
GeoPlugin::createGeocodingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
  qDebug()<<"createGeocodingManagerEngine";
  return NULL;
  //return new QGeoCodingManagerEngineQGC(parameters, error, errorString);
}
//=============================================================================
QGeoMappingManagerEngine*
GeoPlugin::createMappingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
  qDebug()<<"createMappingManagerEngine";
  return new GeoTiledMappingManagerEngine(parameters, error, errorString);
}
//=============================================================================
QGeoRoutingManagerEngine*
GeoPlugin::createRoutingManagerEngine(
    const QVariantMap &, QGeoServiceProvider::Error *, QString *) const
{
  return NULL;
}
//=============================================================================
QPlaceManagerEngine*
GeoPlugin::createPlaceManagerEngine(
    const QVariantMap &, QGeoServiceProvider::Error *, QString *) const
{
  return NULL;
}
//=============================================================================
