#ifndef GeoPlugin_H
#define GeoPlugin_H
#include "TileLoader.h"
#include <ApxPluginInterface.h>
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
    void init() override;
    int flags() override { return Feature | Map; }
    QString title() override { return tr("Location"); }
    QString descr() override { return tr("Location service provider"); }
    QObject *createControl() override { return new TileLoader(); }

    //override
    QGeoCodingManagerEngine *createGeocodingManagerEngine(const QVariantMap &parameters,
                                                          QGeoServiceProvider::Error *error,
                                                          QString *errorString) const override;
    QGeoMappingManagerEngine *createMappingManagerEngine(const QVariantMap &parameters,
                                                         QGeoServiceProvider::Error *error,
                                                         QString *errorString) const override;
    QGeoRoutingManagerEngine *createRoutingManagerEngine(const QVariantMap &parameters,
                                                         QGeoServiceProvider::Error *error,
                                                         QString *errorString) const override;
    QPlaceManagerEngine *createPlaceManagerEngine(const QVariantMap &parameters,
                                                  QGeoServiceProvider::Error *error,
                                                  QString *errorString) const override;
};
//=============================================================================
#endif
