#ifndef GeoPlugin_H
#define GeoPlugin_H
#include <QtCore/QObject>
#include <QtLocation/QGeoServiceProviderFactory>
#include <QtPlugin>
#include <QtCore>
//=============================================================================
class GeoPlugin: public QObject, public QGeoServiceProviderFactory
{
  Q_OBJECT
  Q_INTERFACES(QGeoServiceProviderFactory)
  Q_PLUGIN_METADATA(IID "org.qt-project.qt.geoservice.serviceproviderfactory/5.0" FILE "GeoPlugin.json")

public:
  //override
  QGeoCodingManagerEngine*    createGeocodingManagerEngine    (const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const;
  QGeoMappingManagerEngine*   createMappingManagerEngine      (const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const;
  QGeoRoutingManagerEngine*   createRoutingManagerEngine      (const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const;
  QPlaceManagerEngine*        createPlaceManagerEngine        (const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const;
};
//=============================================================================
#endif
