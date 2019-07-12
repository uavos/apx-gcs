#ifndef KMLOVERLAY_H
#define KMLOVERLAY_H

#include <QtCore>
#include <QtLocation>
#include <Fact/Fact.h>

class KmlOverlay : public Fact
{
    Q_OBJECT
public:
    explicit KmlOverlay(Fact *parent = nullptr);
};

#endif //KMLOVERLAY_H
