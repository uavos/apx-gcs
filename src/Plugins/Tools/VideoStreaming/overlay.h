#ifndef OVERLAY_H
#define OVERLAY_H

#include "Fact/Fact.h"
#include <QImage>

class Overlay : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QStringList varnames READ getVarNames NOTIFY varnamesChanged)
public:
    explicit Overlay(Fact *parent);

    Fact *f_crosshair;
    Fact *f_variables;

    QStringList getVarNames() const;

    void drawOverlay(QImage &image);

private:
    QStringList m_varnames;

private slots:
    void onVariablesValueChanged();

signals:
    void varnamesChanged();
};

#endif // OVERLAY_H
