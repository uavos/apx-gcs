#ifndef KMLOVERLAYPLUGIN_H
#define KMLOVERLAYPLUGIN_H

#include <QtCore>
#include <ApxPluginInterface.h>
#include "kmloverlay.h"

class KmlOverlayPlugin : public ApxPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.uavos.gcs.ApxPluginInterface/1.0")
    Q_INTERFACES(ApxPluginInterface)
public:
    QObject *createControl() { return new KmlOverlay(); }
};

#endif //KMLOVERLAYPLUGIN_H
