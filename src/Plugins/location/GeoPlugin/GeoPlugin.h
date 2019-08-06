#ifndef GeoPlugin_H
#define GeoPlugin_H
#include <ApxPluginInterface.h>
#include <TileLoader.h>
#include <QtCore/QObject>
#include <QtCore>
#include <QtLocation/QGeoServiceProviderFactory>
#include <QtPlugin>
//=============================================================================
class GeoPlugin : public ApxPluginInterface, public QGeoServiceProviderFactory
{
    Q_OBJECT
    Q_INTERFACES(QGeoServiceProviderFactory)
    Q_INTERFACES(ApxPluginInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.geoservice.serviceproviderfactory/5.0" FILE
                          "GeoPlugin.json")

public:
    void init();
    int flags() { return FeaturePlugin; }
    QString title() { return tr("Location"); }
    QString descr() { return tr("Location service provider"); }
    QObject *createControl() { return new TileLoader(); }

    //override
    QGeoCodingManagerEngine *createGeocodingManagerEngine(const QVariantMap &parameters,
                                                          QGeoServiceProvider::Error *error,
                                                          QString *errorString) const;
    QGeoMappingManagerEngine *createMappingManagerEngine(const QVariantMap &parameters,
                                                         QGeoServiceProvider::Error *error,
                                                         QString *errorString) const;
    QGeoRoutingManagerEngine *createRoutingManagerEngine(const QVariantMap &parameters,
                                                         QGeoServiceProvider::Error *error,
                                                         QString *errorString) const;
    QPlaceManagerEngine *createPlaceManagerEngine(const QVariantMap &parameters,
                                                  QGeoServiceProvider::Error *error,
                                                  QString *errorString) const;
};
//=============================================================================
#endif
