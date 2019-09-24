#ifndef KMLOVERLAYPLUGIN_H
#define KMLOVERLAYPLUGIN_H

#include "kmloverlay.h"
#include <ApxPluginInterface.h>
#include <QtCore>

class KmlOverlayPlugin : public ApxPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.uavos.gcs.ApxPluginInterface/1.0")
    Q_INTERFACES(ApxPluginInterface)
public:
    int flags() override { return Feature | Map; }
    QObject *createControl() override { return new KmlOverlay(); }
    QStringList depends() override { return QStringList() << "MapView"; }
};

#endif //KMLOVERLAYPLUGIN_H
